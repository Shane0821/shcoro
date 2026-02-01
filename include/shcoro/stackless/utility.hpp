#pragma once

#include "async.hpp"
#include "mux.hpp"
#include "mux_awaiter.hpp"

// spawns an async task without a scheduler
// NOTE: the inner most coro should not be suspended,
// otherwise the outter coros are never woken up
namespace shcoro {
template <typename T>
AsyncRO<T> spawn_async(Async<T> task) {
    co_return co_await task;
}

// spawns an async task with a scheduler
template <typename T, typename SchedulerT>
AsyncRO<T> spawn_async(Async<T> task, SchedulerT& sched) {
    task.set_scheduler(Scheduler::from(sched));
    co_return co_await task;
}

template <typename T>
MuxAdapter<T> make_mux_adapter(Async<T> task, Scheduler& sched) {
    task.set_scheduler(sched);
    co_return co_await task;
}

template <typename... T>
Mux<all_of_return_t<T...>> all_of(Async<T>... tasks) {
    auto scheduler = co_await GetSchedulerAwaiter{};
    co_return co_await AllOfAwaiter(make_mux_adapter(std::move(tasks), scheduler)...);
}

template <typename... T>
Mux<any_of_return_t<T...>> any_of(Async<T>... tasks) {
    auto scheduler = co_await GetSchedulerAwaiter{};
    co_return co_await AnyOfAwaiter(make_mux_adapter(std::move(tasks), scheduler)...);
}

}  // namespace shcoro
