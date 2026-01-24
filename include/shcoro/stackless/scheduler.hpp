#pragma once

#include <type_traits>

namespace shcoro {
// wraps any scheduler to perform async operations
class AsyncScheduler {
   public:
    AsyncScheduler() = default;

    AsyncScheduler(void* self, const std::type_info* type) {
        self_ = self;
        type_ = type;
    }

    explicit operator bool() const noexcept { return self_ != nullptr; }

    template <class S>
    static AsyncScheduler from(S& s) noexcept {
        return AsyncScheduler{(void*)std::addressof(s), &typeid(S)};
    }

    template <class S>
    S* as() const noexcept {
        if (type_ == &typeid(S)) [[likely]] {
            return static_cast<S*>(self_);
        }
        return nullptr;
    }

   protected:
    void* self_{nullptr};
    const std::type_info* type_{nullptr};
};

}  // namespace shcoro