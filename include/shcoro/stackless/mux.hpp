#pragma once

#include <functional>
#include <vector>

#include "async.hpp"
#include "traits.h"

namespace shcoro {

template <typename T>
class Mux;

template <typename T>
class MuxAdapter;

template <typename T>
class [[nodiscard]] Mux : noncopyable {
   public:
    struct promise_type : promise_suspend_base<std::suspend_always, ResumeCallerAwaiter>,
                          promise_return_base<T>,
                          promise_caller_base,
                          promise_scheduler_base,
                          promise_exception_base {
        promise_type() { SHCORO_LOG("mux promise created: ", this); }
        ~promise_type() { SHCORO_LOG("mux promise destroyed: ", this); }

        auto get_return_object() { return Mux{this}; }

        void set_resume_limit(size_t num) noexcept { resume_limit_ = num; }
        void finish_one() noexcept { finish_count_++; }
        bool resumable() const noexcept { return finish_count_ >= resume_limit_; }
        void log_progress() const {
            SHCORO_LOG("mux progress: ", finish_count_, "/", resume_limit_);
        }

       protected:
        size_t finish_count_{0};
        size_t resume_limit_{0};
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
        SHCORO_LOG("mux await resumed: ", &self_.promise());
        return std::move(this->self_.promise().get_return_value());
    }

    void await_resume()
        requires(std::is_same_v<T, void>)
    {
        SHCORO_LOG("mux await resumed: ", &self_.promise());
    }

    Mux(Mux&& other) noexcept : self_(std::exchange(other.self_, {})) {}

    ~Mux() {
        if (self_) {
            SHCORO_LOG("Mux destroy: ", this);
            self_.destroy();
        }
    }

   private:
    explicit Mux(promise_type* promise) {
        self_ = std::coroutine_handle<promise_type>::from_promise(*promise);
        SHCORO_LOG("Mux created: ", this);
    }

    std::coroutine_handle<promise_type> self_{nullptr};
};

// Adapter between Async and Mux
template <typename T>
class [[nodiscard]] MuxAdapter : noncopyable {
   public:
    using value_type = T;
    using resume_mux_callback = std::function<std::coroutine_handle<>(replace_void_t<T>)>;

    struct ResumeMuxAwaiter;

    struct promise_type : promise_suspend_base<std::suspend_always, ResumeMuxAwaiter>,
                          promise_return_base<T>,
                          promise_exception_base {
        promise_type() {
            SHCORO_LOG("mux adapter promise created: ", this);
            resume_mux_cb_ = [](replace_void_t<T>) -> std::coroutine_handle<> {
                SHCORO_LOG("default resume mux cb called");
                return std::noop_coroutine();
            };
        }
        ~promise_type() { SHCORO_LOG("mux adapter promise destroyed: ", this); }

        auto get_return_object() { return MuxAdapter{this}; }

        void set_resume_mux_callback(auto&& cb) { resume_mux_cb_ = cb; }
        auto get_resume_mux_callback() const { return resume_mux_cb_; }

       protected:
        resume_mux_callback resume_mux_cb_;
    };

    struct ResumeMuxAwaiter {
        constexpr bool await_ready() const noexcept { return false; }
        constexpr void await_resume() const noexcept { /* should never be called */ }
        std::coroutine_handle<> await_suspend(
            std::coroutine_handle<promise_type> h) const noexcept {
            SHCORO_LOG("mux adapter final suspense and call resume cb: ", &h.promise());
            if constexpr (!std::is_same_v<T, void>) {
                SHCORO_LOG("none void cb");
                return h.promise().get_resume_mux_callback()(
                    std::move(h.promise().get_return_value()));
            } else {
                SHCORO_LOG("void cb");
                return h.promise().get_resume_mux_callback()(replace_void_t<T>{});
            }
        }
    };

    auto get() const {
        if constexpr (!std::is_same_v<T, void>) {
            return self_.promise().get_return_value();
        } else {
            return replace_void_t<T>{};
        }
    }

    auto get_self() const noexcept { return self_; }

    void set_resume_mux_callback(auto&& cb) {
        self_.promise().set_resume_mux_callback(cb);
    }

    void resume() const { return self_.resume(); }
    bool done() const noexcept { return self_.done(); }

    MuxAdapter(MuxAdapter&& other) noexcept : self_(std::exchange(other.self_, {})) {}

    ~MuxAdapter() {
        if (self_) {
            SHCORO_LOG("MuxAdapter destroy: ", this);
            self_.destroy();
        }
    }

   private:
    explicit MuxAdapter(promise_type* promise) {
        self_ = std::coroutine_handle<promise_type>::from_promise(*promise);
        SHCORO_LOG("MuxAdapter created: ", this);
    }

    std::coroutine_handle<promise_type> self_{nullptr};
};

}  // namespace shcoro