#include <algorithm>
#include <string>
#include <string_view>
#include <functional>

#include <cstring>
#include <cassert>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "./util/utils.h"
#include "./util/FdHolder.h"
#include "./HttpContext.h"
#include "./HttpParser.h"
#include "./HttpResponseBuilder.h"
#include "./HttpTypes.h"
#include "./TcpSocket.h"
#include "./Logger.h"

void HttpContext::setErrorResponse(HttpStatusCode error_status_code)
{
    write_buffer_ = HttpResponseBuilder(error_status_code).build();
    write_index_ = 0;
    state_ = State::SEND_ERROR;
}

int HttpContext::__recv(std::string &read_buf)
{
    int pos = 0, retval = 0, total = 0;
    while ((retval = ::recv(socket_->fd(), temp_read_buffer_.begin() + pos, std::size(temp_read_buffer_) - pos, MSG_DONTWAIT)) > 0)
    {
        pos += retval;
        if (pos == std::size(temp_read_buffer_))
        {
            total += pos;
            read_buf.reserve(read_buf.size() + std::size(temp_read_buffer_));
            std::copy(std::begin(temp_read_buffer_), std::end(temp_read_buffer_), std::back_inserter(read_buf));
            pos = 0;
        }
    }
    if (retval == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        LOG_ERROR("Receive error, reason: ", logErrStr(errno));
        return -1;
    }

    read_buf.reserve(read_buf.size() + pos);
    std::copy(std::begin(temp_read_buffer_), std::begin(temp_read_buffer_) + pos, std::back_inserter(read_buffer_));
    return total + pos;
}

HttpContext::HttpReadResult HttpContext::recvTillEnd()
{
    const int prev_buffer_size = read_buffer_.size();
    if (const int retval = __recv(read_buffer_); retval == -1)
        return HttpReadResult::ERROR;
    else if (retval == 0)
        return HttpReadResult::PEER_CLOSED;

    static constexpr char needle[] = "\r\n\r\n";
    thread_local auto searcher = std::boyer_moore_searcher(std::begin(needle), std::prev(std::end(needle)));
    auto iter = std::search(read_buffer_.begin() + prev_buffer_size, read_buffer_.end(), searcher);
    if (iter == read_buffer_.end())
        return HttpReadResult::NOT_READY;
    return HttpReadResult::READY;
}

HttpContext::HttpReadResult HttpContext::recvBody()
{
    const int recv_len = __recv(body_buffer_);
    if (const int retval = __recv(read_buffer_); retval == -1)
        return HttpReadResult::ERROR;
    else if (retval == 0)
        return HttpReadResult::PEER_CLOSED;

    to_read_body_bytes_ -= recv_len;
    if (to_read_body_bytes_ <= 0)
    {
        if (to_read_body_bytes_ < 0)
        {
            LOG_WARNING("Size of body_buffer_(", body_buffer_.size(), ") is larger than expected(", ((int)body_buffer_.size()) + to_read_body_bytes_, "), body_buffer_: ", body_buffer_);
            body_buffer_.resize(body_buffer_.size() + to_read_body_bytes_);
        }
        return HttpReadResult::READY;
    }
    return HttpReadResult::NOT_READY;
}

int HttpContext::sendAll()
{
    int retval;
    if (write_index_ != write_buffer_.size())
    {
        while ((retval = ::send(socket_->fd(), write_buffer_.data() + write_index_, write_buffer_.size() - write_index_, MSG_DONTWAIT)) > 0)
            write_index_ += retval;

        if (retval == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            if (errno != EPIPE)
                LOG_ERROR("Send error, reason: ", logErrStr(errno));
            write_file_fd_ = nullptr;
            return -1;
        }
    }

    if (write_index_ == write_buffer_.size() && write_file_fd_)
    {
        while ((retval = ::sendfile(socket_->fd(), write_file_fd_->first.fd(), &write_file_offset_, write_file_fd_->second)) > 0)
            if (write_file_offset_ == write_file_fd_->second)
                break;

        if (retval == -1 && retval != EAGAIN && retval != EWOULDBLOCK)
        {
            if (errno != EPIPE)
                LOG_ERROR("Send error, reason: ", logErrStr(errno));
            write_file_fd_ = nullptr;
            return -1;
        }

        if (write_file_offset_ == write_file_fd_->second)
            write_file_fd_ = nullptr;
    }

    return 0;
}

