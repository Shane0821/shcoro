#include <iostream>

#include "shcoro/stackless/timer.hpp"
#include "shcoro/stackless/utility.hpp"

using shcoro::Async;
using shcoro::spawn_async;
using shcoro::TimedAwaiter;
using shcoro::TimedScheduler;

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
    auto ret1 = spawn_async(outter_func(3), sched);
    auto ret2 = spawn_async(outter_func(5), sched);
    sched.run();
    std::cout << "ret1: " << ret1.get() << '\n';
    std::cout << "ret2: " << ret2.get() << '\n';
    return 0;
}