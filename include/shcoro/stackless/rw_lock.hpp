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

        void await_resume() noexcept { lock_->active_readers_++; }

        RWLock* lock_{nullptr};
    };

    struct WriteAwaiter {
        WriteAwaiter(RWLock* lock) : lock_(lock) {}

        bool await_ready() noexcept {
            return (lock_->active_readers_ == 0 && !lock_->writer_active_);
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
        active_readers_++;
        return true;
    }
    bool try_write_lock() noexcept {
        if (active_readers_ || writer_active_) {
            return false;
        }
        writer_active_ = true;
        return true;
    }

    [[nodiscard]] ReadAwaiter read_lock() { return ReadAwaiter(this); }
    [[nodiscard]] WriteAwaiter write_lock() { return WriteAwaiter(this); }

    void read_unlock() {
        if (--active_readers_ != 0) return;
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
    size_t active_readers_{0};
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
            lock_->waiting_list_.push(CoroNode{.handle_ = caller, .is_writer_ = false});
        }

        void await_resume() noexcept { lock_->active_readers_++; }

        RWLock* lock_{nullptr};
    };

    struct WriteAwaiter {
        WriteAwaiter(RWLock* lock) : lock_(lock) {}

        bool await_ready() noexcept {
            return (!lock_->active_readers_ && !lock_->writer_active_);
        }

        void await_suspend(std::coroutine_handle<> caller) {
            lock_->waiting_list_.push(CoroNode{.handle_ = caller, .is_writer_ = true});
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
        active_readers_++;
        return true;
    }
    bool try_write_lock() noexcept {
        if (active_readers_ || writer_active_) {
            return false;
        }
        writer_active_ = true;
        return true;
    }

    [[nodiscard]] ReadAwaiter read_lock() { return ReadAwaiter(this); }
    [[nodiscard]] WriteAwaiter write_lock() { return WriteAwaiter(this); }

    void read_unlock() {
        if (--active_readers_ != 0) return;
        if (!waiting_list_.empty()) {
            auto nxt = waiting_list_.front().handle_;
            waiting_list_.pop();
            nxt.resume();
        }
    }
    void write_unlock() {
        writer_active_ = false;

        if (waiting_list_.empty()) return;

        auto nxt = waiting_list_.front();
        waiting_list_.pop();
        nxt.handle_.resume();
        if (nxt.is_writer_) return;

        // resume grouped readers
        while (!waiting_list_.empty() && !waiting_list_.front().is_writer_) {
            auto nxt = waiting_list_.front().handle_;
            waiting_list_.pop();
            nxt.resume();
        }
    }

   private:
    struct CoroNode {
        std::coroutine_handle<> handle_;
        bool is_writer_;
    };

    std::queue<CoroNode> waiting_list_;
    size_t active_readers_{0};
    size_t waiting_writer_{0};
    bool writer_active_{false};
};
}  // namespace shcoro