#pragma once

#include <coroutine>

namespace shcoro {

template <typename PromiseType>
struct ResumeCallerAwaiter {
    constexpr bool await_ready() const noexcept { return false; }
    constexpr void await_resume() const noexcept { /* should never be called */ }
    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<PromiseType> h) const noexcept {
        return h.promise().get_caller();
    }
};

struct GetSchedulerAwaiter {
    constexpr bool await_ready() const noexcept { return false; }
    auto await_resume() const noexcept { return scheduler_; }

    template <AsyncPromiseConcept PromiseType>
    bool await_suspend(std::coroutine_handle<PromiseType> h) noexcept {
        scheduler_ = h.promise().get_scheduler();
        return false;
    }

    AsyncScheduler scheduler_;
};

}  // namespace shcoro