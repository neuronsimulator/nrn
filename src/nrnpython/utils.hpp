#pragma once
#include <cstddef>
#include <stdexcept>

#include <nanobind/nanobind.h>

namespace nrn {
namespace detail {

template <typename T, size_t I>
T get(const nanobind::tuple& t) {
    if (I >= t.size()) {
        throw std::runtime_error("Not enough arguments");
    }
    return nanobind::cast<T>(t[I]);
}


template <typename... Ts, typename Tuple, std::size_t... Is>
auto cast_tuple_impl(Tuple&& tuple, std::index_sequence<Is...>) {
    return std::make_tuple(get<Ts, Is>(std::forward<Tuple>(tuple))...);
}
}  // namespace detail

template <typename... Ts, typename Tuple>
auto cast_tuple(Tuple&& tuple) {
    return detail::cast_tuple_impl<Ts...>(std::forward<Tuple>(tuple),
                                          std::make_index_sequence<sizeof...(Ts)>{});
}

}  // namespace nrn
