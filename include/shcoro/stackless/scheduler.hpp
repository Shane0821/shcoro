#pragma once

#include <any>
#include <concepts>
#include <coroutine>
#include <memory>
#include <type_traits>
#include <utility>

namespace shcoro {

template <class SchedulerT>
concept SchedulerNoValue = (!requires { typename SchedulerT::value_type; }) &&
                           requires(SchedulerT& sched, std::coroutine_handle<> h) {
                               // A "no-value" scheduler only needs to be able to
                               // register/unregister a coroutine.
                               sched.register_coro(h);
                               sched.unregister_coro(h);
                           };

template <class SchedulerT>
concept SchedulerWithValue = requires(SchedulerT& sched, std::coroutine_handle<> h,
                                      typename SchedulerT::value_type v,
                                      const typename SchedulerT::value_type& cv) {
    typename SchedulerT::value_type;

    // accept either const-ref or rvalue (or both)
    requires(
        requires { sched.register_coro(h, cv); } ||
        requires { sched.register_coro(h, std::move(v)); });

    sched.unregister_coro(h);
};

template <class SchedulerT>
concept SchedulerConcept = SchedulerNoValue<SchedulerT> || SchedulerWithValue<SchedulerT>;

class Scheduler {
   private:
    struct SchedulerBase;

    template <SchedulerNoValue SchedulerT>
    struct NonOwningSchedulerModelNoValue;

    template <SchedulerWithValue SchedulerT>
    struct NonOwningSchedulerModelWithValue;

   public:
    Scheduler() = default;

    template <SchedulerConcept SchedulerT>
    Scheduler(SchedulerT& sched)
        : pimpl_([&]() -> std::unique_ptr<SchedulerBase> {
              if constexpr (SchedulerWithValue<SchedulerT>) {
                  return std::make_unique<NonOwningSchedulerModelWithValue<SchedulerT>>(
                      sched);
              } else {
                  return std::make_unique<NonOwningSchedulerModelNoValue<SchedulerT>>(
                      sched);
              }
          }()) {}

    Scheduler(const Scheduler& other)
        : pimpl_(other.pimpl_ ? other.pimpl_->clone() : nullptr) {}
    Scheduler& operator=(const Scheduler& other) {
        Scheduler copy(other);
        pimpl_.swap(copy.pimpl_);
        return *this;
    }

    ~Scheduler() = default;
    Scheduler(Scheduler&&) noexcept = default;
    Scheduler& operator=(Scheduler&&) noexcept = default;

    operator bool() const noexcept { return pimpl_ != nullptr; }

    friend void scheduler_register_coro(Scheduler& sched, std::coroutine_handle<> h) {
        sched.pimpl_->register_coro(h);
    }

    template <class ValueT>
    friend void scheduler_register_coro(Scheduler& sched, std::coroutine_handle<> h,
                                        ValueT&& v) {
        sched.pimpl_->register_coro(h, &v);
    }

    friend void scheduler_unregister_coro(Scheduler& sched, std::coroutine_handle<> h) {
        sched.pimpl_->unregister_coro(h);
    }

   private:
    struct SchedulerBase {
        virtual ~SchedulerBase() = default;
        virtual void register_coro(std::coroutine_handle<>) = 0;
        virtual void register_coro(std::coroutine_handle<>, const void*) = 0;
        virtual void unregister_coro(std::coroutine_handle<>) = 0;
        virtual std::unique_ptr<SchedulerBase> clone() const = 0;
    };

    template <SchedulerNoValue SchedulerT>
    struct NonOwningSchedulerModelNoValue final : SchedulerBase {
        explicit NonOwningSchedulerModelNoValue(SchedulerT& s) : sched_(&s) {}

        void register_coro(std::coroutine_handle<> h) override {
            sched_->register_coro(h);
        }

        void register_coro(std::coroutine_handle<> h, const void*) override {}

        void unregister_coro(std::coroutine_handle<> h) override {
            sched_->unregister_coro(h);
        }

        std::unique_ptr<SchedulerBase> clone() const override {
            return std::make_unique<NonOwningSchedulerModelNoValue>(*this);
        }

        SchedulerT* sched_{nullptr};
    };

    template <SchedulerWithValue SchedulerT>
    struct NonOwningSchedulerModelWithValue final : SchedulerBase {
        explicit NonOwningSchedulerModelWithValue(SchedulerT& s) : sched_(&s) {}

        void register_coro(std::coroutine_handle<> h) override {}

        void register_coro(std::coroutine_handle<> h, const void* value) override {
            using ValueT = typename SchedulerT::value_type;
            sched_->register_coro(h, *static_cast<const ValueT*>(value));
        }

        void unregister_coro(std::coroutine_handle<> h) override {
            sched_->unregister_coro(h);
        }

        std::unique_ptr<SchedulerBase> clone() const override {
            return std::make_unique<NonOwningSchedulerModelWithValue>(*this);
        }

        SchedulerT* sched_{nullptr};
    };

    std::unique_ptr<SchedulerBase> pimpl_{};
};

}  // namespace shcoro