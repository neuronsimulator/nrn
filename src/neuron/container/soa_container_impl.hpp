#pragma once
#include "neuron/container/soa_container.hpp"

#include <boost/algorithm/apply_permutation.hpp>
#include <boost/mp11.hpp>

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

namespace detail {
template <typename Storage, typename Indices, typename... Tags>
auto get_zip_helper(Storage& storage, Indices& indices, boost::mp11::mp_list<Tags...>) {
    return ranges::views::zip(indices, storage.template get<Tags>()...);
}
template <typename Storage, typename Permutation, typename... Tags>
void permute_zip_helper(Storage& storage,
                        Permutation const& permutation,
                        boost::mp11::mp_list<Tags...>) {
    (
        [&](auto const& tag) {
            using Tag = std::decay_t<decltype(tag)>;
            auto const num_instances = tag.num_instances();
            for (auto i = 0ul; i < num_instances; ++i) {
                // The apply-a-permutation-vector algorithms that we use modify
                // both the values being permuted (obviously) and the
                // permutation vector itself (not-so-obviously), so it's
                // important that we don't execute the algorithms using the same
                // vector multiple times. In the absense of fields with
                // `num_instances()` copies, this is fine because we create one
                // zip and then apply the permutation one time. But with
                // `num_instances()` we have to copy the permutation vector.
                auto perm_copy = permutation;
                perm_copy(storage.template get_field_instance<Tag>(i));
            }
        }(storage.template get_tag<Tags>()),
        ...);
}
}  // namespace detail

/** @brief Create a zip view of all the data columns in the container that are
 *         not duplicated a number of times set at runtime.
 */
template <typename Storage, template <typename> typename Interface, typename... Tags>
[[nodiscard]] inline auto soa<Storage, Interface, Tags...>::get_zip() {
    using Tags_without_num_instances =
        boost::mp11::mp_remove_if<boost::mp11::mp_list<Tags...>, detail::has_num_instances>;
    return detail::get_zip_helper(*this, m_indices, Tags_without_num_instances{});
}

/** @brief Apply some transformation to all of the data columns at once.
 */
template <typename Storage, template <typename> typename Interface, typename... Tags>
template <typename Permutation>
inline void soa<Storage, Interface, Tags...>::permute_zip(Permutation&& permutation) {
    if (m_frozen_count) {
        throw_error("permute_zip() called on a frozen structure");
    }
    // uncontroversial that applying a permutation changes the underlying
    // storage organisation and potentially invalidates pointers. slightly
    // more controversial: should explicitly permuting the data implicitly
    // mark the container as "sorted"?
    mark_as_unsorted_impl<true>();
    // Apply `permutation` to the columns from tags that do define num_instances()
    using Tags_with_num_instances =
        boost::mp11::mp_copy_if<boost::mp11::mp_list<Tags...>, detail::has_num_instances>;
    // this will copy `permutation` before invoking it, so it is safe to invoke
    // it multiple times
    detail::permute_zip_helper(*this, permutation, Tags_with_num_instances{});
    auto zip = get_zip();  // without tags that define num_instances()
    permutation(zip);      // cannot safely call `permutation` after this
    std::size_t const my_size{size()};
    for (auto i = 0ul; i < my_size; ++i) {
        m_indices[i].set_current_row(i);
    }
}

/** @brief Permute the SOA-format data using an arbitrary vector.
 */
template <typename Storage, template <typename> typename Interface, typename... Tags>
template <typename Range>
inline void soa<Storage, Interface, Tags...>::apply_permutation(Range permutation) {
    check_permutation_vector(permutation);
    permute_zip([permutation = std::move(permutation)](auto& zip) mutable {
        boost::algorithm::apply_permutation(zip, permutation);
    });
}

/** @brief Permute the SOA-format data using an arbitrary vector.
 */
template <typename Storage, template <typename> typename Interface, typename... Tags>
template <typename Range>
inline void soa<Storage, Interface, Tags...>::apply_reverse_permutation(Range permutation) {
    check_permutation_vector(permutation);
    permute_zip([permutation = std::move(permutation)](auto& zip) mutable {
        boost::algorithm::apply_reverse_permutation(zip, permutation);
    });
}

/** Check if the given range is a permutation of the first N integers.
 *  @todo Assert that the given range has an integral value type and that
 *  there is no overflow?
 */
template <typename Storage, template <typename> typename Interface, typename... Tags>
template <typename Rng>
inline void soa<Storage, Interface, Tags...>::check_permutation_vector(Rng const& range) {
    if (ranges::size(range) != size()) {
        throw std::runtime_error("invalid permutation vector: wrong size");
    }
    std::vector<bool> seen(ranges::size(range), false);
    for (auto val: range) {
        if (!(val >= 0 && val < seen.size())) {
            throw std::runtime_error("invalid permutation vector: value out of range");
        }
        if (seen[val]) {
            throw std::runtime_error("invalid permutation vector: repeated value " +
                                     std::to_string(val));
        }
        seen[val] = true;
    }
}

/** @brief Reverse the order of the SOA-format data.
 */
template <typename Storage, template <typename> typename Interface, typename... Tags>
inline void soa<Storage, Interface, Tags...>::reverse() {
    permute_zip([](auto& zip) { std::reverse(ranges::begin(zip), ranges::end(zip)); });
}

/** @brief Rotate the SOA-format data by `i` positions.
 *  @todo  See if std::rotate can be fixed here.
 */
template <typename Storage, template <typename> typename Interface, typename... Tags>
inline void soa<Storage, Interface, Tags...>::rotate(std::size_t i) {
    assert(i < size());
    permute_zip([i](auto& zip) {
        ranges::rotate(ranges::begin(zip), ranges::next(ranges::begin(zip), i), ranges::end(zip));
    });
}
}  // namespace neuron::container
