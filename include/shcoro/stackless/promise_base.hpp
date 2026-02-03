#pragma once

#include <coroutine>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "scheduler.hpp"

namespace shcoro {

// exception handling
struct promise_exception_base {
    void unhandled_exception() { std::terminate(); }
};

// scheduler support
struct promise_scheduler_base {
    void set_scheduler(Scheduler other) noexcept { scheduler_ = other; }
    Scheduler& get_scheduler() noexcept { return scheduler_; }

   protected:
    Scheduler scheduler_;
};

struct promise_caller_base {
    void set_caller(std::coroutine_handle<> handle) noexcept { caller_ = handle; }
    std::coroutine_handle<> get_caller() noexcept { return caller_; }

   protected:
    std::coroutine_handle<> caller_;
};

struct promise_callee_base {
    void set_callee(std::coroutine_handle<> handle) noexcept { callee_ = handle; }
    std::coroutine_handle<> get_callee() noexcept { return callee_; }

   protected:
    std::coroutine_handle<> callee_;
};

struct promise_child_base {
    void add_child(std::coroutine_handle<> handle) { children_.push_back(handle); }

   protected:
    std::vector<std::coroutine_handle<>> children_;
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
    void return_void() const noexcept {}
};

}  // namespace shcoro
