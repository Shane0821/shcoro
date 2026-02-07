#include <iostream>

#include "shcoro/stackless/rw_lock.hpp"
#include "shcoro/stackless/timer.hpp"
#include "shcoro/stackless/utility.hpp"

using shcoro::Async;
using shcoro::RWLock;
using shcoro::RWLockPolicy;
using shcoro::spawn_async;
using shcoro::TimedAwaiter;
using shcoro::TimedScheduler;

RWLock<RWLockPolicy::FAIR> lock;

Async<void> reader(int x) {
    std::cout << "reader called: " << x << '\n';
    co_await lock.read_lock();
    std::cout << "reader acquired lock: " << x << '\n';
    co_await TimedAwaiter{x};
    std::cout << "reader resumed and unlock: " << x << '\n';
    lock.read_unlock();
    std::cout << "reader return: " << x << '\n';
}

Async<void> writer(int x) {
    std::cout << "writer called: " << x << '\n';
    co_await lock.write_lock();
    std::cout << "writer acquired lock: " << x << '\n';
    co_await TimedAwaiter{x};
    std::cout << "writer resumed and unlock: " << x << '\n';
    lock.write_unlock();
    std::cout << "writer return: " << x << '\n';
}

Async<void> main_func() {
    std::cout << "main_func called\n";
    co_await all_of(reader(1), reader(2), writer(3), reader(4), reader(5), writer(6),
                    reader(7), reader(8));
    std::cout << "main_func return\n";
}

int main() {
    TimedScheduler sched;
    auto ret = spawn_async(main_func(), sched);
    sched.run();
    return 0;
}