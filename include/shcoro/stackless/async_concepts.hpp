#pragma once

#include <concepts>

#include "scheduler.hpp"

namespace shcoro {

// Async promise should store AsyncScheduler
template <typename Promise>
concept AsyncPromiseConcept = requires(Promise p, AsyncScheduler sched) {
    // Must have getter
    { p.get_scheduler() } -> std::same_as<AsyncScheduler>;

    // Must have setter
    p.set_scheduler(sched);
};

}  // namespace shcoro