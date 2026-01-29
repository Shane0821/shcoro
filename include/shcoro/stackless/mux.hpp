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
    struct promise_type
        : promise_suspend_base<std::suspend_always, ResumeCallerAwaiter>,
          promise_return_base<T>,
          promise_caller_base,
          promise_scheduler_base,
          promise_exception_base {
        auto get_return_object() { return Mux{this}; }

        void set_self(std::coroutine_handle<promise_type> self) noexcept { self_ = self; }
        auto get_self() const noexcept { return self_; }

        void add_child(std::coroutine_handle<> child) {
            resume_limit_++;
            children_.emplace_back(child);
        }

        void set_resume_limit(size_t num) noexcept { resume_limit_ = num; }
        void finish_one() noexcept { finish_count_++; }
        bool resumable() const noexcept { return finish_count_ >= resume_limit_; }

       protected:
        std::coroutine_handle<promise_type> self_{};
        std::vector<std::coroutine_handle<>> children_{};
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
        this->self_.promise().rethrow_if_exception();
        return std::move(this->self_.promise().get_return_value());
    }

    void await_resume()
        requires(std::is_same_v<T, void>)
    {
        this->self_.promise().rethrow_if_exception();
    }

    Mux(Mux&& other) noexcept : self_(std::exchange(other.self_, nullptr)) {}
    Mux& operator=(Mux&& other) noexcept {
        self_ = std::exchange(other.self_, nullptr);
        return *this;
    }

    ~Mux() {
        if (self_) {
            self_.destroy();
        }
    }

   private:
    explicit Mux(promise_type* promise) {
        self_ = std::coroutine_handle<promise_type>::from_promise(*promise);
        self_.promise().set_self(self_);
    }

    std::coroutine_handle<promise_type> self_{nullptr};
};

// Adapter between Async and Mux
template <typename T>
class [[nodiscard]] MuxAdapter : noncopyable {
   public:
    using resume_mux_callback = std::function<std::coroutine_handle<>()>;

    struct ResumeMuxAwaiter;

    struct promise_type : promise_suspend_base<std::suspend_always, ResumeMuxAwaiter>,
                          promise_return_base<T>,
                          promise_exception_base {
        promise_type() {
            resume_mux_cb_ = []() -> std::coroutine_handle<> {
                return std::noop_coroutine();
            };
        }

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
            return h.promise().get_resume_mux_callback()();
        }
    };

    auto get() const {
        self_.promise().rethrow_if_exception();
        if constexpr (!std::is_same_v<T, void>) {
            return self_.promise().get_return_value();
        } else {
            return empty{};
        }
    }

    auto get_self() const noexcept { return self_; }

    void set_resume_mux_callback(auto&& cb) {
        self_.promise().set_resume_mux_callback(cb);
    }

    void resume() const { return self_.resume(); }
    bool done() const noexcept { return self_.done(); }

    MuxAdapter(MuxAdapter&& other) noexcept
        : self_(std::exchange(other.self_, nullptr)) {}
    MuxAdapter& operator=(MuxAdapter&& other) noexcept {
        self_ = std::exchange(other.self_, nullptr);
        return *this;
    }

    ~MuxAdapter() {
        if (self_) {
            self_.destroy();
        }
    }

   private:
    explicit MuxAdapter(promise_type* promise) {
        self_ = std::coroutine_handle<promise_type>::from_promise(*promise);
    }

    std::coroutine_handle<promise_type> self_{nullptr};
};

}  // namespace shcoro