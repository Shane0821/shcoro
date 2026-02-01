#pragma once

#include <sys/time.h>

#include <chrono>
#include <coroutine>
#include <set>
#include <thread>
#include <unordered_map>

#include "promise_concepts.hpp"

namespace shcoro {
class TimedScheduler {
   public:
    void register_task(std::coroutine_handle<> coro, time_t duration) {
        coros_.insert(
            {std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) +
                 duration,
             coro});
    }

    void run() {
        while (!coros_.empty()) {
            auto it = coros_.begin();
            if (it == coros_.end()) continue;
            time_t cur =
                std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            if (it->first > cur) {
                std::this_thread::sleep_for(std::chrono::seconds(it->first - cur));
            }
            it->second.resume();
            coros_.erase(it);
        }
    }

   private:
    std::set<std::pair<time_t, std::coroutine_handle<>>> coros_;
};

struct TimedAwaiter {
    TimedAwaiter(time_t duration = 0) { duration_ = duration; }
    constexpr bool await_ready() const noexcept { return false; }

    template <shcoro::PromiseSchedulerConcept CallerPromiseType>
    auto await_suspend(std::coroutine_handle<CallerPromiseType> caller) noexcept {
        caller.promise().get_scheduler().template as<TimedScheduler>()->register_task(
            caller, duration_);
    }

    void await_resume() const noexcept {}

    time_t duration_;
};
}  // namespace shcoro
