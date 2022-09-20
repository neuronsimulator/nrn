#pragma once
#include "backtrace_utils.h"
#include "neuron/container/data_handle.hpp"

#include <string_view>
#include <vector>

namespace neuron::container {
namespace detail {
// https://stackoverflow.com/a/67106430
template <typename T, typename... Types>
inline constexpr bool are_types_unique_v =
    (!std::is_same_v<T, Types> && ...) && are_types_unique_v<Types...>;
template <typename T>
inline constexpr bool are_types_unique_v<T> = true;
}  // namespace detail
/** @brief Utility for generating SOA data structures.
 *  @tparam Tags Parameter pack of tag types that define the columns included in
 *               the container. Types may not be repeated.
 */
template <typename RowIdentifier, typename... Tags>
struct soa {
    soa() = default;
    // Make it harder to invalidate the pointers/references to instances of
    // this struct that are stored in Node objects.
    soa(soa&&) = delete;
    soa(soa const&) = delete;
    soa& operator=(soa&&) = delete;
    soa& operator=(soa const&) = delete;

    /** @brief Remove the i-th row from the container.
     *
     *  This is currently implemented by swapping the last element into position
     *  i (if those are not the same element) and reducing the size by one.
     *  Iterators to the last element and the deleted element will be invalidated.
     */
    void erase(std::size_t i) {
        m_sorted = false;
        auto const old_size = size();
        assert(i < old_size);
        if (i != old_size - 1) {
            // Swap positions `i` and `old_size - 1` in each vector
            using std::swap;
            swap(m_indices[i], m_indices.back());
            (swap(get<Tags>()[i], get<Tags>().back()), ...);
            // Tell the new entry at `i` that its index is `i` now.
            m_indices[i].set_current_row(i);
        }
        resize(old_size - 1);
    }

    /** @brief Get the size of the container.
     */
    [[nodiscard]] std::size_t size() const {
        // Check our various std::vector members are still the same size as each
        // other.
        assert(((m_indices.size() == get<Tags>().size()) && ...));
        return m_indices.size();
    }

    /** @brief Mark that the underlying vectors are now "sorted".
     *
     *  Is is user-defined precisely what "sorted" means, but the soa<...> class
     *  will reset this flag after any operation that invalidates pointers to
     *  the storage vectors, such as applying a permutation, adding a new
     *  element, or deleting an element.
     *
     *  @todo Clarify whether the flag means that pointers *might* have been
     *  invalidated, or that they actually were.
     *  @todo Consider the difference between invalidating pointers and
     *  invalidating indices, and whether it's important to us.
     */
    void mark_as_sorted(bool value = true) {
        m_sorted = value;
    }

    /** @brief Query if the underlying vectors are still "sorted".
     *
     *  This returns true after a call to mark_as_sorted() if nothing has
     *  modified the storage vector layout.
     */
    [[nodiscard]] bool is_sorted() const {
        return m_sorted;
    }

    /** @brief Resize the container.
     */
    void resize(std::size_t size) {
        m_sorted = false;  // resize might trigger reallocation
        m_indices.resize(size);
        (get<Tags>().resize(size), ...);
    }

    // The following methods are defined in soa_container_impl.hpp because they
    // require Boost and range::v3 headers.
    template <typename Range>
    inline void apply_permutation(Range& permutation);
    template <typename Range>
    inline void apply_reverse_permutation(Range& permutation);
    template <typename Rng>
    inline void check_permutation_vector(Rng const& range);
    inline void reverse();
    inline void rotate(std::size_t i);

