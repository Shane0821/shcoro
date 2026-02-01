#include <iostream>

#include "shcoro/stackless/timer.hpp"
#include "shcoro/stackless/utility.hpp"

using shcoro::Async;
using shcoro::spawn_async;
using shcoro::TimedAwaiter;
using shcoro::TimedScheduler;

Async<void> void_func() {
    std::cout << "void_func called\n";
    std::cout << "void_func return\n";
    co_return;
}

Async<int> inner_func(int x) {
    std::cout << "inner_func called\n";
    int ret = x * x;
    std::cout << "register to scheduler: " << x << '\n';
    co_await TimedAwaiter{x};
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
    auto ret = co_await all_of(middle_func(x), middle_func(x * 2), void_func());
    long long z = std::get<0>(ret) + std::get<1>(ret);
    std::cout << "outter_func return: " << z << '\n';
    co_return z;
}

int main() {
    TimedScheduler sched;
    auto ret1 = spawn_async(outter_func(3), sched);
    sched.run();
    std::cout << "ret1: " << ret1.get() << '\n';
    return 0;
}