#pragma once

#include <coroutine>

template <typename PromiseType>
struct ResumeCallerAwaiter {
    constexpr bool await_ready() const noexcept { return false; }
    constexpr void await_resume() const noexcept { /* should never be called */ }
    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<PromiseType> h) const noexcept {
        return h.promise().get_caller();
    }
};