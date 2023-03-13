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
    explicit WebServer(
        std::string root_dir = "./root",
        LogLevel log_level = LogLevel::WARNING,
        int max_thread_num = hardwareConcurrency()) : root_dir_(std::move(root_dir))
    {
        if (!Logger::instance().init("./server.log", log_level))
            std::cerr << "Failed to initialize logger with params: "
                      << "./server.log, LogLevel::DEBUG" << std::endl;
    }
    bool start(std::string_view ip, uint16_t port, int acceptor_count = 1, int worker_count = 2, int worker_pool_size = 4);
    bool start(uint16_t port);

private:
    std::vector<std::thread> acceptors_;

    ThreadPool workers_;
    std::vector<FdHolder> worker_epfds_; 
    const std::string root_dir_;

    void acceptorLoop(std::string_view ip, uint16_t port);
    void workerLoop(int epfd, int pool_size);
};