#pragma once

#include "async.hpp"
#include "mux.hpp"

// spawns an async task without a scheduler
// NOTE: the inner most coro should not be suspended,
// otherwise the outter coros are never woken up
namespace shcoro {
template <typename T>
AsyncRO<T> spawn_async(Async<T> task) {
    co_return co_await task;
}

// spawns an async task with a scheduler
template <typename T, typename Scheduler>
AsyncRO<T> spawn_async(Async<T> task, Scheduler& scheduler) {
    task.set_scheduler(AsyncScheduler::from(scheduler));
    co_return co_await task;
}

template <typename T>
MuxAdapter<T> make_mux_adapter(Async<T> task, AsyncScheduler sched) {
    task.set_scheduler(sched);
    co_return co_await task;
}

template <typename... T>
Mux<std::tuple<T...>> all_of(Async<T>... tasks) {
    auto scheduler = co_await GetSchedulerAwaiter{};
    co_return co_await AllOfAwaiter(make_mux_adapter(std::move(tasks), scheduler)...);
}

}  // namespace shcoro
