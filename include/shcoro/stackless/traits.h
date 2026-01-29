#pragma once

#include <iostream>
#include <tuple>

namespace shcoro {

struct empty {
    // for debugging
    friend std::ostream& operator<<(std::ostream& os, const empty&) {
        return os << "[empty]";
    }
};

template <typename T>
struct void_to_empty {
    using type = T;
};

template <>
struct void_to_empty<void> {
    using type = empty;
};

template <typename... Args>
struct replace_void_tuple {
    using type = std::tuple<typename void_to_empty<Args>::type...>;
};

template <typename... Args>
using replace_void_tuple_t = typename replace_void_tuple<Args...>::type;

};  // namespace shcoro