void HttpContext::handleStateRecvHead()
{
    auto read_res = recvTillEnd();
    if (read_res == HttpReadResult::NOT_READY)
        epollModOneShot(epoll_fd_, EPOLLIN /*|EPOLLET*/, socket_->fd());
    else if (read_res == HttpReadResult::READY)
    {
        // LOG_DEBUG("Receive request header:\n", read_buffer_);
        const int parse_res = parser_.parse(read_buffer_);
        if (!parse_res)
        {
            LOG_DEBUG("Failed to parse request");
            setErrorResponse(HttpStatusCode::BAD_REQUEST);
        }
        else
        {
            auto content_length = parser_.getContentLength();
            if (content_length)
            {
                // LOG_DEBUG("Request content-length > 0, change state_ to State::RECEIVE_BODY");
                body_buffer_.reserve(content_length);
                if (parse_res < read_buffer_.size())
                    body_buffer_.append(read_buffer_.substr(parse_res, content_length));

                to_read_body_bytes_ = content_length - body_buffer_.size();
                if (to_read_body_bytes_ > 0)
                {
                    state_ = State::RECEIVE_BODY;
                    if (epollModOneShot(epoll_fd_, EPOLLIN /*|EPOLLET*/, socket_->fd()) == -1)
                        LOG_ERROR("Epoll oneshot event EPOLLIN modify failed for fd(", socket_->fd(), "), reason: ", logErrStr(errno));
                    return;
                }
            }
            handleRequest();
        }

        if (epollModOneShot(epoll_fd_, EPOLLOUT /*|EPOLLET*/, socket_->fd()) == -1)
            LOG_ERROR("Epoll oneshot event EPOLLOUT modify failed for fd(", socket_->fd(), "), reason: ", logErrStr(errno));
    }
    else if (read_res == HttpReadResult::ERROR)
    {
        setErrorResponse(HttpStatusCode::INTERNAL_SERVER_ERROR);
        if (epollModOneShot(epoll_fd_, EPOLLOUT /*|EPOLLET*/, socket_->fd()) == -1)
        {
            LOG_ERROR("Epoll oneshot event EPOLLOUT modify failed for fd(", socket_->fd(), "), reason: ", logErrStr(errno));
            remove_connection_callback_(socket_->fd());
        }
    }
    else if (read_res == HttpReadResult::PEER_CLOSED)
    {
        LOG_DEBUG("Peer connection closed");
        epollDel(epoll_fd_, socket_->fd());
        remove_connection_callback_(socket_->fd());
    }
    else
    {
        assert(0); // HttpReadResult not handled
    }
}

void HttpContext::handleStateRecvBody()
{
    auto read_res = recvBody();
    if (read_res == HttpReadResult::NOT_READY)
        epollModOneShot(epoll_fd_, EPOLLIN /*|EPOLLET*/, socket_->fd());
    else if (read_res == HttpReadResult::READY)
    {
        handleRequest();
        // if (epollModOneShot(epoll_fd_, EPOLLOUT, socket_->fd()) == -1)
        //     LOG_ERROR("Epoll oneshot event EPOLLOUT modify failed for fd(", socket_->fd(), "), reason: ", logErrStr(errno));
        if (epollModOneShot(epoll_fd_, EPOLLOUT /*|EPOLLET*/, socket_->fd()) == -1)
            LOG_ERROR("Epoll oneshot event EPOLLOUT modify failed for fd(", socket_->fd(), "), reason: ", logErrStr(errno));
    }
    else if (read_res == HttpReadResult::ERROR)
    {
        setErrorResponse(HttpStatusCode::INTERNAL_SERVER_ERROR);
        // if (epollModOneShot(epoll_fd_, EPOLLOUT, socket_->fd()) == -1)
        // {
        //     LOG_ERROR("Epoll oneshot event EPOLLOUT modify failed for fd(", socket_->fd(), "), reason: ", logErrStr(errno));
        //     remove_connection_callback_(socket_->fd());
        // }
        if (epollModOneShot(epoll_fd_, EPOLLOUT /*|EPOLLET*/, socket_->fd()) == -1)
        {
            LOG_ERROR("Epoll oneshot event EPOLLOUT modify failed for fd(", socket_->fd(), "), reason: ", logErrStr(errno));
            remove_connection_callback_(socket_->fd());
        }
    }
    else if (read_res == HttpReadResult::PEER_CLOSED)
    {
        LOG_DEBUG("Peer connection closed");
        epollDel(epoll_fd_, socket_->fd());
        remove_connection_callback_(socket_->fd());
    }
    else
    {
        assert(0); // HttpReadResult not handled
    }
}

void HttpContext::doRead()
{
    LOG_DEBUG("HttpContext doRead(), this = ", (long)this);
    if (state_ == State::RECEIVE_HEAD)
        handleStateRecvHead();
    else if (state_ == State::RECEIVE_BODY)
        handleStateRecvBody();
    else
        LOG_WARNING("Unhandled state: ", static_cast<int>(state_));
}

