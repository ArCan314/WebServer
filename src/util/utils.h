#pragma once

#include <string>
#include <string_view>
#include <iostream>
#include <thread>

#include <string.h>

#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "../Logger.h"

inline int epollAdd(uint32_t epfd, uint32_t events, int fd)
{
    LOG_DEBUG("EPOLLADD epfd = ", epfd, ", fd = ", fd);
    epoll_event event;
    explicit_bzero(&event, sizeof(event));
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
}

inline int epollAddOneShot(uint32_t epfd, uint32_t events, int fd)
{
    return epollAdd(epfd, events | EPOLLONESHOT, fd);
}

inline int epollMod(uint32_t epfd, uint32_t events, int fd)
{
    LOG_DEBUG("EPOLLMOD epfd = ", epfd, ", fd = ", fd);
    epoll_event event;
    explicit_bzero(&event, sizeof(event));
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
}

inline int epollModOneShot(uint32_t epfd, uint32_t events, int fd)
{
    return epollMod(epfd, events | EPOLLONESHOT, fd);
}

inline int epollDel(uint32_t epfd, int fd)
{
    LOG_DEBUG("EPOLLDEL epfd = ", epfd, ", fd = ", fd);
    return epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
}

inline int hardwareConcurrency()
{
    return get_nprocs();
}

[[nodiscard]] inline bool hasFile(std::string_view dir)
{
    struct stat statbuf;
    explicit_bzero(&statbuf, sizeof(statbuf));

    return stat(dir.data(), &statbuf) != -1 && S_ISREG(statbuf.st_mode);
}

[[nodiscard]] inline bool hasDir(std::string_view dir)
{
    struct stat statbuf;
    explicit_bzero(&statbuf, sizeof(statbuf));

    return stat(dir.data(), &statbuf) != -1 && S_ISDIR(statbuf.st_mode);
}
