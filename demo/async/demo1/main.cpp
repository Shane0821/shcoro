#include <iostream>

#include "shcoro/stackless/utility.hpp"

using shcoro::Async;
using shcoro::spawn_task;

Async<int> inner_func(int x) {
    std::cout << "inner_func called\n";
    int ret = x * x;
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
    auto ret = spawn_task(outter_func(3));
    std::cout << "ret: " << ret.get() << '\n';
    return 0;
}