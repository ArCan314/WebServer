#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <functional>

#include <unistd.h>

#include "./util/Noncopyable.h"
#include "./util/Queue.h"
#include "./Logger.h"

class ThreadPool : NonCopyable
{
public:
    using Task = std::function<void()>;
private:
    const int kMaxThreadNum = 16;
    std::vector<std::thread> threads_;
    Queue<Task> queue_;
    std::mutex run_mutex_;
    bool is_running_{false};

    [[nodiscard]] auto size() const noexcept { return threads_.size(); }
    [[nodiscard]] bool full() const noexcept { return size() == kMaxThreadNum; }
    [[nodiscard]] bool empty() const noexcept { return size() == 0; }
public:
    ThreadPool() = default;
    explicit ThreadPool(int max_thread_num) : kMaxThreadNum(max_thread_num) {}

    void start(int thread_num)
    {
        is_running_ = true;
        LOG_DEBUG("ThreadPool start with ", thread_num, " threads");
        for (int i = 0; i < thread_num; i++)
            if (threads_.size() == kMaxThreadNum)
                break;
            else
                threads_.emplace_back([this](){ this->work(); });
    }

    void run(Task task)
    {
        if (is_running_)
        {
            if (empty())
                task();
            else
                queue_.enqueue(task);
        }
    }

    void stop()
    {
        is_running_ = false;
        queue_.stop();

        for (int i = 0; i < size(); i++)
            threads_[i].join();
    }

private:
    void work()
    {
        while (is_running_)
        {
            auto task = queue_.dequeue();
            if (task.has_value())
                task.value()();
        }
    }
};