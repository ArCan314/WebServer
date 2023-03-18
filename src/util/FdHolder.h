#pragma once

#include <unistd.h>

#include "./Noncopyable.h"
#include "../Logger.h"

class FdHolder : NonCopyable
{
private:
    int fd_;

public:
    FdHolder(FdHolder &&other) : fd_(other.fd_)
    {
        // LOG_DEBUG("Hold fd ", fd_);
        other.fd_ = -1;
    }
    explicit FdHolder(int fd) : fd_(fd)
    {
        // LOG_DEBUG("Hold fd ", fd_);
    }
    int fd() const noexcept { return fd_; }
    ~FdHolder()
    {
        // LOG_DEBUG("Release fd ", fd_);
        if (fd_ > 0)
            close(fd_);
    }
};