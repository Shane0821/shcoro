#pragma once

#include <concepts>

#include "scheduler.hpp"

namespace shcoro {

// Async promise should store AsyncScheduler
template <typename Promise>
concept PromiseSchedulerConcept = requires(Promise p, Scheduler sched) {
    // Must have getter
    { p.get_scheduler() } -> std::same_as<Scheduler&>;

    // Must have setter
    p.set_scheduler(sched);
};

}  // namespace shcoro