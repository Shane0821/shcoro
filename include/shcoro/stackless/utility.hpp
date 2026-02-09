#pragma once

#include "async.hpp"
#include "awaiter_concepts.hpp"
#include "mux.hpp"
#include "mux_awaiter.hpp"

// spawns an async task without a scheduler
// NOTE: the inner most coro should not be suspended,
// otherwise the outter coros are never woken up
namespace shcoro {
template <ContinuationAwaiterConcept T>
AsyncRO<awaiter_return_t<T>> spawn_async(T task) {
    co_return co_await task;
}

// spawns an async task with a scheduler
template <ContinuationAwaiterConcept T, typename SchedulerT>
AsyncRO<awaiter_return_t<T>> spawn_async(T task, SchedulerT& sched) {
    task.set_scheduler(sched);
    co_return co_await task;
}

template <ContinuationAwaiterConcept T>
MuxAdapter<awaiter_return_t<T>> make_mux_adapter(T task, Scheduler& sched) {
    task.set_scheduler(sched);
    co_return co_await task;
}

template <ContinuationAwaiterConcept... T>
Mux<all_of_return_t<awaiter_return_t<T>...>> all_of(T... tasks) {
    auto scheduler = co_await GetSchedulerAwaiter{};
    co_return co_await AllOfAwaiter(make_mux_adapter(std::move(tasks), scheduler)...);
}

template <ContinuationAwaiterConcept... T>
Mux<any_of_return_t<awaiter_return_t<T>...>> any_of(T... tasks) {
    auto scheduler = co_await GetSchedulerAwaiter{};
    co_return co_await AnyOfAwaiter(make_mux_adapter(std::move(tasks), scheduler)...);
}

}  // namespace shcoro
