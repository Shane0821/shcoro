#pragma once

#include <concepts>
#include <coroutine>
#include <exception>
#include <iostream>
#include <optional>
#include <type_traits>

#include "scheduler.h"

namespace shcoro {
// Async promise should store a scheduler
template <typename T>
concept AsyncPromiseConcept = requires(T t, Scheduler* sched) {
    // Must have getter
    { t.get_scheduler() } -> std::same_as<Scheduler*>;

    // Must have setter
    t.set_scheduler(sched);
};

template <typename T>
struct async_promise;

template <typename T>
class AsyncBase {
   public:
    using promise_type = async_promise<T>;

    explicit AsyncBase(promise_type* promise) {
        self_ = std::coroutine_handle<promise_type>::from_promise(*promise);
    }

    AsyncBase(const AsyncBase&) = delete;
    AsyncBase& operator=(const AsyncBase&) = delete;

    AsyncBase(AsyncBase&& other) noexcept : self_(other.self_) { other.self_ = nullptr; }
    AsyncBase& operator=(AsyncBase&& other) noexcept {
        self_ = other.self_;
        other.self_ = nullptr;
        return *this;
    }

    ~AsyncBase() {
        if (self_) {
            self_.destroy();
        }
    }

    constexpr bool await_ready() const noexcept { return false; }

    template <typename PromiseType>
    auto await_suspend(std::coroutine_handle<PromiseType> caller) noexcept {
        self_.promise().set_caller(caller);
        if constexpr (AsyncPromiseConcept<PromiseType>) {
            self_.promise().set_scheduler(caller.promise().get_scheduler());
        }
        std::cout << "switch to child\n";
        return self_;
    }

    void set_scheduler(Scheduler* sched) { self_.promise().set_scheduler(sched); }

   protected:
    std::coroutine_handle<promise_type> self_{nullptr};
};

// NOTE: Async can only be called inside a coroutine context
template <typename T = void>
class Async : public AsyncBase<T> {
   public:
    auto await_resume() const& -> const T& {
        std::cout << "await_resume\n";
        return this->self_.promise().get_return_value();
    }

    auto await_resume() && -> T&& {
        std::cout << "await_resume\n";
        return std::move(this->self_.promise()).get_return_value();
    }

   protected:
    using AsyncBase<T>::AsyncBase;
};

template <>
class Async<void> : public AsyncBase<void> {
   public:
    void await_resume() const noexcept { std::cout << "await_resume\n"; }

   protected:
    using AsyncBase<void>::AsyncBase;
};

struct async_promise_base {
    struct ResumeCallerAwaiter {
        constexpr bool await_ready() const noexcept { return false; }
        constexpr void await_resume() const noexcept { /* should never be called */ }

        template <typename Promise>
        std::coroutine_handle<> await_suspend(
            std::coroutine_handle<Promise> h) const noexcept {
            std::cout << "switch to caller\n";
            return h.promise().get_caller();
        }
    };

    ~async_promise_base() { std::cout << "promise destroyed\n"; }
    async_promise_base() { std::cout << "promise created\n"; }

    std::suspend_always initial_suspend() noexcept {
        std::cout << "initial suspend\n";
        return {};
    }
    auto final_suspend() noexcept { return ResumeCallerAwaiter{}; }

    void set_caller(std::coroutine_handle<> handle) noexcept { caller_ = handle; }
    std::coroutine_handle<> get_caller() noexcept { return caller_; }

    void set_scheduler(Scheduler* other) noexcept { scheduler_ = other; }
    Scheduler* get_scheduler() noexcept { return scheduler_; }

    void unhandled_exception() { exception_ = std::current_exception(); }
    void rethrow_if_exception() {
        if (exception_) [[unlikely]] {
            std::rethrow_exception(exception_);
        }
    }

   protected:
    std::coroutine_handle<> caller_{};
    std::exception_ptr exception_{nullptr};
    Scheduler* scheduler_{nullptr};
};

template <typename T>
struct async_promise : async_promise_base {
    auto get_return_object() { return Async<T>{this}; }

    template <typename U>
    void return_value(U&& val) {
        value_ = std::forward<U>(val);
    }
    const T& get_return_value() const { return value_; }

   protected:
    T value_{};
};

template <>
struct async_promise<void> : async_promise_base {
    auto get_return_object() { return Async<void>{this}; }

    void return_void() {}
};

template <typename T>
class RO {
   public:
    ~RO() {
        if (self_) {
            self_.destroy();
        }
    }

    struct promise_type {
        auto get_return_object() { return RO<T>{this}; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        template <typename V>
        void return_value(V&& val) {
            v_ = std::forward<V>(val);
        }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() {}

        T get_value() { return *v_; }

       private:
        std::optional<T> v_;
    };

    T get() { return self_.promise().get_value(); }

   private:
    explicit RO(promise_type* promise) {
        self_ = std::coroutine_handle<promise_type>::from_promise(*promise);
    }
    std::coroutine_handle<promise_type> self_{nullptr};
};

template <typename T>
RO<T> spawn_task(Async<T> task, Scheduler* scheduler = nullptr) {
    task.set_scheduler(scheduler);
    co_return co_await task;
}

}  // namespace shcoro