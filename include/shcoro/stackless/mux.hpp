#pragma once

#include <vector>

#include "async.hpp"

namespace shcoro {

template <typename... Args>
class Mux;

template <typename T, typename... Args>
class MuxAdapter;

template <typename... Args>
class [[nodiscard]] Mux : noncopyable {
   public:
    struct promise_type
        : promise_suspend_base<std::suspend_always, ResumeCallerAwaiter<promise_type>>,
          promise_return_base<Args...>,
          promise_caller_base,
          promise_scheduler_base,
          promise_exception_base {
        promise_type() { children_.reserve(sizeof...(Args)); }

        auto get_return_object() { return Mux{this}; }

        void set_self(std::coroutine_handle<promise_type> self) noexcept { self_ = self; }

        void add_child(std::coroutine_handle<> child) { children_.emplace_back(child); }

        void set_wakeup_limit(size_t num) noexcept { wake_up_limit_ = num; }
        std::coroutine_handle<> finish_one() noexcept {
            finish_count_++;
            if (finish_count_ < wake_up_limit_) {
                return std::noop_coroutine();
            }
            return self_;
        }

       protected:
        std::coroutine_handle<promise_type> self_{};
        std::vector<std::coroutine_handle<>> children_{};
        size_t finish_count_{0};
        size_t wake_up_limit_{sizeof...(Args)};
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
        requires((sizeof...(Args) > 1) || (!std::is_same_v<Args..., void>))
    {
        this->self_.promise().rethrow_if_exception();
        return std::move(this->self_.promise().get_return_value());
    }

    void await_resume()
        requires((sizeof...(Args) == 1) && std::is_same_v<Args..., void>)
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

struct SetMuxWakeupLimitAwaiter {
    SetMuxWakeupLimitAwaiter(size_t limit) : limit_(limit) {}

    constexpr bool await_ready() const noexcept { return false; }
    constexpr void await_resume() const noexcept {}
    template <typename... Args>
    bool await_suspend(
        std::coroutine_handle<typename Mux<Args...>::promise_type> h) const noexcept {
        h.promise().set_wakeup_limit(limit_);
        return false;
    }

    size_t limit_;
};

// Adapter between Async and Mux
template <typename T, typename... Args>
class [[nodiscard]] MuxAdapter : noncopyable {
   public:
    using mux_promise_type = Mux<Args...>::promise_type;

    struct ResumeMuxAwaiter;

    struct promise_type : promise_suspend_base<std::suspend_never, ResumeMuxAwaiter>,
                          promise_return_base<T>,
                          promise_exception_base {
        auto get_return_object() { return MuxAdapter{this}; }

        void set_mux(std::coroutine_handle<mux_promise_type> mux) { mux_ = mux; }
        std::coroutine_handle<mux_promise_type> get_mux() const { return mux_; }

       protected:
        std::coroutine_handle<mux_promise_type> mux_{};
    };

    struct ResumeMuxAwaiter {
        constexpr bool await_ready() const noexcept { return false; }
        constexpr void await_resume() const noexcept { /* should never be called */ }
        std::coroutine_handle<> await_suspend(
            std::coroutine_handle<promise_type> h) const noexcept {
            return h.promise().get_mux().promise().finish_one();
        }
    };

    constexpr bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<mux_promise_type> mux) noexcept {
        self_.promise().set_mux(mux); // ! should set this before execution
        return false;  // back to mux
    }

    void await_resume() { self_.promise().rethrow_if_exception(); }

    T get() {
        self_.promise().rethrow_if_exception();
        if constexpr (!std::is_same_v<T, void>) {
            return self_.promise().get_return_value();
        }
    }

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