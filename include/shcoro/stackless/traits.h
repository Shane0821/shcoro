#pragma once

#include <iostream>
#include <tuple>
#include <variant>

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

template <typename T>
using void_to_empty_t = typename void_to_empty<T>::type;

template <typename T>
using replace_void_t = void_to_empty<T>::type;

template <typename... Args>
struct replace_void_tuple {
    using type = std::tuple<typename void_to_empty<Args>::type...>;
};

template <typename... Args>
using all_of_return_t = typename replace_void_tuple<Args...>::type;

template <std::size_t I, class T>
struct indexed_type {
    using value_type = T;
    T value;
};

template <class Seq, class... Args>
struct replace_void_indexed_variant_impl;

template <std::size_t... I, class... Args>
struct replace_void_indexed_variant_impl<std::index_sequence<I...>, Args...> {
    using type = std::variant<indexed_type<I, void_to_empty_t<Args>>...>;
};

template <class... Args>
struct replace_void_indexed_variant {
    using type =
        typename replace_void_indexed_variant_impl<std::index_sequence_for<Args...>,
                                                   Args...>::type;
};

template <typename... Args>
using any_of_return_t = typename replace_void_indexed_variant<Args...>::type;

};  // namespace shcoro