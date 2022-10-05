#pragma once
#include "backtrace_utils.h"
#include "neuron/container/data_handle.hpp"
#include "neuron/container/soa_identifier.hpp"

#include <functional>
#include <string_view>
#include <utility>
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

template <typename RowIdentifier, typename... Tags>
struct soa;

/** @brief Token whose lifetime manages frozen/sorted state of a container.
 */
template <typename Container>
struct state_token {
    constexpr state_token(state_token&& other)
        : m_container{std::exchange(other.m_container, nullptr)} {}
    constexpr state_token(state_token const&) = delete;
    constexpr state_token& operator=(state_token&& other) {
        m_container = std::exchange(other.m_container, nullptr);
        return *this;
    }
    constexpr state_token& operator=(state_token const&) = delete;
    ~state_token() {
        if (m_container) {
            m_container->decrease_frozen_count();
        }
    }

  private:
    template <typename RowIdentifier, typename... Tags>
    friend struct soa;
    constexpr state_token(Container& container)
        : m_container{&container} {}
    Container* m_container{};
};

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
        if (m_frozen_count) {
            throw std::runtime_error("soa<...>::erase called in read-only mode");
        }
        mark_as_unsorted_impl<true>();
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
        m_indices.resize(old_size - 1);
        (get<Tags>().resize(old_size - 1), ...);
    }

    /** @brief Get the size of the container.
     */
    [[nodiscard]] std::size_t size() const {
        // Check our various std::vector members are still the same size as each
        // other.
        assert(((m_indices.size() == get<Tags>().size()) && ...));
        return m_indices.size();
    }

    /** @brief Query if the storage is currently frozen.
     *
     *  When the container is frozen then no operations are allowed that would
     *  change the address of any data, however the values themselves may still
     *  be read from and written to. A container that is sorted and frozen is
     *  guaranteed to remain sorted until it is thawed (unfrozen).
     */
    [[nodiscard]] bool is_frozen() const {
        return m_frozen_count;
    }

  private:
    friend struct state_token<soa>;

    /** @brief Flag that the storage is no longer frozen.
     *
     *  This is called from the destructor of state_token.
     */
    void decrease_frozen_count() {
        --m_frozen_count;
    }

  public:
    /** @brief Return type of get_sorted_token()
     */
    using sorted_token_type = state_token<soa>;

    /** @brief Mark the container as sorted and return a token guaranteeing that.
     *
     *  Is is user-defined precisely what "sorted" means, but the soa<...> class
     *  makes some guarantees:
     *  - if the container is frozen, no pointers to elements in the underlying
     *    storage will be invalidated -- attempts to do so will throw or abort.
     *  - if the container is not frozen, it will remain flagged as sorted until
     *    a potentially-pointer-invalidating operation (insertion, deletion,
     *    permutation) occurs, or mark_as_unsorted() is called.
     *
     *  The container will be frozen for the lifetime of the token returned from
     *  this function, and therefore also sorted for at least that time. This
     *  token has the semantics of a unique_ptr, i.e. it cannot be copied but
     *  can be moved, and destroying a moved-from token has no effect.
     *
     *  The tokens returned by this function are reference counted; the
     *  container will be frozen for as long as any token is alive.
     *
     *  @todo A future extension could be to preserve the sorted flag until
     *        pointers are actually, not potentially, invalidated.
     */
    [[nodiscard]] sorted_token_type get_sorted_token() {
        // Increment the reference count, marking the container as frozen.
        ++m_frozen_count;
        // Mark the container as sorted
        m_sorted = true;
        // Return a token that calls decrease_frozen_count() at the end of its lifetime
        return sorted_token_type{*this};
    }

    /** @brief Tell the container it is no longer sorted.
     *
     *  The meaning of being sorted is externally defined, and it is possible
     *  that some external change to an input of the (external) algorithm
     *  defining the sort order can mean that the data are no longer considered
     *  sorted, even if nothing has actually changed inside this container.
     */
    void mark_as_unsorted() {
        mark_as_unsorted_impl<false>();
    }

    /** @brief Set the callback that is invoked when the container becomes unsorted.
     *
     *  This is invoked by mark_as_unsorted() and when a container operation
     *  (insertion, permutation, deletion) causes the container to transition
     *  from being sorted to being unsorted.
     */
    void set_unsorted_callback(std::function<void()> unsorted_callback) {
        m_unsorted_callback = std::move(unsorted_callback);
    }

    /** @brief Query if the underlying vectors are still "sorted".
     *
     *  See the documentation of get_sorted_token() for an explanation of what
     *  this means.
     */
    [[nodiscard]] bool is_sorted() const {
        return m_sorted;
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

  private:
    [[nodiscard]] inline auto get_zip();

    template <bool internal>
    void mark_as_unsorted_impl() {
        if (m_frozen_count) {
            // Currently you can only obtain a frozen container by calling
            // get_sorted_token(), which explicitly guarantees that the
            // container will remain sorted for the lifetime of the returned
            // token.
            throw_error("mark_as_unsorted() called on a frozen structure");
        }
        // Only execute the callback if we're transitioning from sorted to
        // or if this was an explicit mark_as_unsorted() call
        bool const execute_callback{m_sorted || !internal};
        m_sorted = false;
        if (execute_callback && m_unsorted_callback) {
            m_unsorted_callback();
        }
    }


  public:
    /** @brief Append a new entry to all elements of the container.
     *  @todo  Perhaps the return type should be an owning view, not just an
     *         owning row identifier.
     *  @todo  Perhaps a different name would be better, given the comment below
     *         about the semantics.
     *
     *  Note that this has slightly strange semantics: the returned object owns
     *  row that has been added, so if you discard the return value then the
     *  added row will immediately be deleted.
     */
    [[nodiscard]] owning_identifier_base<soa, RowIdentifier> emplace_back() {
        if (m_frozen_count) {
            throw_error("emplace_back() called on a frozen structure");
        }
        // Important that this comes after the m_frozen_count check
        owning_identifier_base<soa, RowIdentifier> index{*this};
        // this can trigger reallocation
        mark_as_unsorted_impl<true>();
        index.set_current_row(size());
        m_indices.push_back(std::move(index));
        // Append a new element to all the data columns too.
        (get<Tags>().emplace_back(), ...);
        return index;
    }

    /** @brief Get the offset-th identifier.
     */
    [[nodiscard]] RowIdentifier identifier(std::size_t offset) const {
        return m_indices.at(offset);
    }

    /** @brief Get the offset-th element of the column named by Tag.
     *
     *  Because this is returning a single value, it is permitted even in
     *  read-only mode. The container being in read only mode means that
     *  operations that would invalidate iterators/pointers are forbidden, not
     *  that actual data values cannot change.
     */
    template <typename Tag>
    [[nodiscard]] typename Tag::type& get(std::size_t offset) {
        static_assert(has_tag_v<Tag>);
        return std::get<storage_t<Tag>>(m_data).at(offset);
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
        if (m_frozen_count) {
            throw_error("non-const get() called on frozen structure");
        }
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
        // Don't go via non-const get<T>() because of the m_frozen_count check
        find_data_handle(handle, m_indices, ptr) ||
            (find_data_handle(handle, std::get<storage_t<Tags>>(m_data), ptr) || ...);
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
                // Probably OK to call this in read-only mode?
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
        if (m_frozen_count) {
            throw_error("permute_zip() called on a frozen structure");
        }
        // uncontroversial that applying a permutation changes the underlying
        // storage organisation and potentially invalidates pointers. slightly
        // more controversial: should explicitly permuting the data implicitly
        // mark the container as "sorted"?
        mark_as_unsorted_impl<true>();
        auto zip = get_zip();
        permutation(zip);
        std::size_t const my_size{size()};
        for (auto i = 0ul; i < my_size; ++i) {
            m_indices[i].set_current_row(i);
        }
    }

    [[noreturn]] void throw_error(std::string_view message) {
        std::ostringstream oss;
        oss << cxx_demangle(typeid(soa).name()) << "[frozen_count=" << m_frozen_count
            << ",sorted=" << std::boolalpha << m_sorted << "]: " << message;
        throw std::runtime_error(std::move(oss).str());
    }

    /** @brief Flag for get_sorted_token(), mark_as_unsorted() and is_sorted().
     */
    bool m_sorted{false};

    /** @brief Reference count for tokens guaranteeing the container is in frozen mode.
     */
    std::size_t m_frozen_count{false};

    /** @brief Pointers to identifiers that record the current physical row.
     */
    std::vector<RowIdentifier> m_indices{};

    /** @brief Collection of data columns.
     */
    std::tuple<storage_t<Tags>...> m_data{};

    /** @brief Callback that is invoked when the container becomes unsorted.
     */
    std::function<void()> m_unsorted_callback{};
};
}  // namespace neuron::container
