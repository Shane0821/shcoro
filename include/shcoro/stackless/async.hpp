#pragma once

#include <coroutine>
#include <exception>
#include <optional>

#include "promise_concepts.hpp"
#include "awaiters.hpp"
#include "noncopyable.h"
#include "promise_base.hpp"
#include "scheduler.hpp"

namespace shcoro {
// Async operation that can be suspended within a nested coroutine
template <typename T = void>
class [[nodiscard]] Async : noncopyable {
   public:
    struct promise_type
        : promise_suspend_base<std::suspend_always, ResumeCallerAwaiter<promise_type>>,
          promise_return_base<T>,
          promise_exception_base,
          promise_scheduler_base,
          promise_caller_base {
        auto get_return_object() { return Async{this}; }
    };

    constexpr bool await_ready() const noexcept { return false; }

    template <typename CallerPromiseType>
    auto await_suspend(std::coroutine_handle<CallerPromiseType> caller) noexcept {
        self_.promise().set_caller(caller);
        if constexpr (PromiseSchedulerConcept<CallerPromiseType>) {
            self_.promise().set_scheduler(caller.promise().get_scheduler());
        }
        return self_;
    }

    auto await_resume()
        requires(!std::is_same_v<T, void>)
    {
        this->self_.promise().rethrow_if_exception();
        return std::move(this->self_.promise().get_return_value());
    }

    void await_resume()
        requires(std::is_same_v<T, void>)
    {
        this->self_.promise().rethrow_if_exception();
    }

    void set_scheduler(AsyncScheduler sched) noexcept {
        self_.promise().set_scheduler(sched);
    }

    Async(Async&& other) noexcept : self_(std::exchange(other.self_, nullptr)) {}
    Async& operator=(Async&& other) noexcept {
        self_ = std::exchange(other.self_, nullptr);
        return *this;
    }

    ~Async() {
        if (self_) {
            self_.destroy();
        }
    }

   private:
    explicit Async(promise_type* promise) {
        self_ = std::coroutine_handle<promise_type>::from_promise(*promise);
    }

    std::coroutine_handle<promise_type> self_{nullptr};
};

// A wrapper to spawn a Async task
template <typename T = void>
class AsyncRO : noncopyable {
   public:
    struct promise_type : promise_suspend_base<std::suspend_never, std::suspend_always>,
                          promise_return_base<T>,
                          promise_exception_base

    {
        auto get_return_object() { return AsyncRO{this}; }
    };

    ~AsyncRO() {
        if (self_) {
            self_.destroy();
        }
    }

    T get() {
        self_.promise().rethrow_if_exception();
        if constexpr (!std::is_same_v<T, void>) {
            return self_.promise().get_return_value();
        }
    }

   private:
    explicit AsyncRO(promise_type* promise) {
        self_ = std::coroutine_handle<promise_type>::from_promise(*promise);
    }

    std::coroutine_handle<promise_type> self_{nullptr};
};

}  // namespace shcoro