#pragma once

#include "async.hpp"

namespace shcoro {
template <typename T>
AsyncRO<T> spawn_task(Async<T> task) {
    co_return co_await task;
}

template <typename T, typename Scheduler>
AsyncRO<T> spawn_task(Async<T> task, Scheduler& scheduler) {
    task.set_scheduler(AsyncScheduler::from(scheduler));
    co_return co_await task;
}
}  // namespace shcoro
