#pragma once

#include <coroutine>
#include <list>
#include <unordered_map>

#include "promise_concepts.hpp"
#include "shcoro/utils/logger.h"

namespace shcoro {
class FIFOScheduler {
   public:
    void register_coro(std::coroutine_handle<> coro) {
        SHCORO_LOG("fifo register: ", coro.address());
        auto it = coros_.insert(coros_.end(), coro);
        coro_map_[coro.address()] = it;
    }

    void unregister_coro(std::coroutine_handle<> coro) {
        auto addr = coro.address();
        if (coro_map_.find(addr) != coro_map_.end()) {
            SHCORO_LOG("fifo unregister: ", addr);
            coros_.erase(coro_map_[addr]);
            coro_map_.erase(addr);
        }
    }

    void run_once() {
        if (!coros_.empty()) {
            SHCORO_LOG("remaining task: ", coros_.size());
            auto handle = coros_.front();
            SHCORO_LOG("unregister handle: ", handle.address());
            unregister_coro(handle);
            SHCORO_LOG("resume handle: ", handle.address());
            handle.resume();
        }
    }

    void run() {
        while (!coros_.empty()) {
            SHCORO_LOG("remaining task: ", coros_.size());
            auto handle = coros_.front();
            SHCORO_LOG("unregister handle: ", handle.address());
            unregister_coro(handle);
            SHCORO_LOG("resume handle: ", handle.address());
            handle.resume();
        }
    }

   private:
    std::list<std::coroutine_handle<>> coros_;
    std::unordered_map<void*, decltype(coros_)::iterator> coro_map_;
};

using FIFOAwaiter = SchedulerAwaiter<void>;

}  // namespace shcoro
