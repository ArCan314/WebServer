#include <array>
#include <memory>
#include <functional>
#include <shared_mutex>

#include <cassert>
#include <cinttypes>

#include <sys/timerfd.h>
#include <sys/epoll.h>

#include "./WebServer.h"
#include "./TcpSocket.h"
#include "./ThreadPool.h"
#include "./HttpContext.h"
#include "./util/utils.h"
#include "./util/FdHolder.h"
#include "./util/Noncopyable.h"
#include "./Logger.h"

static timespec getTimeSpec(long ms)
{
    timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1'000'000;
    return ts;
}

static int setTimerFd(int fd, long loop_ms)
{
    itimerspec timerspec;
    explicit_bzero(&timerspec, sizeof(timerspec));

    timerspec.it_interval = getTimeSpec(loop_ms);
    timerspec.it_value = getTimeSpec(loop_ms);
    return timerfd_settime(fd, 0, &timerspec, nullptr);
}

static void acceptorEventLoop(int epfd, const std::vector<FdHolder> &worker_epfds, const TcpSocket &listen_socket)
{
    static constexpr int kMaxEventArrSize = 1;
    std::array<epoll_event, kMaxEventArrSize> events;
    int worker_epfd_ind = 0;

    while (true)
    {
        int event_count = epoll_wait(epfd, events.data(), events.size(), -1);
        while (event_count == -1 && errno == EINTR)
            event_count = epoll_wait(epfd, events.data(), events.size(), -1);

        if (event_count == -1)
        {
            LOG_ERROR("epoll_wait fails, reason: ", logErrStr(errno));
            return;
        }

        if (event_count == 0)
            continue;

        const auto &event = events[0];
        while (true) // loop until no connection can be accepted
        {
            int client_fd = listen_socket.accept();
            if (client_fd == -1)
                break;

            if (epollAddOneShot(worker_epfds[worker_epfd_ind++].fd(), EPOLLIN | EPOLLRDHUP /*|EPOLLET*/, client_fd) == -1)
            {
                LOG_ERROR("Failed to add oneshot epoll event EPOLLIN|EPOLLRDHUP for socket(fd=", client_fd, "), reason: ", logErrStr(errno));
                return;
            }
            worker_epfd_ind %= worker_epfds.size();
        }
    }
}

void WebServer::acceptorLoop(std::string_view ip, uint16_t port)
{
    LOG_INFO("Acceptor thread start on (", ip, ", ", port, ")");

    auto listen_socket = TcpSocket();
    LOGIF_BERROR(listen_socket.setReuseAddr(true), "Failed to set reuse addr option for fd = ", listen_socket.fd());
    LOGIF_BERROR(listen_socket.setReusePort(true), "Failed to set reuse port option for fd = ", listen_socket.fd());
    LOGIF_BERROR(listen_socket.setNonBlocking(true), "Failed to set nonblocking option for fd = ", listen_socket.fd());

    if (listen_socket.bind(ip, port) == -1)
    {
        LOG_ERROR("Failed to bind(", ip, ",", port, ")");
        return;
    }

    if (listen_socket.listen() == -1)
    {
        LOG_ERROR("Failed to listen.");
        return;
    }

    const int epfd = epoll_create(1);
    if (epfd == -1)
    {
        LOG_ERROR("Failed to create epoll fd, reason: ", logErrStr(errno));
    }

    if (epollAdd(epfd, EPOLLIN | EPOLLET, listen_socket.fd()) == -1)
    {
        LOG_ERROR("Failed to add epoll event EPOLLIN on socket(fd=", listen_socket.fd(), ")");
        return;
    }

    const FdHolder epfd_guard(epfd);
    acceptorEventLoop(epfd, worker_epfds_, listen_socket);
}

