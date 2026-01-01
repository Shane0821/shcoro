#pragma once

#include <coroutine>
#include <exception>
#include <iterator>
#include <type_traits>

namespace shcoro {

template <typename T>
class generator {
   public:
    using value_type = std::remove_reference_t<T>;
    using reference_type = std::conditional_t<std::is_reference_v<T>, T, T&>;
    using pointer_type = value_type*;

    struct promise_type {
        auto get_return_object() { return generator<T>{this}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(value_type& val) {
            value_ = val;
            return {};
        }
        std::suspend_always yield_value(value_type&& val) {
            value_ = std::move(val);
            return {};
        }
        void unhandled_exception() { exception_ = std::current_exception(); }

        value_type value() const { return value_; }
        reference_type value() { return value_; }

        void rethrow_if_exception() {
            if (exception_) {
                std::rethrow_exception(exception_);
            }
        }

       private:
        value_type value_{};
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

        reference_type operator*() { return handle_.promise().value(); }

        pointer_type operator->() const { return std::addressof(operator*()); }

       private:
        coroutine_handle_t handle_{nullptr};
    };

   public:
    generator(const generator&) = delete;
    generator& operator=(const generator&) = delete;

    generator(generator&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    generator& operator=(generator&& other) noexcept {
        handle_ = other.handle_;
        other.handle_ = nullptr;
        return *this;
    }

    ~generator() {
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
    explicit generator(promise_type* promise) {
        handle_ = coroutine_handle_t::from_promise(*promise);
    }

    coroutine_handle_t handle_{nullptr};
};

}  // namespace shcoro