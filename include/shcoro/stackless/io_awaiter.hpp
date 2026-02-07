#pragma once

#include <sys/time.h>

#include <chrono>
#include <coroutine>
#include <set>
#include <thread>
#include <unordered_map>

#include "promise_concepts.hpp"
#include "shcoro/utils/logger.h"

namespace shcoro {
template <typename IOHandle>
struct IOAwaiter {
    IOAwaiter(const IOHandle& handle) : io_handle_(handle) {}
    IOAwaiter(IOHandle&& handle) : io_handle_(std::move(handle)) {}

    constexpr bool await_ready() const noexcept { return false; }

    template <shcoro::PromiseSchedulerConcept CallerPromiseType>
    auto await_suspend(std::coroutine_handle<CallerPromiseType> caller) noexcept {
        scheduler_register_coro(caller.promise().get_scheduler(), caller,
                                std::move(io_handle_));
    }

    void await_resume() const noexcept {}

    IOHandle io_handle_;
};
}  // namespace shcoro
