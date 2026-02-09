#pragma once

#include <concepts>

#include "scheduler.hpp"

namespace shcoro {

template <typename Awaitable>
concept ContinuationAwaiterConcept = requires(Awaitable a, Scheduler sched) {
    typename Awaitable::promise_type;

    requires std::derived_from<
        typename Awaitable::promise_type,
        promise_suspend_base<std::suspend_always, ResumeCallerAwaiter>>;
    a.set_scheduler(sched);

    requires std::derived_from<typename Awaitable::promise_type, promise_caller_base>;

    a.set_scheduler(sched);

    { a.await_ready() } -> std::same_as<bool>;
    requires requires(std::coroutine_handle<> h) {
        {
            a.await_suspend(h)
        } -> std::same_as<std::coroutine_handle<typename Awaitable::promise_type>>;
    };
    { a.await_resume() } -> std::same_as<typename Awaitable::promise_type::return_type>;
};

template <ContinuationAwaiterConcept T>
using awaiter_return_t = typename T::promise_type::return_type;

}  // namespace shcoro