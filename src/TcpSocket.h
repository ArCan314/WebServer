#pragma once
#include <string_view>
#include <memory>
#include <optional>

#include <inttypes.h>
#include <endian.h>

#include <sys/socket.h>

#include "./util/Noncopyable.h"
#include "./util/utils.h"
#include "./Logger.h"

class TcpSocket : NonCopyable
{
public:
    using FdType = int;
    static constexpr int kListenBackLogSize = SOMAXCONN;
    static constexpr int kDefaultLingerSecond = 5;

    TcpSocket();
    explicit TcpSocket(int fd) : fd_(fd)
    {
        LOG_DEBUG("Move socket ", fd);
    }

    [[nodiscard]] int bind(std::string_view ip, int port) const;
    [[nodiscard]] int listen(int backlog = kListenBackLogSize) const;
    int fd() const noexcept { return fd_; }
    std::optional<std::pair<std::string, uint16_t>> getPeerAddress() const;
    [[nodiscard]] int accept() const;

    ~TcpSocket();

    [[nodiscard]] bool setLinger(bool is_on, int second = kDefaultLingerSecond) const;
    [[nodiscard]] bool setReuseAddr(bool /*is_reuse*/) const;
    [[nodiscard]] bool setReusePort(bool /*is_reuse*/) const;
    [[nodiscard]] bool setNonBlocking(bool /*is_non_blocking*/) const;

private:
    FdType fd_;
};