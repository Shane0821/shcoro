#pragma once

#include <coroutine>
#include <stdexcept>

#include "scheduler.hpp"

namespace shcoro {

struct promise_common_base {
    auto initial_suspend() noexcept { return std::suspend_always{}; }
    auto final_suspend() noexcept { return std::suspend_always{}; }

    void unhandled_exception() { exception_ = std::current_exception(); }
    void rethrow_if_exception() {
        if (exception_) [[unlikely]] {
            std::rethrow_exception(exception_);
        }
    }

    void set_scheduler(AsyncScheduler other) noexcept { scheduler_ = other; }
    AsyncScheduler get_scheduler() const noexcept { return scheduler_; }

   protected:
    std::exception_ptr exception_{nullptr};
    AsyncScheduler scheduler_;
};

template <typename T>
struct async_promise_base : promise_common_base {
    template <typename U>
    void return_value(U&& val) {
        value_ = std::forward<U>(val);
    }

    T get_return_value() { return std::move(value_); }

   protected:
    T value_{};
};

template <>
struct async_promise_base<void> : promise_common_base {
    void return_void() {}
};

}  // namespace shcoro
