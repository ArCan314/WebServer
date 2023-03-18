#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>

#include <inttypes.h>

#include "./HttpParser.h"
#include "./TimerQueue.h"
#include "./HttpResponseBuilder.h"
#include "./util/Noncopyable.h"

class TcpSocket;
class FdHolder;

class HttpContext : NonCopyable
{
public:
    HttpContext() = default;
    explicit HttpContext(std::unique_ptr<TcpSocket> &&socket,
                         int epoll_fd,
                         std::function<void(int)> remove_connection_callback,
                         std::string_view root_dir,
                         TimerQueue::TimerId timer_id);

    [[nodiscard]] TimerQueue::TimerId getTimerId() const noexcept { return timer_id_; }

    void doRead();
    void doWrite();

    ~HttpContext() { LOG_DEBUG("Destroy HttpContext ", (long)this); }

    void setContext(std::unique_ptr<TcpSocket> &&socket,
                    int epoll_fd,
                    std::function<void(int)> remove_connection_callback,
                    std::string_view root_dir,
                    TimerQueue::TimerId timer_id);
    void resetContext() { socket_ = nullptr; }

private:
    static constexpr int kReserveBufferSize = 1024;

    enum class HttpReadResult
    {
        NOT_READY,
        READY,
        ERROR,
        PEER_CLOSED,
    };

    enum class State
    {
        RECEIVE_HEAD,
        RECEIVE_BODY,
        SEND,
        SEND_ERROR,
        CLOSE,
    };

    HttpParser parser_;
    HttpResponseBuilder response_builder_;

    std::unique_ptr<TcpSocket> socket_;

    static constexpr std::size_t kTempReadBufferSize = 1024;
    std::array<uint8_t, kTempReadBufferSize> temp_read_buffer_;
    std::string read_buffer_;
    // int read_index_;

    std::string body_buffer_;
    int to_read_body_bytes_;

    std::string write_buffer_;
    int write_index_;

    std::unique_ptr<std::pair<FdHolder, std::size_t>> write_file_fd_;
    off_t write_file_offset_;

    int epoll_fd_;
    std::function<void(int)> remove_connection_callback_;
    std::string_view root_dir_;

    State state_{State::RECEIVE_HEAD};

    TimerQueue::TimerId timer_id_;

    [[nodiscard]] int __recv(std::string &read_buf);
    [[nodiscard]] HttpReadResult recvTillEnd();
    [[nodiscard]] HttpReadResult recvBody();

    [[nodiscard]] int sendAll();

    void handleStateRecvHead();
    void handleStateRecvBody();

    void handleRequest();
    void handleMethodGetAndHead();
    void handleMethodTrace();

    void reset();
    void setDefaultErrorResponse(HttpStatusCode, const std::string = "", bool is_method_head = false);
};