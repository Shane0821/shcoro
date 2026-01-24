#include <sys/time.h>

#include <chrono>
#include <iostream>
#include <set>
#include <thread>
#include <unordered_map>

#include "shcoro/stackless/utility.hpp"

using shcoro::Async;
using shcoro::spawn_task;

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

    template <shcoro::AsyncPromiseConcept CallerPromiseType>
    auto await_suspend(std::coroutine_handle<CallerPromiseType> caller) noexcept {
        caller.promise().get_scheduler().template as<TimedScheduler>()->register_task(
            caller, duration_);
    }

    void await_resume() const noexcept {}

    time_t duration_;
};

Async<int> inner_func(int x) {
    std::cout << "inner_func called\n";
    int ret = x * x;
    std::cout << "register to scheduler: " << x << '\n';
    co_await TimedAwaiter{5};
    std::cout << "woken up by scheduler: " << x << '\n';
    std::cout << "inner_func return: " << ret << '\n';
    co_return ret;
}

Async<double> middle_func(int x) {
    std::cout << "middle_func called\n";
    int y = co_await inner_func(x);
    double ret = 3.0 * y;
    std::cout << "middle_func return: " << ret << '\n';
    co_return ret;
}

Async<long long> outter_func(int x) {
    std::cout << "outter_func called\n";
    double z = co_await middle_func(x);
    std::cout << "outter_func return: " << z << '\n';
    co_return z;
}

int main() {
    TimedScheduler sched;
    auto ret1 = spawn_task(outter_func(3), sched);
    auto ret2 = spawn_task(outter_func(5), sched);
    sched.run();
    std::cout << "ret1: " << ret1.get() << '\n';
    std::cout << "ret2: " << ret2.get() << '\n';
    return 0;
}