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

template <typename... Ts, typename Tuple, std::size_t... Is>
auto cast_tuple_impl(Tuple&& tuple, std::index_sequence<Is...>) {
    auto count = sizeof...(Is);
    if (count > tuple.size()) {
        throw std::runtime_error(fmt::format(
            "Not enough arguments, expected {}, but only received {}", count, tuple.size()));
    }
    return std::make_tuple(get<Ts, Is>(std::forward<Tuple>(tuple))...);
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
template <typename... Ts, typename Tuple>
auto cast_tuple(Tuple&& tuple) {
    return detail::cast_tuple_impl<Ts...>(std::forward<Tuple>(tuple),
                                          std::make_index_sequence<sizeof...(Ts)>{});
}

}  // namespace nrn