void WebServer::workerLoop(int epfd, int pool_size)
{
    const int timerfd = timerfd_create(CLOCK_REALTIME, 0);
    if (timerfd == -1)
    {
        LOG_ERROR("Failed to create timerfd, reason: ", logErrStr(errno));
        return;
    }
    const FdHolder timerfd_guard(timerfd);

    static constexpr int kMaxEventArrSize = 10000;
    static constexpr int kConnectionTimeOutMs = 5000;
    std::array<epoll_event, kMaxEventArrSize> events;
    std::vector<std::unique_ptr<HttpContext>> contexts;
    std::vector<int> contexts_is_valid;
    std::mutex contexts_mtx;
    TimerQueue timers;
    ThreadPool pool;
    pool.start(pool_size);

    const auto eraseContext = [&contexts, &contexts_mtx, &contexts_is_valid, &timers](int fd)
    {
        const std::lock_guard lock(contexts_mtx);
        LOG_DEBUG("EraseContext called, fd = ", fd);
        if (fd < contexts.size() && contexts_is_valid[fd])
        {
            LOG_DEBUG("Remove contexts[", fd, "], ptr = ", long(contexts[fd].get()), ", contexts.size() = ", contexts.size());
            timers.removeTimer(contexts[fd]->getTimerId());
            contexts_is_valid[fd] = false;
            contexts[fd]->resetContext();
            return true;
        }
        return false;
    };

    const auto setContext = [this, &contexts, &contexts_mtx, &contexts_is_valid, epfd, &eraseContext](std::unique_ptr<TcpSocket> connection, TimerQueue::TimerId timer_id)
    {
        const auto fd = connection->fd();

        // auto context = std::make_unique<HttpContext>(
        //     std::move(connection),
        //     epfd,
        //     [&eraseContext](int fd)
        //     { eraseContext(fd); },
        //     this->root_dir_,
        //     timer_id);

        {
            const std::lock_guard lock(contexts_mtx);
            if (fd >= contexts.size())
            {
                contexts.resize(fd + 1);
                contexts_is_valid.resize(fd + 1);
            }

            if (contexts[fd])
                contexts[fd]->setContext(
                    std::move(connection),
                    epfd,
                    [&eraseContext](int fd)
                    { eraseContext(fd); },
                    this->root_dir_,
                    timer_id);
            else
                contexts[fd] = std::make_unique<HttpContext>(
                    std::move(connection),
                    epfd,
                    [&eraseContext](int fd)
                    { eraseContext(fd); },
                    this->root_dir_,
                    timer_id);
            contexts_is_valid[fd] = true;
            
            LOG_DEBUG("Set contexts[", fd, "], ptr = ", long(contexts[fd].get()), ", contexts.size() = ", contexts.size());
            return contexts[fd].get();
        }
    };

    const auto getContext = [&contexts, &contexts_is_valid, &contexts_mtx](int fd) -> HttpContext *
    {
        const std::lock_guard lock(contexts_mtx);
        if (fd >= contexts.size())
        {
            contexts.resize(fd + 1);
            contexts_is_valid.resize(fd + 1);
        }
        return contexts_is_valid[fd] ? contexts[fd].get() : nullptr;
    };

    if (epollAdd(epfd, EPOLLIN, timerfd) == -1)
    {
        LOG_ERROR("Failed to add epoll event EPOLLIN on timer_fd(fd=", timerfd, ")");
        return;
    }

    static constexpr int kTimerExpirationInterval = 2000;
    if (setTimerFd(timerfd, kTimerExpirationInterval) == -1)
    {
        LOG_ERROR("Failed to set timer interval = ", kTimerExpirationInterval);
        return;
    }

    while (true)
    {
        int event_count = epoll_wait(epfd, events.data(), events.size(), -1);
        while (event_count == -1 && errno == EINTR)
            event_count = epoll_wait(epfd, events.data(), events.size(), -1);

        if (event_count == -1)
        {
            LOG_ERROR("epoll_wait fails, reason: ", logErrStr(errno));
            return;
        }

        bool has_timer_event = false;
        for (int i = 0; i < event_count; i++)
        {
            const auto &event = events[i];
            if (event.events & EPOLLRDHUP || event.events & EPOLLHUP)
            {
                LOG_INFO("Event EPOLLRDHUP or EPOLLHUP raised on fd ", event.data.fd);
                if (eraseContext(event.data.fd))
                    epollDel(epfd, event.data.fd);
            }
            else if (event.data.fd == timerfd)
            {
                has_timer_event = true;
            }
            else if (event.events & EPOLLIN)
            {
                LOG_DEBUG("EPOLLIN epfd = ", epfd, ", fd = ", event.data.fd);
                const int fd = event.data.fd;
                auto context = getContext(fd);
                LOG_DEBUG("Context ptr = ", context ? (long)context : 0l);
                if (context == nullptr)
                {
                    LOG_DEBUG("Create context on fd = ", fd);
                    context = setContext(
                        std::make_unique<TcpSocket>(fd),
                        timers.addTimer([fd, &eraseContext]()
                                        { eraseContext(fd); },
                                        kConnectionTimeOutMs));
                }

                timers.resetTimer(context->getTimerId(), kConnectionTimeOutMs);
                pool.run([context]()
                         { context->doRead(); });
            }
            else if (event.events & EPOLLOUT)
            {
                LOG_DEBUG("EPOLLOUT epfd = ", epfd, ", fd = ", event.data.fd);
                auto context = getContext(event.data.fd);
                if (context)
                {
                    timers.resetTimer(context->getTimerId(), kConnectionTimeOutMs);
                    pool.run([context]()
                             { context->doWrite(); });
                }
            }
        }

        if (has_timer_event)
        {
            // LOG_DEBUG("Timer expired");
            uint64_t buffer;
            LOGIF_PWARNING(read(timerfd, &buffer, sizeof(buffer)), "Failed to read timerfd buffer, reason: ", logErrStr(errno));
            timers.tick();
        }
    }

    pool.stop();
}

bool WebServer::start(std::string_view ip, uint16_t port, int acceptor_count, int worker_count, int worker_pool_size)
{
    LOG_INFO("Start WebServer on (", ip, ", ", port, ")");
    if (!hasDir(root_dir_))
    {
        LOG_ERROR("Cannot find root dir(", root_dir_, ") or root dir is not a directory");
        return false;
    }

    workers_.start(worker_count);
    for (int i = 0; i < worker_count; i++)
    {
        const int worker_epfd = epoll_create(1);
        if (worker_epfd == -1)
        {
            LOG_ERROR("Failed to create epoll fd, reason: ", logErrStr(errno));
            return false;
        }
        worker_epfds_.emplace_back(worker_epfd);
        workers_.run([this, worker_epfd, worker_pool_size]()
                     { this->workerLoop(worker_epfd, worker_pool_size); });
    }

    for (int i = 0; i < acceptor_count; i++)
        acceptors_.emplace_back(&WebServer::acceptorLoop, this, ip, port);

    for (auto &thread : acceptors_)
        thread.join();

    workers_.stop();
    return true;
}

bool WebServer::start(uint16_t port)
{
    return start("0.0.0.0", port);
}