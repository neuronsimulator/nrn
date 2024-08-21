#pragma once
#include <cstddef>
#include <stdexcept>

#include <nanobind/nanobind.h>
#include <fmt/format.h>

namespace nrn {
namespace detail {

template <typename T, size_t I>
T get(const nanobind::tuple& t) {
    return nanobind::cast<T>(t[I]);
}

template <typename... Ts, std::size_t... Is>
auto cast_tuple_impl(const nanobind::tuple& tuple, std::index_sequence<Is...>) {
    auto count = sizeof...(Is);
    if (count > tuple.size()) {
        throw std::runtime_error(fmt::format(
            "Not enough arguments, expected {}, but only received {}", count, tuple.size()));
    }
    return std::make_tuple(get<Ts, Is>(tuple)...);
}
}  // namespace detail

/*!
 * Extract an exact number of elements from a tuple, including casting them to certain types.
 *
 * This uses `nanobind::cast`, so expect a `nanobind::cast_error()` if there are incompatible types.
 * If the number of supplied elements in the tuple is fewer than the expected number, a std::runtime
 * error is thrown.
 *
 * @param tuple can either be a nanobind::args or a nanobind::tuple
 */
template <typename... Ts>
auto cast_tuple(const nanobind::tuple& tuple) {
    return detail::cast_tuple_impl<Ts...>(tuple,
                                          std::make_index_sequence<sizeof...(Ts)>{});
}

}  // namespace nrn
