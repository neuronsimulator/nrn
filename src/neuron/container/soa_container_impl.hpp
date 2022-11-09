#pragma once
#include "neuron/container/soa_container.hpp"

namespace neuron::container {
namespace detail {
/** Check if the given range is a permutation of the first N integers.
 */
template <typename Rng>
void check_permutation_vector(Rng const& range, std::size_t size) {
    if (range.size() != size) {
        throw std::runtime_error("invalid permutation vector: wrong size");
    }
    std::vector<bool> seen(size, false);
    for (auto val: range) {
        if (!(val >= 0 && val < size)) {
            throw std::runtime_error("invalid permutation vector: value out of range");
        }
        if (seen[val]) {
            throw std::runtime_error("invalid permutation vector: repeated value " +
                                     std::to_string(val));
        }
        seen[val] = true;
    }
}

template <typename Diff>
void swap_all(Diff i, Diff n) {}

template <typename Diff, typename T, typename... U>
void swap_all(Diff i, Diff n, T&& t, U&&... u) {
    using std::swap;
    using plain_T = typename std::remove_reference_t<std::remove_cv_t<T>>::value_type;
    if constexpr (std::is_arithmetic_v<plain_T> ||
                  std::is_same_v<plain_T, non_owning_identifier_without_container>) {
        auto it = std::begin(t);
        swap(it[i], it[n]);
    } else {
        for (auto& e: t) {
            auto it = std::begin(e);
            swap(it[i], it[n]);
        }
    }
    swap_all(i, n, std::forward<U>(u)...);
}

template <typename IndexType, typename... T>
void apply_reverse_permutation(IndexType&& ind, T&&... items) {
    using std::swap;
    auto size = std::distance(std::begin(ind), std::end(ind));
    using Diff = decltype(size);
    for (Diff i = 0; i < size; i++) {
        auto it = std::begin(ind);
        while (i != it[i]) {
            Diff next = it[i];
            swap_all(i, next, std::forward<T>(items)...);
            swap(it[i], it[next]);
        }
    }
}
}  // namespace detail

/** @brief Permute the SOA-format data using an arbitrary vector.
 */
template <typename Storage, typename... Tags>
template <typename Range>
inline void soa<Storage, Tags...>::apply_reverse_permutation(Range permutation) {
    // Check that the given vector is a valid permutation of length size().
    std::size_t const my_size{size()};
    detail::check_permutation_vector(permutation, my_size);
    // Applying a permutation in general invalidates indices, so it is forbidden if the structure is
    // frozen, and it leaves the structure unsorted.
    if (m_frozen_count) {
        throw_error("apply_reverse_permutation() called on a frozen structure");
    }
    mark_as_unsorted_impl<true>();
    // Now we apply the reverse permutation in `permutation` to all of the columns in the
    // container. Depending on whether or not the tag types have num_instances() member functions
    // the relevant elements of m_data will either be std::vector<T> or std::vector<std::vector<U>>,
    // where in both cases T and U are simple value types that can be swapped.
    detail::apply_reverse_permutation(std::move(permutation),
                                      m_indices,
                                      std::get<tag_index_v<Tags>>(m_data)...);
    // update the indices in the container
    for (auto i = 0ul; i < my_size; ++i) {
        m_indices[i].set_current_row(i);
    }
}
}  // namespace neuron::container
