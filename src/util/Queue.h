#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <atomic>
#include <type_traits>

template <typename T>
class Queue
{
private:
    std::deque<T> queue_;
    std::condition_variable cond_;
    std::mutex mutex_;
    std::atomic_bool is_running_{true};

    void __enqueue(const T &elem)
    {
        std::lock_guard lock(mutex_);
        queue_.emplace_back(elem);
        cond_.notify_one();
    }

    void __enqueue(T &&elem)
    {
        const std::lock_guard lock(mutex_);
        queue_.emplace_back(elem);
        cond_.notify_one();
    }

public:
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
    void enqueue(U &&elem)
    {
        // static_assert(std::is_convertible_v<U, T>);
        if (is_running_)
            __enqueue(std::forward<T>(elem));
    }

    std::optional<T> dequeue()
    {
        std::unique_lock lock(mutex_);
        while (queue_.empty() || !is_running_)
            cond_.wait(lock, [&]() -> bool
                       { return !queue_.empty() || !is_running_; });

        if (!is_running_)
            return std::nullopt;

        T res(std::move(queue_.front()));
        queue_.pop_front();
        return res;
    }

    void stop()
    {
        const std::lock_guard lock(mutex_);
        is_running_ = false;
        cond_.notify_all();
    }

    auto size() const noexcept
    {
        return queue_.size();
    }
};