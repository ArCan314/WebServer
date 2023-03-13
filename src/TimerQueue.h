#pragma once

#include <set>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>

#include "./util/Noncopyable.h"

// OneShot timers
class TimerQueue : NonCopyable
{
public:
    using TimerId = unsigned;
    TimerQueue() = default;

    TimerId addTimer(std::function<void()> &&callback, int expire_ms)
    {
        const TimerId id = getId();
        const auto now = std::chrono::system_clock::now();
        const auto expire_time = now + std::chrono::milliseconds(expire_ms);

        {
            const std::lock_guard lock(mutex_);
            auto [iter, _] = timers_.emplace(expire_time, id);
            indices_.emplace(id, iter);
            callbacks_.emplace(id, std::move(callback));
        }

        return id;
    }

    void resetTimer(TimerId timer_id, int expire_ms)
    {
        const auto now = std::chrono::system_clock::now();
        const auto expire_time = now + std::chrono::milliseconds(expire_ms);

        {
            const std::lock_guard lock(mutex_);
            auto ind_iter = indices_.find(timer_id);
            if (ind_iter == indices_.end())
                return;

            auto timer_iter = ind_iter->second;
            auto hint = timers_.erase(timer_iter);
            timer_iter = timers_.emplace_hint(hint, expire_time, timer_id); // O(1) hint add
            ind_iter->second = timer_iter;
        }
    }

    void removeTimer(TimerId id)
    {
        const std::lock_guard lock(mutex_);

        auto iter = indices_.find(id);
        if (iter == indices_.end())
            return;

        timers_.erase(iter->second);
        callbacks_.erase(iter->first);
        indices_.erase(iter);
    }

    long tick() // returns next expire ms from now, returns -1 if no timer
    {
        const auto now = std::chrono::system_clock::now();
        long res = -1;
        std::vector<std::function<void()>> callbacks;

        {
            const std::lock_guard lock(mutex_);
            auto iter = timers_.begin();
            while (iter != timers_.end() && iter->first <= now)
            {
                auto callback_iter = callbacks_.find(iter->second);
                if (callback_iter != callbacks_.end())
                {
                    callbacks.push_back(std::move(callback_iter->second));
                    callbacks_.erase(callback_iter);
                }

                indices_.erase(iter->second);
                iter++;
            }
            if (iter != timers_.end())
                res = std::chrono::duration_cast<std::chrono::milliseconds>(iter->first - now).count();

            timers_.erase(timers_.begin(), iter);
        }

        for (const auto &callback : callbacks)
            callback();

        return res;
    }

private:
    std::set<std::pair<std::chrono::time_point<std::chrono::system_clock>, TimerId>> timers_; // <expire time, timer_id>

    using TimerIter = decltype(timers_)::iterator;
    std::unordered_map<TimerId, TimerIter> indices_;
    std::unordered_map<TimerId, std::function<void()>> callbacks_;

    std::mutex mutex_;
    static TimerId getId() noexcept
    {
        static std::atomic<TimerId> id{0};
        if constexpr (decltype(id)::is_always_lock_free)
            return id++;
        else
        {
            static std::mutex mtx;
            TimerId res;
            {
                const std::lock_guard lock(mtx);
                res = id++;
            }
            return res;
        }
    }
};