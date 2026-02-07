#pragma once

#include <stdint.h>

#include <coroutine>
#include <queue>

#include "shcoro/utils/noncopyable.h"

namespace shcoro {

enum class RWLockPolicy { READ_RPIOR, FAIR };

template <RWLockPolicy Policy = RWLockPolicy::READ_RPIOR>
class RWLock final : noncopyable {
   public:
    struct ReadAwaiter {
        ReadAwaiter(RWLock* lock) : lock_(lock) {}

        bool await_ready() noexcept { return !lock_->writer_active_; }

        void await_suspend(std::coroutine_handle<> caller) {
            lock_->waiting_list_.push(caller);
        }

        void await_resume() noexcept { lock_->active_reader_++; }

        RWLock* lock_{nullptr};
    };

    struct WriteAwaiter {
        WriteAwaiter(RWLock* lock) : lock_(lock) {}

        bool await_ready() noexcept {
            return (lock_->active_reader_ == 0 && !lock_->writer_active_);
        }

        void await_suspend(std::coroutine_handle<> caller) {
            lock_->waiting_list_.push(caller);
        }

        void await_resume() noexcept { lock_->writer_active_ = true; }

        RWLock* lock_{nullptr};
    };

    RWLock() = default;
    RWLock(RWLock&& other) noexcept = default;
    ~RWLock() = default;

    bool try_read_lock() noexcept {
        if (writer_active_) {
            return false;
        }
        active_reader_++;
        return true;
    }
    bool try_write_lock() noexcept {
        if (active_reader_ || writer_active_) {
            return false;
        }
        writer_active_ = true;
        return true;
    }

    [[nodiscard]] ReadAwaiter read_lock() { return ReadAwaiter(this); }
    [[nodiscard]] WriteAwaiter write_lock() { return WriteAwaiter(this); }

    void read_unlock() {
        if (--active_reader_ != 0) return;
        if (!waiting_list_.empty()) {
            auto nxt = waiting_list_.front();
            waiting_list_.pop();
            nxt.resume();
        }
    }
    void write_unlock() {
        writer_active_ = false;
        if (!waiting_list_.empty()) {
            auto nxt = waiting_list_.front();
            waiting_list_.pop();
            nxt.resume();
        }
    }

   private:
    std::queue<std::coroutine_handle<>> waiting_list_;
    size_t active_reader_{0};
    bool writer_active_{false};
};

template <>
class RWLock<RWLockPolicy::FAIR> final : noncopyable {
   public:
    struct ReadAwaiter {
        ReadAwaiter(RWLock* lock) : lock_(lock) {}

        bool await_ready() noexcept {
            return (!lock_->writer_active_ && !lock_->waiting_writer_);
        }

        void await_suspend(std::coroutine_handle<> caller) {
            lock_->waiting_list_.push(caller);
        }

        void await_resume() noexcept { lock_->active_reader_++; }

        RWLock* lock_{nullptr};
    };

    struct WriteAwaiter {
        WriteAwaiter(RWLock* lock) : lock_(lock) {}

        bool await_ready() noexcept {
            return (!lock_->active_reader_ && !lock_->writer_active_);
        }

        void await_suspend(std::coroutine_handle<> caller) {
            lock_->waiting_list_.push(caller);
            lock_->waiting_writer_++;
        }

        void await_resume() noexcept {
            lock_->waiting_writer_--;
            lock_->writer_active_ = true;
        }

        RWLock* lock_{nullptr};
    };

    RWLock() = default;
    RWLock(RWLock&& other) noexcept = default;
    ~RWLock() = default;

    bool try_read_lock() noexcept {
        if (writer_active_ || waiting_writer_) {
            return false;
        }
        active_reader_++;
        return true;
    }
    bool try_write_lock() noexcept {
        if (active_reader_ || writer_active_) {
            return false;
        }
        writer_active_ = true;
        return true;
    }

    [[nodiscard]] ReadAwaiter read_lock() { return ReadAwaiter(this); }
    [[nodiscard]] WriteAwaiter write_lock() { return WriteAwaiter(this); }

    void read_unlock() {
        if (--active_reader_ != 0) return;
        if (!waiting_list_.empty()) {
            auto nxt = waiting_list_.front();
            waiting_list_.pop();
            nxt.resume();
        }
    }
    void write_unlock() {
        writer_active_ = false;
        if (!waiting_list_.empty()) {
            auto nxt = waiting_list_.front();
            waiting_list_.pop();
            nxt.resume();
        }
    }

   private:
    std::queue<std::coroutine_handle<>> waiting_list_;
    size_t active_reader_{0};
    size_t waiting_writer_{0};
    bool writer_active_{false};
};
}  // namespace shcoro