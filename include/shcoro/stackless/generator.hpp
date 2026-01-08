#pragma once

#include <coroutine>
#include <exception>
#include <iterator>
#include <type_traits>

namespace shcoro {

template <typename T>
class Generator {
   public:
    struct promise_type {
        auto get_return_object() { return Generator<T>{this}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        template <typename V>
        std::suspend_always yield_value(V&& val) {
            value_ = std::forward<V>(val);
            return {};
        }
        void unhandled_exception() { exception_ = std::current_exception(); }

        const T& value() const { return value_; }

        void rethrow_if_exception() {
            if (exception_) {
                std::rethrow_exception(exception_);
            }
        }

       private:
        T value_{};
        std::exception_ptr exception_{nullptr};
    };

   private:
    using coroutine_handle_t = std::coroutine_handle<promise_type>;
    struct sentinel {};

    struct iterator {
        iterator() {}

        explicit iterator(coroutine_handle_t coroutine) : handle_(coroutine) {}

        friend bool operator==(const iterator& it, sentinel) {
            return it.handle_ == nullptr || it.handle_.done();
        }
        friend bool operator==(sentinel s, const iterator& it) { return (it == s); }

        friend bool operator!=(const iterator& it, sentinel s) { return !(it == s); }
        friend bool operator!=(sentinel s, const iterator& it) { return it != s; }

        iterator& operator++() {
            handle_.resume();
            if (handle_.done()) [[unlikely]] {
                handle_.promise().rethrow_if_exception();
            }
            return *this;
        }

        void operator++(int) { (void)operator++(); }

        const T& operator*() const { return handle_.promise().value(); }

        const T* operator->() const { return std::addressof(operator*()); }

       private:
        coroutine_handle_t handle_{nullptr};
    };

   public:
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;

    Generator(Generator&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    Generator& operator=(Generator&& other) noexcept {
        handle_ = other.handle_;
        other.handle_ = nullptr;
        return *this;
    }

    ~Generator() {
        if (handle_) {
            handle_.destroy();
        }
    }

    auto begin() {
        if (handle_ != nullptr) {
            handle_.resume();
            if (handle_.done()) [[unlikely]] {
                handle_.promise().rethrow_if_exception();
            }
        }
        return iterator{handle_};
    }

    auto end() { return sentinel{}; }

   private:
    explicit Generator(promise_type* promise) {
        handle_ = coroutine_handle_t::from_promise(*promise);
    }

    coroutine_handle_t handle_{nullptr};
};

}  // namespace shcoro