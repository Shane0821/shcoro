#pragma once

#include "mux.hpp"

namespace shcoro {

template <typename... T>
struct AllOfAwaiter {
    AllOfAwaiter(MuxAdapter<T>... adapters)
        : adapters_(std::make_tuple(std::move(adapters)...)) {}

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
    std::tuple<MuxAdapter<T>...> adapters_;
};
};