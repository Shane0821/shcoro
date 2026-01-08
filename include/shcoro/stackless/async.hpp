#pragma once

#include <concepts>
#include <coroutine>
#include <exception>
#include <type_traits>

#include "scheduler.h"

namespace shcoro {

template <typename T>
concept AsyncPromiseConcept = requires(T t, Scheduler* sched) {
    // Must have getter
    { t.get_scheduler() } -> std::same_as<Scheduler*>;

    // Must have setter
    t.set_scheduler(sched);
};

template <typename IOHandleType>
struct IOAwaiter {
    IOAwaiter(IOHandleType handle) { handle_ = handle; }

    constexpr bool await_ready() const noexcept { return false; }

    template <AsyncPromiseConcept PromiseType>
    void await_suspend(std::coroutine_handle<PromiseType> caller) {}

    constexpr void await_resume() const noexcept {}

    IOHandleType handle_;
};

template <typename T = void>
class Async {
   public:
    using value_type = std::remove_reference_t<T>;
    using reference_type = std::conditional_t<std::is_reference_v<T>, T, T&>;
    using pointer_type = value_type*;

    Async(const Async&) = delete;
    Async& operator=(const Async&) = delete;

    Async(Async&& other) noexcept : self_(other.self_) { other.self_ = nullptr; }
    Async& operator=(Async&& other) noexcept {
        self_ = other.self_;
        other.self_ = nullptr;
        return *this;
    }

    ~Async() {
        if (self_) {
            self_.destroy();
        }
    }

    struct promise_type {
        auto get_return_object() { return Async<T>{this}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        template <typename V>
        void return_value(V&& val) {
            value_ = std::forward<V>(val);
        }
        auto final_suspend() noexcept {
            struct ResumeCaller {
                constexpr bool await_ready() const noexcept { return false; }
                constexpr void await_resume() noexcept {}
                std::coroutine_handle<> await_suspend(
                    std::coroutine_handle<promise_type> h) {
                    // symmetric transfer
                    return h.promise().get_caller();
                }
            };
            return ResumeCaller{};
        }
        void unhandled_exception() { std::rethrow_exception(std::current_exception()); }

        void set_caller(std::coroutine_handle<> handle) { caller_ = handle; }
        decltype(auto) get_caller() { return caller_; }

        void set_scheduler(Scheduler* other) { scheduler_ = other; }
        Scheduler* get_scheduler() { return scheduler_; }

        value_type get_return_value() const { return value_; }

       private:
        std::coroutine_handle<> caller_;
        Scheduler* scheduler_{nullptr};
        value_type value_;
    };

    void set_scheduler(Scheduler* sched) { self_.promise().set_scheduler(sched); }

    constexpr bool await_ready() const noexcept { return false; }

    template <AsyncPromiseConcept OtherPromiseType>
    auto await_suspend(std::coroutine_handle<OtherPromiseType> caller) {
        self_.promise().set_caller(caller);
        self_.promise().set_scheduler(caller.promise().get_scheduler());
        return self_;
    }

    value_type await_resume() const { return self_.promise().get_return_value(); }

   private:
    explicit Async(promise_type* promise) {
        self_ = std::coroutine_handle<promise_type>::from_promise(*promise);
    }

    std::coroutine_handle<promise_type> self_{nullptr};
};

template <typename T>
class RO {
   public:
    struct promise_type {
        void return_value(T& val) { v_ = val; }
        void return_value(T&& val) { v_ = std::move(val); }

        T get_value() { return *v_; }

       private:
        std::optional<T> v_;
    };

    T result() { return self_.promise().get_value(); }

   private:
    std::coroutine_handle<promise_type> self_{nullptr};
};

template <typename T>
RO<T> spawn_task(Scheduler* shced, Async<T> task) {
    task.set_scheduler(sched);
    co_return co_await task;
}

}  // namespace shcoro