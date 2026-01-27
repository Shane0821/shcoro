#pragma once

#include <coroutine>
#include <stdexcept>
#include <tuple>

#include "scheduler.hpp"

namespace shcoro {

// exception handling
struct promise_exception_base {
    void unhandled_exception() { exception_ = std::current_exception(); }
    void rethrow_if_exception() {
        if (exception_) [[unlikely]] {
            std::rethrow_exception(exception_);
        }
    }

   protected:
    std::exception_ptr exception_{nullptr};
};

// scheduler support
struct promise_scheduler_base {
    void set_scheduler(AsyncScheduler other) noexcept { scheduler_ = other; }
    AsyncScheduler get_scheduler() const noexcept { return scheduler_; }

   protected:
    AsyncScheduler scheduler_;
};

struct promise_caller_base {
    void set_caller(std::coroutine_handle<> handle) noexcept { caller_ = handle; }
    std::coroutine_handle<> get_caller() noexcept { return caller_; }

   protected:
    std::coroutine_handle<> caller_;
};

// suspension behaviour control
template <typename InitialSuspend, typename FinalSuspend>
struct promise_suspend_base {
    InitialSuspend initial_suspend() noexcept { return {}; }
    FinalSuspend final_suspend() noexcept { return {}; }
};

// return value handling
template <typename T>
struct promise_return_base {
    template <typename U>
    void return_value(U&& val) {
        value_ = std::forward<U>(val);
    }

    auto get_return_value() { return std::move(value_); }

   protected:
    T value_{};
};

template <>
struct promise_return_base<void> {
    void return_void() {}
};

}  // namespace shcoro
