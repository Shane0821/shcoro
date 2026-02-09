#pragma once

#include <coroutine>
#include <exception>
#include <optional>

#include "awaiter_base.hpp"
#include "promise_base.hpp"
#include "promise_concepts.hpp"
#include "scheduler.hpp"
#include "shcoro/utils/logger.h"
#include "shcoro/utils/noncopyable.h"

namespace shcoro {
// Async operation that can be suspended within a nested coroutine
template <typename T = void>
class [[nodiscard]] Async : noncopyable {
   public:
    struct promise_type : promise_suspend_base<std::suspend_always, ResumeCallerAwaiter>,
                          promise_return_base<T>,
                          promise_exception_base,
                          promise_scheduler_base,
                          promise_caller_base {
        promise_type() { SHCORO_LOG("async promise created: ", this); }
        ~promise_type() {
            SHCORO_LOG("async promise destroyed: ", this);
            if (scheduler_) {
                scheduler_unregister_coro(
                    scheduler_, std::coroutine_handle<promise_type>::from_promise(*this));
            }
        }
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
        SHCORO_LOG("async await resume: ", &self_.promise());
        return std::move(this->self_.promise().get_return_value());
    }

    void await_resume()
        requires(std::is_same_v<T, void>)
    {
        SHCORO_LOG("async await resume: ", &self_.promise());
    }

    void set_scheduler(Scheduler sched) noexcept { self_.promise().set_scheduler(sched); }

    Async(Async&& other) noexcept : self_(std::exchange(other.self_, {})) {}

    ~Async() {
        if (self_) {
            SHCORO_LOG("Async destroy: ", &self_.promise());
            self_.destroy();
        }
    }

   private:
    explicit Async(promise_type* promise) {
        self_ = std::coroutine_handle<promise_type>::from_promise(*promise);
        SHCORO_LOG("Async created: ", &self_.promise());
    }

    std::coroutine_handle<promise_type> self_{nullptr};
};

// A wrapper to spawn a Async task
template <typename T = void>
class [[nodiscard]] AsyncRO : noncopyable {
   public:
    struct promise_type : promise_suspend_base<std::suspend_never, std::suspend_always>,
                          promise_return_base<T>,
                          promise_exception_base

    {
        promise_type() { SHCORO_LOG("async ro promise created: ", this); }
        ~promise_type() { SHCORO_LOG("async ro promise destroyed: ", this); }
        auto get_return_object() { return AsyncRO{this}; }
    };

    AsyncRO(AsyncRO&& other) noexcept : self_(std::exchange(other.self_, {})) {}

    ~AsyncRO() {
        if (self_) {
            SHCORO_LOG("AsyncRO destroy: ", &self_.promise());
            self_.destroy();
        }
    }

    T get() {
        if constexpr (!std::is_same_v<T, void>) {
            return self_.promise().get_return_value();
        }
    }

   private:
    explicit AsyncRO(promise_type* promise) {
        self_ = std::coroutine_handle<promise_type>::from_promise(*promise);
        SHCORO_LOG("AsyncRO created: ", &self_.promise());
    }

    std::coroutine_handle<promise_type> self_{nullptr};
};

}  // namespace shcoro