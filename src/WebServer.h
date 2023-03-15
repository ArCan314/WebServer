#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <string_view>
#include <optional>
#include <shared_mutex>
#include <iostream>
#include <thread>
#include <deque>

#include <cinttypes>

#include <sys/timerfd.h>

#include "./util/Noncopyable.h"
#include "./util/utils.h"
#include "./HttpResponseBuilder.h"
#include "./TcpSocket.h"
#include "./ThreadPool.h"
#include "./HttpContext.h"
#include "./util/FdHolder.h"
#include "./Logger.h"
#include "./TimerQueue.h"

class WebServer : NonCopyable
{
public:
    WebServer() = default;
    WebServer &setLogPath(std::string path)
    {
        log_path_ = std::move(path);
        return *this;
    }

    WebServer &setRootPath(std::string path)
    {
        root_path_ = std::move(path);
        return *this;
    }

    WebServer &setLogLevel(LogLevel level)
    {
        log_level_ = level;
        return *this;
    }

    WebServer &addListenAddress(const std::string &ip, uint16_t port, int count = 1)
    {
        for (int i = 0; i < count; i++)
            listen_addresses_.emplace_back(ip, port);
        return *this;
    }

    WebServer &setWorkerThreadNum(int size)
    {
        worker_size_ = size;
        return *this;
    }

    WebServer &setWorkerPoolSize(int size)
    {
        worker_pool_size_ = size;
        return *this;
    }

    int getTotalThreadNum() const noexcept
    {
        return listen_addresses_.size() + worker_size_ * worker_pool_size_ + worker_size_;
    }

    bool start();

private:
    std::vector<std::thread> acceptors_;

    ThreadPool workers_;
    int worker_size_{3};
    int worker_pool_size_{4};

    std::vector<FdHolder> worker_epfds_; 

    std::string log_path_{"./server.log"};
    LogLevel log_level_{LogLevel::WARNING};

    std::string root_path_{"./root"};

    std::vector<std::pair<std::string, uint16_t>> listen_addresses_;

    void acceptorLoop(std::string_view ip, uint16_t port);
    void workerLoop(int epfd);
};