#include "./TcpSocket.h"

#include <memory>
#include <string_view>
#include <optional>

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "./util/utils.h"
#include "./Logger.h"

TcpSocket::TcpSocket()
{
    fd_ = socket(PF_INET, SOCK_STREAM, 0);

    if (fd_ == -1)
    {
        auto err_no = errno;
        LOG_ERROR("Failed to create socket, errinfo = ", logErrStr(err_no));
        throw std::exception();
    }

    int sockopt_val = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &sockopt_val, sizeof(sockopt_val)) == -1)
        LOG_WARNING("Failed to set SO_REUSEADDR on socket_fd = ", fd_);
    LOG_DEBUG("Create socket with fd = ", fd_);
}

int TcpSocket::bind(std::string_view ip, int port) const
{
    const auto addr = inet_addr(ip.data());
    if (addr == INADDR_NONE)
    {
        LOG_ERROR("Invalid input for inet_addr, ip = ", ip);
        return -1;
    }

    sockaddr_in listen_addr;
    explicit_bzero((void *)&listen_addr, sizeof(listen_addr));

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);
    listen_addr.sin_addr.s_addr = addr;

    if (::bind(fd_, (sockaddr *)&listen_addr, sizeof(listen_addr)) == -1)
    {
        LOG_ERROR("Failed to bind(", ip, ", ", port, "), reason: ", logErrStr(errno));
        return -1;
    }

    return 0;
}

int TcpSocket::listen(int backlog) const
{
    if (::listen(fd_, backlog) == -1)
    {
        LOG_ERROR("Failed to listen on socket(fd=", fd_, "), reason: ", logErrStr(errno));
        return -1;
    }
    return 0;
}

std::optional<std::pair<std::string, uint16_t>> TcpSocket::getPeerAddress() const
{
    sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    explicit_bzero(&client_addr, sizeof(client_addr));

    if (::getpeername(fd_, (sockaddr *)&client_addr, &addrlen) == -1)
    {
        LOG_WARNING("Call getpeername fails on socket(fd=", fd_, "), reason: ", logErrStr(errno));
        return std::nullopt;
    }
    thread_local std::array<char, INET_ADDRSTRLEN> buffer;
    if (!inet_ntop(AF_INET, &client_addr.sin_addr, buffer.data(), std::size(buffer)))
    {
        LOG_WARNING("Failed to call inet_ntop, reason: ", logErrStr(errno));
        return std::nullopt;
    }

    return std::make_pair(std::string(buffer.data()), client_addr.sin_port);
}

int TcpSocket::accept() const 
{
    const int retval = ::accept(fd_, nullptr, nullptr);
    if (retval == -1)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            LOG_ERROR("Call ::accept fails on socket(fd=", fd_, "), reason: ", logErrStr(errno));
    }
    return retval;
} 

TcpSocket::~TcpSocket()
{
    LOG_DEBUG("Close socket fd ", fd_);
    if (::close(fd_) == -1)
        LOG_ERROR("Call close failed on fd ", fd_, ", reason: ", logErrStr(errno));
}

bool TcpSocket::setLinger(bool is_on, int second) const
{
    linger opt_linger;
    opt_linger.l_onoff = is_on;
    opt_linger.l_linger = second;

    const int retval = setsockopt(fd_, SOL_SOCKET, SO_LINGER, &opt_linger, sizeof(opt_linger));
    if (retval == -1)
    {
        LOG_WARNING("Failed to set SO_LINGER on fd ", fd_, ", params: ", is_on, ", ", second);
        return false;
    }
    return true;
}

bool TcpSocket::setReuseAddr(bool is_reuse) const
{
    int reuse = is_reuse;
    const int retval = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (retval == -1)
    {
        LOG_WARNING(logstr("Failed to setsockopt: SO_REUSEADDR to ", reuse, " on fd = ", fd(), ", reason = ", logErrStr(errno)));
        return false;
    }
    return true;
}

bool TcpSocket::setReusePort(bool is_reuse) const
{
    int reuse = is_reuse;
    const int retval = setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    if (retval == -1)
    {
        LOG_WARNING(logstr("Failed to setsockopt: SO_REUSEPORT to ", reuse, " on fd = ", fd(), ", reason = ", logErrStr(errno)));
        return false;
    }
    return true;
}

bool TcpSocket::setNonBlocking(bool is_non_blocking) const
{
    const int old_option = fcntl(fd_, F_GETFL);
    if (old_option == -1)
    {
        LOG_WARNING("Failed getting old_option on fd ", fd_);
        return false;
    }

    if (fcntl(fd_, F_SETFL, is_non_blocking ? (old_option | O_NONBLOCK) : (old_option & ~O_NONBLOCK)) == -1)
    {
        LOG_WARNING("Failed set fd(", fd_, ") to ", is_non_blocking ? "non blocking" : "blocking");
        return false;
    }

    return true;
}