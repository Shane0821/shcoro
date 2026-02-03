#pragma once

#include "mux.hpp"

namespace shcoro {

template <typename... T>
struct AllOfAwaiter {
    AllOfAwaiter(MuxAdapter<T>&&... adapters)
        : adapters_(std::make_tuple(std::move(adapters)...)) {}

    constexpr bool await_ready() const noexcept { return false; }

    template <typename MuxPromise>
    bool await_suspend(std::coroutine_handle<MuxPromise> mux) {
        return std::apply(
            [=](auto&&... adapters) {
                auto fn = [&](auto& adapter) {
                    adapter.resume();
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

    auto await_resume() const {
        return std::apply(
            [](auto&&... adapters) { return std::make_tuple(adapters.get()...); },
            adapters_);
    }

   protected:
    std::tuple<MuxAdapter<T>...> adapters_;
};

template <typename... T>
struct AnyOfAwaiter {
    using return_type = any_of_return_t<T...>;

    AnyOfAwaiter(MuxAdapter<T>&&... adapters)
        : adapters_(std::make_tuple(std::move(adapters)...)) {}

    constexpr bool await_ready() const noexcept { return false; }

    template <typename MuxPromise>
    bool await_suspend(std::coroutine_handle<MuxPromise> mux) {
        mux.promise().set_resume_limit(1);
        return await_suspend_impl(mux, std::index_sequence_for<T...>{});
    }

    auto await_resume() { return std::move(ret_); }

   protected:
    template <typename MuxPromise, std::size_t... Is>
    bool await_suspend_impl(std::coroutine_handle<MuxPromise> mux,
                            std::index_sequence<Is...>) {
        // return true to suspend, false to continue immediately
        return std::apply(
            [&](auto&&... adapters) {
                auto try_one = [&](auto I, auto&& adapter) -> bool {
                    using Adapter = std::decay_t<decltype(adapter)>;
                    using value_type = replace_void_t<typename Adapter::value_type>;

                    adapter.resume();
                    if (adapter.done()) {
                        // Use compile-time index: Is is constexpr
                        this->ret_ = indexed_type<I, value_type>{adapter.get()};
                        return true;
                    }

                    arm_callback<I>(adapter, mux);
                    return false;
                };

                // Expand with compile-time indices
                bool completed =
                    (try_one(std::integral_constant<std::size_t, Is>{}, adapters) || ...);

                // suspend if not completed immediately
                return !completed;
            },
            adapters_);
    }

    template <std::size_t I, class Adapter, class MuxPromise>
    void arm_callback(Adapter&& adapter, std::coroutine_handle<MuxPromise> mux) {
        using value_type = replace_void_t<typename std::decay_t<Adapter>::value_type>;
        adapter.set_resume_mux_callback(
            [mux, this](auto ret) mutable -> std::coroutine_handle<> {
                this->ret_ = indexed_type<I, value_type>{std::move(ret)};
                return mux;
            });
    }

    std::tuple<MuxAdapter<T>...> adapters_;
    return_type ret_;
};

};  // namespace shcoro