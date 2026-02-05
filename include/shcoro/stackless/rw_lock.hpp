#pragma once

#include <stdint.h>

#include <coroutine>
#include <queue>

#include "noncopyable.h"

namespace shcoro {
class RWLock final : noncopyable {
   public:
    struct ReadAwaiter {
        ReadAwaiter(RWLock* lock) : lock_(lock) {}

        bool await_ready() noexcept { return !lock_->has_writer_; }

        void await_suspend(std::coroutine_handle<> caller) {
            lock_->waiting_list_.push(caller);
        }

        void await_resume() noexcept { lock_->read_cnt_++; }

        RWLock* lock_{nullptr};
    };

    struct WriteAwaiter {
        WriteAwaiter(RWLock* lock) : lock_(lock) {}

        bool await_ready() noexcept {
            return !(lock_->read_cnt_ > 0 || lock_->has_writer_);
        }

        void await_suspend(std::coroutine_handle<> caller) {
            lock_->waiting_list_.push(caller);
        }

        void await_resume() noexcept { lock_->has_writer_ = true; }

        RWLock* lock_{nullptr};
    };

    RWLock() = default;
    RWLock(RWLock&& other) noexcept = default;
    ~RWLock() = default;

    bool try_read_lock() noexcept {
        if (has_writer_) {
            return false;
        }
        read_cnt_++;
        return true;
    }
    bool try_write_lock() noexcept {
        if (read_cnt_ || has_writer_) {
            return false;
        }
        has_writer_ = true;
        return false;
    }

    [[nodiscard]] ReadAwaiter read_lock() { return ReadAwaiter(this); }
    [[nodiscard]] WriteAwaiter write_lock() { return WriteAwaiter(this); }

    void read_unlock() {
        read_cnt_--;
        if (read_cnt_ == 0 && !waiting_list_.empty()) {
            auto nxt = waiting_list_.front();
            waiting_list_.pop();
            nxt.resume();
        }
    }
    void write_unlock() {
        has_writer_ = false;
        if (!waiting_list_.empty()) {
            auto nxt = waiting_list_.front();
            waiting_list_.pop();
            nxt.resume();
        }
    }

   private:
    std::queue<std::coroutine_handle<>> waiting_list_;
    size_t read_cnt_{0};
    bool has_writer_{false};
};
}  // namespace shcoro