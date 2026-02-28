#pragma once

#include <sys/time.h>

#include <chrono>
#include <coroutine>
#include <set>
#include <thread>
#include <unordered_map>

#include "promise_concepts.hpp"
#include "scheduler_awaiter.hpp"
#include "shcoro/utils/logger.h"

namespace shcoro {
class TimedScheduler {
   public:
    using value_type = time_t;

    void register_coro(std::coroutine_handle<> coro, time_t duration) {
        SHCORO_LOG("timer register: ", duration);
        auto [it, suc] = coros_.insert(
            {std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) +
                 duration,
             coro});
        coro_map_[coro.address()] = it;
    }

    void unregister_coro(std::coroutine_handle<> coro) {
        auto addr = coro.address();
        if (coro_map_.find(addr) != coro_map_.end()) {
            SHCORO_LOG("timer unregister");
            coros_.erase(coro_map_[addr]);
            coro_map_.erase(addr);
        }
    }

    void run_once() {
        if (!coros_.empty()) {
            SHCORO_LOG("remaining task: ", coros_.size());
            auto it = coros_.begin();
            time_t cur =
                std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            if (it->first > cur) {
                std::this_thread::sleep_for(std::chrono::seconds(it->first - cur));
            }
            auto handle = it->second;
            SHCORO_LOG("unregister handle");
            unregister_coro(handle);
            SHCORO_LOG("resume handle");
            handle.resume();
        }
    }

    void run() {
        while (!coros_.empty()) {
            SHCORO_LOG("remaining task: ", coros_.size());
            auto it = coros_.begin();
            time_t cur =
                std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            if (it->first > cur) {
                std::this_thread::sleep_for(std::chrono::seconds(it->first - cur));
            }
            auto handle = it->second;
            SHCORO_LOG("unregister handle");
            unregister_coro(handle);
            SHCORO_LOG("resume handle");
            handle.resume();
        }
    }

   private:
    std::set<std::pair<time_t, std::coroutine_handle<>>> coros_;
    std::unordered_map<void*, decltype(coros_)::iterator> coro_map_;
};

using TimedAwaiter = SchedulerAwaiter<time_t>;

}  // namespace shcoro
