#pragma once

#include <functional>
#include <vector>

#include "async.hpp"

namespace shcoro {

template <typename... T>
class Mux;

template <typename T>
class MuxAdapter;

template <typename... T>
class [[nodiscard]] Mux : noncopyable {
   public:
    struct promise_type
        : promise_suspend_base<std::suspend_always, ResumeCallerAwaiter<promise_type>>,
          promise_return_base<T...>,
          promise_caller_base,
          promise_scheduler_base,
          promise_exception_base {
        promise_type() { children_.reserve(sizeof...(T)); }

        auto get_return_object() { return Mux{this}; }

        void set_self(std::coroutine_handle<promise_type> self) noexcept { self_ = self; }
        auto get_self() const noexcept { return self_; }

        void add_child(std::coroutine_handle<> child) { children_.emplace_back(child); }

        void set_resume_limit(size_t num) noexcept { resume_limit_ = num; }
        void finish_one() noexcept { finish_count_++; }
        bool resumable() const noexcept { return finish_count_ >= resume_limit_; }

       protected:
        std::coroutine_handle<promise_type> self_{};
        std::vector<std::coroutine_handle<>> children_{};
        size_t finish_count_{0};
        size_t resume_limit_{sizeof...(T)};
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
        requires((sizeof...(T) > 1) || (!std::is_same_v<T..., void>))
    {
        this->self_.promise().rethrow_if_exception();
        return std::move(this->self_.promise().get_return_value());
    }

    void await_resume()
        requires((sizeof...(T) == 1) && std::is_same_v<T..., void>)
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

    struct promise_type : promise_suspend_base<std::suspend_never, ResumeMuxAwaiter>,
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

    T get() const {
        self_.promise().rethrow_if_exception();
        if constexpr (!std::is_same_v<T, void>) {
            return self_.promise().get_return_value();
        }
    }

    bool done() { return self_.done(); }

    void set_resume_mux_callback(auto&& cb) {
        self_.promise().set_resume_mux_callback(cb);
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

template <typename... T>
struct AllOfAwaiter {
    AllOfAwaiter(MuxAdapter<T>... adapters)
        : adapters_(std::make_tuple<MuxAdapter<T>...>(std::move(adapters)...)) {}

    constexpr bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<typename Mux<T...>::promise_type> mux) {
        static_assert(sizeof...(T) > 0);
        return std::apply(
            [&](auto&&... adapters) {
                auto fn = [&](auto& adapter) {
                    if (adapter.done()) {
                        mux.promise().finish_one();
                    } else {
                        adapter.set_resume_mux_callback(
                            [mux]() -> std::coroutine_handle<> {
                                auto& promise = mux.promise();
                                promise.finish_one();
                                if (!promise.resumable()) {
                                    return std::noop_coroutine();
                                }
                                return mux;
                            });
                    }
                };

                (fn(adapters), ...);
                return mux.promise().resumable();
            },
            adapters_);
    }
    constexpr std::tuple<T...> await_resume() noexcept {
        return std::apply(
            [](auto&&... adapters) { return std::make_tuple<T...>(adapters.get()...); },
            adapters_);
    }

   protected:
    std::tuple<MuxAdapter<T>...> adapters_;
};

}  // namespace shcoro