    /** @brief Append a new entry to all elements of the container.
     *  @todo Return non-owning view/reference to the newly added entry?
     */
    void emplace_back(RowIdentifier index) {
        // this can trigger reallocation
        m_sorted = false;
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
    static_assert(detail::are_types_unique_v<RowIdentifier, Tags...>,
                  "All tag types should be unique");

    template <typename Tag>
    using storage_t = std::vector<typename Tag::type>;

  public:
    template <typename Tag>
    static constexpr bool has_tag_v = std::disjunction_v<std::is_same<Tag, Tags>...>;

    /** @brief Get the column container named by Tag.
     */
    template <typename Tag>
    [[nodiscard]] std::vector<typename Tag::type>& get() {
        static_assert(has_tag_v<Tag>);
        return std::get<storage_t<Tag>>(m_data);
    }

    /** @brief Get the column container named by Tag.
     */
    template <typename Tag>
    [[nodiscard]] std::vector<typename Tag::type> const& get() const {
        static_assert(has_tag_v<Tag>);
        return std::get<storage_t<Tag>>(m_data);
    }

    /** @brief Return a permutation-stable handle if ptr is inside us.
     *  @todo Check const-correctness. Presumably a const version would return
     *  data_handle<T const>, which would hold a pointer-to-const for the container?
     */
    template <typename T>
    [[nodiscard]] neuron::container::data_handle<T> find_data_handle(T* ptr) {
        neuron::container::data_handle<T> handle{};
        find_data_handle(handle, m_indices, ptr) ||
            (find_data_handle(handle, get<Tags>(), ptr) || ...);
        return handle;
    }

  private:
    template <typename Tag, typename T>
    constexpr bool find_container_info(utils::storage_info& info,
                                       std::vector<T> const& cont1,
                                       void const* cont2) const {
        if (&cont1 != cont2) {
            return false;
        }
        info.name = cxx_demangle(typeid(Tag).name());
        info.size = cont1.size();
        constexpr std::string_view prefix{"neuron::container::"};
        if (std::string_view{info.name}.substr(0, prefix.size()) == prefix) {
            info.name.erase(0, prefix.size());
        }
        return true;
    }

  public:
    [[nodiscard]] std::optional<utils::storage_info> find_container_info(void const* cont) const {
        utils::storage_info info{};
        // FIXME: generate a proper tag type for the index column?
        if (find_container_info<RowIdentifier>(info, m_indices, cont) ||
            (find_container_info<Tags>(info, get<Tags>(), cont) || ...)) {
            return info;
        } else {
            return {std::nullopt};
        }
    }

  private:
    template <typename T, typename U>
    constexpr bool find_data_handle(neuron::container::data_handle<T>& handle,
                                    std::vector<U>& container,
                                    T* ptr) {
        assert(!handle);
        if constexpr (std::is_same_v<T, U>) {
            if (!container.empty() && ptr >= container.data() &&
                ptr < std::next(container.data(), container.size())) {
                auto const row = ptr - container.data();
                assert(row < container.size());
                // This pointer seems to live inside this container. This is all
                // a bit fragile; these pointers can be invalidated by almost
                // all mutating operations on the container. To make things a
                // bit more robust, we could insist that the container is
                // "sorted". FIXME: re-enable this
                // assert(is_sorted());
                handle = neuron::container::data_handle<T>{identifier(row), container};
                assert(handle.refers_to_a_modern_data_structure());
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    /** @brief Apply some transformation to all of the data columns at once.
     */
    template <typename Permutation>
    void permute_zip(Permutation permutation) {
        // uncontroversial that applying a permutation changes the underlying
        // storage organisation and potentially invalidates pointers. slightly
        // more controversial: should explicitly permuting the data implicitly
        // mark the container as "sorted"?
        m_sorted = false;
        auto zip = get_zip();
        permutation(zip);
        std::size_t const my_size{size()};
        for (auto i = 0ul; i < my_size; ++i) {
            m_indices[i].set_current_row(i);
        }
    }

    [[nodiscard]] auto get_zip();

    /** @brief Flag for mark_as_sorted and is_sorted().
     */
    bool m_sorted{false};

    /** @brief Pointers to identifiers that record the current physical row.
     */
    std::vector<RowIdentifier> m_indices{};

    /** @brief Collection of data columns.
     */
    std::tuple<storage_t<Tags>...> m_data{};
};
}  // namespace neuron::container