void HttpContext::doWrite()
{
    LOG_DEBUG("HttpContext doWrite(), this = ", (long)this);
    const int retval = sendAll();
    if (retval == -1)
    {
        if (epollDel(epoll_fd_, socket_->fd()) == -1)
            LOG_ERROR("Epoll event delete failed for fd(", socket_->fd(), "), reason: ", logErrStr(errno));
        remove_connection_callback_(socket_->fd());
        return;
    }

    if (write_index_ != write_buffer_.size() || write_file_fd_)
    {
        // if (epollModOneShot(epoll_fd_, EPOLLOUT, socket_->fd()) == -1)
        // {
        //     LOG_ERROR("Epoll oneshot event EPOLLOUT modify failed for fd(", socket_->fd(), "), reason: ", logErrStr(errno));
        //     remove_connection_callback_(socket_->fd());
        // }
        if (epollModOneShot(epoll_fd_, EPOLLOUT /*|EPOLLET*/, socket_->fd()) == -1)
        {
            LOG_ERROR("Epoll oneshot event EPOLLOUT modify failed for fd(", socket_->fd(), "), reason: ", logErrStr(errno));
            remove_connection_callback_(socket_->fd());
        }
    }
    else
    {
        if (parser_.isKeepAlive() && state_ != State::SEND_ERROR)
        {
            reset();
            // if (epollModOneShot(epoll_fd_, EPOLLIN, socket_->fd()) == -1)
            if (epollModOneShot(epoll_fd_, EPOLLIN /*|EPOLLET*/, socket_->fd()) == -1)
            {
                LOG_ERROR("Epoll oneshot event EPOLLIN modify failed for fd(", socket_->fd(), "), reason: ", logErrStr(errno));
                remove_connection_callback_(socket_->fd());
            }
        }
        else
        {
            state_ = State::CLOSE;
            if (epollDel(epoll_fd_, socket_->fd()) == -1)
                LOG_ERROR("Epoll event delete failed for fd(", socket_->fd(), "), reason: ", logErrStr(errno));
            remove_connection_callback_(socket_->fd());
        }
    }
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
static std::string lexicalCast(T num)
{
    std::string res;
    if (num == 0)
        res.push_back('0');
    else
    {
        static constexpr std::size_t kBufferSize = 64;
        std::array<char, kBufferSize> buffer;
        int pos = std::size(buffer) - 1;
        while (num)
        {
            buffer[pos--] = '0' + num % 10;
            num /= 10;
        }

        res.append(&buffer[pos + 1], std::size(buffer) - pos - 1);
    }
    return res;
}

void HttpContext::handleRequest()
{
    // LOG_DEBUG("Handle request, method=",
    //           kHttpMethodStr[static_cast<int>(parser_.method())],
    //           ", version=HTTP", static_cast<int>(parser_.version()), ", url=", parser_.url());

    if (static_cast<int>(parser_.version()) > static_cast<int>(HttpVersion::HTTP11))
    {
        LOG_DEBUG("Http version ", static_cast<int>(parser_.version()), " is not supported");
        setErrorResponse(HttpStatusCode::HTTP_VERSION_NOT_SUPPORTED);
        return;
    }

    if (parser_.method() == HttpMethod::GET || parser_.method() == HttpMethod::HEAD)
    {
        std::string full_url = std::string(root_dir_).append(parser_.url());
        if (parser_.url() == "/")
            full_url.append("index.html");

        if (!hasFile(full_url))
        {
            LOG_DEBUG("Cannot find file, url = ", full_url);
            setErrorResponse(HttpStatusCode::NOT_FOUND);
            return;
        }

        if (access(full_url.data(), R_OK) == -1)
        {
            if (errno == EACCES)
                LOG_DEBUG("No read permission on file ", full_url);
            else
                LOG_WARNING("Failed to call access with parameter(", full_url, "), reason: ", logErrStr(errno));
            setErrorResponse(HttpStatusCode::INTERNAL_SERVER_ERROR);
            return;
        }

        const int file_fd = ::open(full_url.data(), O_RDONLY);
        if (file_fd == -1)
        {
            LOG_WARNING("Failed to call open with parameter(", full_url, "), reason: ", logErrStr(errno));
            setErrorResponse(HttpStatusCode::INTERNAL_SERVER_ERROR);
            return;
        }

        struct stat file_stat;
        explicit_bzero(&file_stat, sizeof(file_stat));
        if (fstat(file_fd, &file_stat) == -1)
        {
            LOG_WARNING("Failed to call fstat, reason: ", logErrStr(errno));
            setErrorResponse(HttpStatusCode::INTERNAL_SERVER_ERROR);
            return;
        }

        if (parser_.method() == HttpMethod::GET)
        {
            write_file_fd_ = std::make_unique<std::pair<FdHolder, std::size_t>>(file_fd, file_stat.st_size);
            write_file_offset_ = 0;
        }

        auto builder = HttpResponseBuilder()
                           .addHeader("Content-Length", lexicalCast(file_stat.st_size))
                           .addHeader("Content-Type", parser_.mime().data());
        if (parser_.isKeepAlive())
            builder.addHeader("Connection", "keep-alive");

        write_buffer_ = builder.build();
        write_index_ = 0;
        state_ = State::SEND;
        // LOG_DEBUG("Set response header: ", write_buffer_);
    }
    else
    {
        LOG_DEBUG("Http method ", kHttpMethodStr[static_cast<int>(parser_.method())], " is not supported.");
        setErrorResponse(HttpStatusCode::NOT_IMPLEMENTED);
        return;
    }
}

void HttpContext::reset()
{
    state_ = State::RECEIVE_HEAD;
    read_buffer_.clear();

    body_buffer_.clear();
    to_read_body_bytes_ = 0;

    write_buffer_.clear();
    write_index_ = 0;
    write_file_fd_ = nullptr;
    write_file_offset_ = 0;
}