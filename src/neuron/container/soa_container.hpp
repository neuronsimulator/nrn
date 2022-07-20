#pragma once
#include <boost/algorithm/apply_permutation.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/set.hpp>

#include <range/v3/algorithm/rotate.hpp>
// #include <range/v3/algorithm/unstable_remove_if.hpp>
#include <range/v3/utility/common_tuple.hpp>
#include <range/v3/utility/swap.hpp>
#include <range/v3/view/zip.hpp>

#include <vector>

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

/** @brief Utility for generating SOA data structures.
 *  @tparam Tags Parameter pack of tag types that define the columns included in
 *               the container. Types may not be repeated.
 */
template <typename RowIdentifier, typename... Tags>
struct SOAContainer {
    // All elements of `Tags` should be unique.
    static_assert(boost::mp11::mp_is_set<boost::mp11::mp_list<Tags...>>::value);

    SOAContainer() = default;
    // Make it harder to invalidate the pointers/references to instances of
    // this struct that are stored in Node objects.
    SOAContainer(SOAContainer&&) = delete;
    SOAContainer(SOAContainer const&) = delete;
    SOAContainer& operator=(SOAContainer&&) = delete;
    SOAContainer& operator=(SOAContainer const&) = delete;

    /** @brief Get the size of the container.
     */
    [[nodiscard]] std::size_t size() const {
        // Check our various std::vector members are still the same size as each
        // other.
        assert(((m_indices.size() == get<Tags>().size()) && ...));
        return m_indices.size();
    }

    /** @brief Resize the container.
     */
    void resize(std::size_t size) {
        m_indices.resize(size);
        (get<Tags>().resize(size), ...);
    }

    /** @brief Reverse the order of the SOA-format data.
     */
    void reverse() {
        permute_zip([](auto zip) { std::reverse(ranges::begin(zip), ranges::end(zip)); });
    }

    /** @brief Permute the SOA-format data using an arbitrary vector.
     */
    template <typename Range>
    void apply_permutation(Range& permutation) {
        assert(ranges::size(permutation) == size());
        permute_zip(
            [&permutation](auto zip) { boost::algorithm::apply_permutation(zip, permutation); });
    }

    /** @brief Permute the SOA-format data using an arbitrary vector.
     */
    template <typename Range>
    void apply_reverse_permutation(Range& permutation) {
        assert(ranges::size(permutation) == size());
        permute_zip([&permutation](auto zip) {
            boost::algorithm::apply_reverse_permutation(zip, permutation);
        });
    }

    /** @brief Rotate the SOA-format data by `i` positions.
     *  @todo  See if std::rotate can be fixed here.
     */
    void rotate(std::size_t i) {
        assert(i < size());
        permute_zip([i](auto& zip) {
            ranges::rotate(ranges::begin(zip),
                           ranges::next(ranges::begin(zip), i),
                           ranges::end(zip));
        });
    }

    /** @brief Remove the i-th row from the container.
     */
    void erase(std::size_t i) {
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

    /** @brief Append a new entry to all elements of the container.
     *  @todo Return non-owning view/reference to the newly added entry?
     */
    void emplace_back(RowIdentifier index) {
        index.set_current_row(size());
        m_indices.push_back(std::move(index));
        // Append a new element to all the data columns too.
        (get<Tags>().emplace_back(), ...);
    }

    /** @brief Get the offset-th identifier.
     */
    [[nodiscard]] RowIdentifier identifier(std::size_t offset) const {
        return m_indices.at(offset);
    }

    /** @brief Get the offset-th element of the column named by Tag.
     */
    template <typename Tag>
    [[nodiscard]] typename Tag::type& get(std::size_t offset) {
        return get<Tag>().at(offset);
    }

    /** @brief Get the offset-th element of the column named by Tag.
     */
    template <typename Tag>
    [[nodiscard]] typename Tag::type const& get(std::size_t offset) const {
        return get<Tag>().at(offset);
    }

  private:
    /** @brief Get the column container named by Tag.
     */
    template <typename Tag>
    [[nodiscard]] std::vector<typename Tag::type>& get() {
        using tag_index_t = boost::mp11::mp_find<boost::mp11::mp_list<Tags...>, Tag>;
        static_assert(tag_index_t::value < sizeof...(Tags));
        return std::get<tag_index_t::value>(m_data);
    }

    /** @brief Get the column container named by Tag.
     */
    template <typename Tag>
    [[nodiscard]] std::vector<typename Tag::type> const& get() const {
        using tag_index_t = boost::mp11::mp_find<boost::mp11::mp_list<Tags...>, Tag>;
        static_assert(tag_index_t::value < sizeof...(Tags));
        return std::get<tag_index_t::value>(m_data);
    }

    /** @brief Apply some transformation to all of the data columns at once.
     */
    template <typename Permutation>
    void permute_zip(Permutation permutation) {
        auto zip = get_zip();
        permutation(zip);
        std::size_t const my_size{size()};
        for (auto i = 0ul; i < my_size; ++i) {
            m_indices[i].set_current_row(i);
        }
    }

    /** @brief Create a zip view of all the data columns in the container.
     */
    [[nodiscard]] auto get_zip() {
        return ranges::views::zip(m_indices, get<Tags>()...);
    }

    /** @brief Pointers to identifiers that record the current physical row.
     */
    std::vector<RowIdentifier> m_indices{};

    /** @brief Collection of data columns.
     */
    std::tuple<std::vector<typename Tags::type>...> m_data{};
};