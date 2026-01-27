#pragma once

#include <coroutine>
#include <tuple>

#include "promise_concepts.hpp"

namespace shcoro {

template <typename PromiseType>
struct ResumeCallerAwaiter {
    constexpr bool await_ready() const noexcept { return false; }
    constexpr void await_resume() const noexcept { /* should never be called */ }
    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<PromiseType> h) const noexcept {
        return h.promise().get_caller();
    }
};

struct GetSchedulerAwaiter {
    constexpr bool await_ready() const noexcept { return false; }
    auto await_resume() const noexcept { return scheduler_; }

    template <PromiseSchedulerConcept PromiseType>
    bool await_suspend(std::coroutine_handle<PromiseType> h) noexcept {
        scheduler_ = h.promise().get_scheduler();
        return false;
    }

    AsyncScheduler scheduler_;
};

struct GetCoroAwaiter {
    constexpr bool await_ready() const noexcept { return false; }
    auto await_resume() const noexcept { return coro_; }

    bool await_suspend(std::coroutine_handle<> caller) noexcept {
        coro_ = caller;
        return false;
    }

    std::coroutine_handle<> coro_;
};

template <typename... Adapters>
struct AllOfAwaiter {
    AllOfAwaiter(Adapters&&... adapters)
        : adapters_(std::make_tuple<Adapters...>(std::forward<Adapters>(adapters)...)) {}

    constexpr bool await_ready() const noexcept { return false; }

    template <typename MuxPromise>
    bool await_suspend(std::coroutine_handle<MuxPromise> mux) {
        return std::apply(
            [=](auto&&... adapters) {
                auto fn = [&](auto& adapter) {
                    if (adapter.done()) {
                        return;
                    }
                    mux.promise().add_child(adapter.get_self());
                    adapter.set_resume_mux_callback([mux]() -> std::coroutine_handle<> {
                        auto& promise = mux.promise();
                        promise.finish_one();
                        if (!promise.resumable()) {
                            return std::noop_coroutine();
                        }
                        return mux;
                    });
                };

                (fn(adapters), ...);
                return !mux.promise().resumable();
            },
            adapters_);
    }
    auto await_resume() const noexcept {
        return std::apply(
            [](auto&&... adapters) { return std::make_tuple(adapters.get()...); },
            adapters_);
    }

   protected:
    std::tuple<Adapters...> adapters_;
};

}  // namespace shcoro