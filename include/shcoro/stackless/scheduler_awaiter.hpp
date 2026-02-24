#pragma once

#include <coroutine>

#include "promise_concepts.hpp"

namespace shcoro {

template <typename ValueT>
struct SchedulerAwaiter {
    SchedulerAwaiter(ValueT&& value) : value_(std::move(value)) {}
    SchedulerAwaiter(const ValueT& value) : value_(value) {}

    constexpr bool await_ready() const noexcept { return false; }

    template <shcoro::PromiseSchedulerConcept CallerPromiseType>
    auto await_suspend(std::coroutine_handle<CallerPromiseType> caller) noexcept {
        scheduler_register_coro(caller.promise().get_scheduler(), caller, std::move(value_));
    }

    void await_resume() const noexcept {}

    ValueT value_;
};

template <>
struct SchedulerAwaiter<void> {
    SchedulerAwaiter() = default;
    constexpr bool await_ready() const noexcept { return false; }
    template <shcoro::PromiseSchedulerConcept CallerPromiseType>
    auto await_suspend(std::coroutine_handle<CallerPromiseType> caller) noexcept {
        scheduler_register_coro(caller.promise().get_scheduler(), caller);
    }
    void await_resume() const noexcept {}
};

}