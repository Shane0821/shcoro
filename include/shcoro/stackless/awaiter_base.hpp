#pragma once

#include <coroutine>
#include <tuple>

#include "promise_concepts.hpp"

namespace shcoro {

struct ResumeCallerAwaiter {
    constexpr bool await_ready() const noexcept { return false; }
    constexpr void await_resume() const noexcept { /* should never be called */ }
    template <typename PromiseType>
    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<PromiseType> h) const noexcept {
        return h.promise().get_caller();
    }
};

struct GetSchedulerAwaiter {
    constexpr bool await_ready() const noexcept { return false; }
    auto await_resume() const noexcept { return scheduler_; }

    template <PromiseSchedulerConcept PromiseType>
    bool await_suspend(std::coroutine_handle<PromiseType> h) noexcept {
        scheduler_ = h.promise().get_scheduler();
        return false;
    }

    AsyncScheduler scheduler_;
};

struct GetCoroAwaiter {
    constexpr bool await_ready() const noexcept { return false; }
    auto await_resume() const noexcept { return coro_; }

    bool await_suspend(std::coroutine_handle<> caller) noexcept {
        coro_ = caller;
        return false;
    }

    std::coroutine_handle<> coro_;
};

}  // namespace shcoro