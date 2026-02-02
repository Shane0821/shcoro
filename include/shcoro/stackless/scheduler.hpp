#pragma once

#include <any>
#include <concepts>
#include <coroutine>
#include <memory>
#include <type_traits>
#include <utility>

namespace shcoro {

class Scheduler {
   public:
    Scheduler() = default;

    template <class SchedulerT>
        requires(!std::is_same_v<SchedulerT, Scheduler>)
    Scheduler(SchedulerT& sched)
        : pimpl_(std::make_unique<NonOwningSchedulerModel<SchedulerT>>(sched)) {}

    Scheduler(const Scheduler& other) : pimpl_(other.pimpl_->clone()) {}
    Scheduler& operator=(const Scheduler& other) {
        Scheduler copy(other);
        pimpl_.swap(copy.pimpl_);
        return *this;
    }

    ~Scheduler() = default;
    Scheduler(Scheduler&&) noexcept = default;
    Scheduler& operator=(Scheduler&&) noexcept = default;

    operator bool() const noexcept { return pimpl_ != nullptr; }

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
        virtual void register_coro(std::coroutine_handle<>, const void*) = 0;
        virtual void unregister_coro(std::coroutine_handle<>) = 0;
        virtual std::unique_ptr<SchedulerBase> clone() const = 0;
    };

    template <class SchedulerT>
    struct NonOwningSchedulerModel final : SchedulerBase {
        explicit NonOwningSchedulerModel(SchedulerT& s) : sched_(s) {}

        void register_coro(std::coroutine_handle<> h, const void* value) override {
            using ValueT = typename SchedulerT::value_type;
            sched_.register_coro(h, *static_cast<const ValueT*>(value));
        }

        void unregister_coro(std::coroutine_handle<> h) override {
            sched_.unregister_coro(h);
        }

        std::unique_ptr<SchedulerBase> clone() const override {
            return std::make_unique<NonOwningSchedulerModel>(*this);
        }

        SchedulerT& sched_;
    };

    std::unique_ptr<SchedulerBase> pimpl_;
};

}  // namespace shcoro