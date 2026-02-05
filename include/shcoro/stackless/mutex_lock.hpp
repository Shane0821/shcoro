#pragma once

#include <coroutine>
#include <queue>

#include "noncopyable.h"

namespace shcoro {
class MutexLock final : noncopyable {
   public:
    struct MutexAwaiter {
        MutexAwaiter(MutexLock* mutex) : mutex_(mutex) {}

        bool await_ready() const noexcept { return mutex_->try_lock(); }

        void await_suspend(std::coroutine_handle<> caller) {
            mutex_->waiting_list_.push(caller);
        }

        void await_resume() const noexcept {}

        MutexLock* mutex_{nullptr};
    };

    MutexLock() = default;
    MutexLock(MutexLock&& other) noexcept = default;
    ~MutexLock() = default;

    bool try_lock() noexcept {
        auto ret = locked_;
        locked_ = true;
        return !ret;
    }

    [[nodiscard]] MutexAwaiter lock() { return MutexAwaiter(this); }

    void unlock() {
        if (!waiting_list_.empty()) {
            auto nxt = waiting_list_.front();
            waiting_list_.pop();
            nxt.resume();
        } else {
            locked_ = false;
        }
    }

   private:
    std::queue<std::coroutine_handle<>> waiting_list_;
    bool locked_ = false;
};
}  // namespace shcoro