#pragma once
#include "neuron/container/soa_container.hpp"

#include <boost/algorithm/apply_permutation.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/set.hpp>

#include <range/v3/algorithm/rotate.hpp>
#include <range/v3/utility/common_tuple.hpp>
#include <range/v3/utility/swap.hpp>
#include <range/v3/view/zip.hpp>
/** @file
 *  @brief neuron::container::soa<...> implementation requiring 3rd party headers.
 *
 *  Specifically this includes relatively-rarely-used implementations that
 *  require Boost and range::v3 headers. Source files that use these
 *  implementations will need to explicitly include this header.
 */
namespace neuron::container {
/** @brief ADL-visible swap overload for ranges::common_tuple<Ts...>.
 *
 *  It seems that because range-v3 provides ranges::swap, which is a Niebloid
 *  like std::ranges::swap, they do not provide an ADL-visible overload of the
 *  old-fashioned swap. This stops Boost's permute algorithm from finding an
 *  implementation when it is used with the types returned by zip iterators. We
 *  rely on this swap() being in the same namespace as one of the zip members
 *  (initialliy NodeIdentifier).
 */
template <typename... Ts>
void swap(ranges::common_tuple<Ts...>&& lhs, ranges::common_tuple<Ts...>&& rhs) noexcept {
    std::tuple<Ts...> lhs_std{std::move(lhs)}, rhs_std(std::move(rhs));
    std::swap(lhs_std, rhs_std);
}

/** @brief ADL-visible swap overload for ranges::common_pair<T, U>.
 *
 *  This is needed instead of the ranges::common_tuple<Ts...> overload for zips
 *  of width two.
 */
template <typename T, typename U>
void swap(ranges::common_pair<T, U>&& lhs, ranges::common_pair<T, U>&& rhs) noexcept {
    using pair_t = std::pair<T, U>;
    std::tuple<T, U> lhs_std{pair_t{std::move(lhs)}}, rhs_std(pair_t{std::move(rhs)});
    std::swap(lhs_std, rhs_std);
}

/** @brief Create a zip view of all the data columns in the container.
 */
template <typename RowIdentifier, typename... Tags>
[[nodiscard]] inline auto soa<RowIdentifier, Tags...>::get_zip() {
    return ranges::views::zip(m_indices, get<Tags>()...);
}

/** @brief Permute the SOA-format data using an arbitrary vector.
 */
template <typename RowIdentifier, typename... Tags>
template <typename Range>
inline void soa<RowIdentifier, Tags...>::apply_permutation(Range& permutation) {
    check_permutation_vector(permutation);
    permute_zip(
        [&permutation](auto zip) { boost::algorithm::apply_permutation(zip, permutation); });
}

/** @brief Permute the SOA-format data using an arbitrary vector.
 */
template <typename RowIdentifier, typename... Tags>
template <typename Range>
inline void soa<RowIdentifier, Tags...>::apply_reverse_permutation(Range& permutation) {
    check_permutation_vector(permutation);
    permute_zip([&permutation](auto zip) {
        boost::algorithm::apply_reverse_permutation(zip, permutation);
    });
}

/** Check if the given range is a permutation of the first N integers.
 *  @todo Assert that the given range has an integral value type and that
 *  there is no overflow?
 */
template <typename RowIdentifier, typename... Tags>
template <typename Rng>
inline void soa<RowIdentifier, Tags...>::check_permutation_vector(Rng const& range) {
    if (ranges::size(range) != size()) {
        throw std::runtime_error("invalid permutation vector: wrong size");
    }
    std::vector<bool> seen(ranges::size(range), false);
    for (auto val: range) {
        if (!(val >= 0 && val < seen.size())) {
            throw std::runtime_error("invalid permutation vector: value out of range");
        }
        if (seen[val]) {
            throw std::runtime_error("invalid permutation vector: repeated value");
        }
        seen[val] = true;
    }
}

/** @brief Remove the i-th row from the container.
 */
template <typename RowIdentifier, typename... Tags>
void soa<RowIdentifier, Tags...>::erase(std::size_t i) {
    // pointers to the last element will be invalidated, as it gets swapped
    // into position `i`
    m_sorted = false;
    auto const old_size = size();
    assert(i < old_size);
    if (i != old_size - 1) {
        auto zip = get_zip();
        auto iter_i = ranges::next(ranges::begin(zip), i);
        auto iter_last = ranges::prev(ranges::end(zip));
        // Swap positions `i` and `old_size-1`
        ranges::iter_swap(iter_i, iter_last);
        // Tell the new entry at `i` that its index is `i` now.
        std::get<0>(*iter_i).set_current_row(i);
    }
    resize(old_size - 1);
}


/** @brief Reverse the order of the SOA-format data.
 */
template <typename RowIdentifier, typename... Tags>
inline void soa<RowIdentifier, Tags...>::reverse() {
    permute_zip([](auto zip) { std::reverse(ranges::begin(zip), ranges::end(zip)); });
}

/** @brief Rotate the SOA-format data by `i` positions.
 *  @todo  See if std::rotate can be fixed here.
 */
template <typename RowIdentifier, typename... Tags>
inline void soa<RowIdentifier, Tags...>::rotate(std::size_t i) {
    assert(i < size());
    permute_zip([i](auto& zip) {
        ranges::rotate(ranges::begin(zip), ranges::next(ranges::begin(zip), i), ranges::end(zip));
    });
}


}  // namespace neuron::container
