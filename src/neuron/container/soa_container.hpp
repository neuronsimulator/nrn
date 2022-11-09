#pragma once
#include "backtrace_utils.h"
#include "neuron/container/data_handle.hpp"
#include "neuron/container/soa_identifier.hpp"

#include <atomic>
#include <functional>
#include <limits>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace neuron::container {
namespace detail {
template <typename T, typename... Ts>
inline constexpr bool type_in_pack_v = std::disjunction_v<std::is_same<T, Ts>...>;
// https://stackoverflow.com/a/67106430
template <typename T, typename... Types>
inline constexpr bool are_types_unique_v =
    (!std::is_same_v<T, Types> && ...) && are_types_unique_v<Types...>;
template <typename T>
inline constexpr bool are_types_unique_v<T> = true;
// https://stackoverflow.com/a/18063608
template <typename T, typename Tuple>
struct index_of_type_helper;
template <typename T, typename... Ts>
struct index_of_type_helper<T, std::tuple<T, Ts...>> {
    static constexpr std::size_t value = 0;
};
template <typename T, typename U, typename... Ts>
struct index_of_type_helper<T, std::tuple<U, Ts...>> {
    static constexpr std::size_t value = 1 + index_of_type_helper<T, std::tuple<Ts...>>::value;
};
template <typename T, typename... Ts>
inline constexpr std::size_t index_of_type_v = []() {
    constexpr bool Ts_are_unique = are_types_unique_v<Ts...>;
    constexpr bool T_is_in_Ts = type_in_pack_v<T, Ts...>;
    static_assert(Ts_are_unique,
                  "index_of_type_v<T, Ts...> assumes there are no duplicates in Ts...");
    static_assert(T_is_in_Ts, "index_of_type_v<T, Ts...> assumes that T occurs in Ts...");
    // make the error message better by avoiding instantiating index_of_type_helper if the
    // assertions fail
    if constexpr (Ts_are_unique && T_is_in_Ts) {
        return index_of_type_helper<T, std::tuple<Ts...>>::value;
    } else {
        return std::numeric_limits<std::size_t>::max();  // unreachable without hitting
                                                         // static_assert
    }
}();

// Detect if a type T has a non-static member function called default_value
template <typename T, typename = void>
inline constexpr bool has_default_value_v = false;
template <typename T>
inline constexpr bool
    has_default_value_v<T, std::void_t<decltype(std::declval<T>().default_value())>> = true;

// Detect if a type T has a non-static member function called num_instances().
template <typename T, typename = void>
struct has_num_instances: std::false_type {};
template <typename T>
struct has_num_instances<T, std::void_t<decltype(std::declval<T>().num_instances())>>
    : std::true_type {};
template <typename T>
inline constexpr bool has_num_instances_v = has_num_instances<T>::value;

// Detect if a type T has a non-static member function called num_instances().
template <typename T, typename = void>
struct has_name: std::false_type {};
template <typename T>
struct has_name<T, std::void_t<decltype(std::declval<T>().name())>>: std::true_type {};
template <typename T>
inline constexpr bool has_name_v = has_name<T>::value;

// std::type_identity in C++20
template <typename T>
struct type_identity {
    using type = T;
};
}  // namespace detail

/** @brief Token whose lifetime manages frozen/sorted state of a container.
 */
template <typename Container>
struct state_token {
    constexpr state_token() = default;
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
    template <typename, typename...>
    friend struct soa;
    constexpr state_token(Container& container)
        : m_container{&container} {}
    Container* m_container{};
};

/**
 * @brief Utility for generating SOA data structures.
 * @headerfile neuron/container/soa_container.hpp
 * @tparam Storage    Name of the actual storage type derived from soa<...>.
 * @tparam Tags       Parameter pack of tag types that define the columns
 *                    included in the container. Types may not be repeated.
 *
 * This CRTP base class is used to implement the ~global SOA storage structs
 * that hold (so far) Node and Mechanism data. Ownership of rows in these
 * structs is managed via instances of the owning identifier type @ref
 * neuron::container::owning_identifier instantiated with Storage, and
 * non-owning reference to rows in the data structures are managed via instances
 * of the @ref neuron::container::non_owning_identifier template instantiated
 * with Storage. These identifiers are typically wrapped in a
 * data-structure-specific (i.e. Node- or Mechanism-specific) interface type
 * that provides data-structure-specific accessors and methods to obtain actual
 * data values and more generic handle types such as @ref
 * neuron::container::data_handle<T> and @ref
 * neuron::container::generic_data_handle.
 */
template <typename Storage, typename... Tags>
struct soa {
    /**
     * @brief Construct with default-constructed tag type instances.
     */
    soa() {
        initialise_data();
    }

    /**
     * @brief Construct with specific tag instances.
     *
     * This is useful if the tag types are not empty, for example if the number
     * of times a column is duplicated is a runtime value.
     */
    soa(Tags&&... tag_instances)
        : m_tags{std::forward<Tags>(tag_instances)...} {
        initialise_data();
    }

    /**
     * @brief @ref soa is not movable
     *
     * This is to make it harder to accidentally invalidate pointers-to-storage
     * in handles.
     */
    soa(soa&&) = delete;

    /**
     * @brief @ref soa is not copiable
     *
     * This is partly to make it harder to accidentally invalidate
     * pointers-to-storage in handles, and partly because it could be very
     * expensive so it might be better to be more explicit.
     */
    soa(soa const&) = delete;

    /**
     * @brief @ref soa is not move assignable
     *
     * For the same reason it isn't movable.
     */
    soa& operator=(soa&&) = delete;

    /**
     * @brief @ref soa is not copy assignable
     *
     * For the same reasons it isn't copy constructible
     */
    soa& operator=(soa const&) = delete;

    /**
     * @brief Get the size of the container.
     */
    [[nodiscard]] std::size_t size() const {
        // Check our various std::vector members are still the same size as each
        // other. This check could be omitted in release builds...
        for_all_vectors(*this, [check_size = m_indices.size()](auto const& tag, auto const& vec) {
            assert(vec.size() == check_size);
        });
        return m_indices.size();
    }

  private:
    /**
     * @brief Remove the @f$i^{\text{th}}@f$ row from the container.
     *
     * This is currently implemented by swapping the last element into position
     * @f$i@f$ (if those are not the same element) and reducing the size by one.
     * Iterators to the last element and the deleted element will be
     * invalidated.
     */
    void erase(std::size_t i) {
        if (m_frozen_count) {
            throw_error("erase() called on a frozen structure");
        }
        mark_as_unsorted_impl<true>();
        auto const old_size = size();
        assert(i < old_size);
        if (i != old_size - 1) {
            // Swap positions `i` and `old_size - 1` in each vector
            for_all_vectors(*this, [i](auto const& tag, auto& vec) {
                using std::swap;
                swap(vec[i], vec.back());
            });
            // Tell the new entry at `i` that its index is `i` now.
            m_indices[i].set_current_row(i);
        }
        for_all_vectors(*this, [new_size = old_size - 1](auto const& tag, auto& vec) {
            vec.resize(new_size);
        });
    }

    friend struct state_token<Storage>;
    friend struct owning_identifier<Storage>;

    static_assert(detail::are_types_unique_v<non_owning_identifier<Storage>, Tags...>,
                  "All tag types should be unique");
    template <typename Tag>
    static constexpr std::size_t tag_index_v = detail::index_of_type_v<Tag, Tags...>;

    template <typename Tag>
    void initialise_data_helper(Tag const& tag) {
        if constexpr (detail::has_num_instances_v<Tag>) {
            auto& vector_of_vectors = std::get<tag_index_v<Tag>>(m_data);
            assert(vector_of_vectors.empty());
            vector_of_vectors.resize(tag.num_instances());
        }
    }

    // for tags with a runtime-specified number of copies, set that number
    void initialise_data() {
        // For all tags that have a runtime-specified number of copies, get the
        // vector<vector<T>> and resize it to hold num_instances() vector<T>
        (initialise_data_helper(get_tag<Tags>()), ...);
    }

    template <typename Tag, typename This, typename Callable>
    static void for_all_tag_vectors(This& this_ref, Callable const& callable) {
        auto const& tag = this_ref.template get_tag<Tag>();
        if constexpr (detail::has_num_instances_v<Tag>) {
            auto const num_instances = tag.num_instances();
            for (auto i = 0; i < num_instances; ++i) {
                callable(tag, this_ref.template get_field_instance<Tag>(i));
            }
        } else {
            callable(tag, this_ref.template get<Tag>());
        }
    }

    /**
     * @brief Apply the given function to all data vectors in this container.
     *
     * This centralises handling of tag types that are duplicated a
     * runtime-specified number of times.
     */
    template <typename This, typename Callable>
    static void for_all_vectors(This& this_ref, Callable const& callable) {
        callable(detail::type_identity<non_owning_identifier<Storage>>{}, this_ref.m_indices);
        (for_all_tag_vectors<Tags>(this_ref, callable), ...);
    }

    /**
     * @brief Flag that the storage is no longer frozen.
     *
     * This is called from the destructor of state_token.
     */
    void decrease_frozen_count() {
        assert(m_frozen_count);
        --m_frozen_count;
    }

  public:
    /**
     * @brief Return type of get_sorted_token()
     */
    using sorted_token_type = state_token<Storage>;

    /**
     * @brief Mark the container as sorted and return a token guaranteeing that.
     *
     * Is is user-defined precisely what "sorted" means, but the soa<...> class
     * makes some guarantees:
     * - if the container is frozen, no pointers to elements in the underlying
     *   storage will be invalidated -- attempts to do so will throw or abort.
     * - if the container is not frozen, it will remain flagged as sorted until
     *   a potentially-pointer-invalidating operation (insertion, deletion,
     *   permutation) occurs, or mark_as_unsorted() is called.
     *
     * The container will be frozen for the lifetime of the token returned from
     * this function, and therefore also sorted for at least that time. This
     * token has the semantics of a unique_ptr, i.e. it cannot be copied but
     * can be moved, and destroying a moved-from token has no effect.
     *
     * The tokens returned by this function are reference counted; the
     * container will be frozen for as long as any token is alive.
     *
     * Note that "frozen" refers to the storage layout, not to the stored value,
     * meaning that values inside a frozen container can still be modified --
     * "frozen" is not "runtime const".
     *
     * @todo A future extension could be to preserve the sorted flag until
     *       pointers are actually, not potentially, invalidated.
     */
    [[nodiscard]] sorted_token_type get_sorted_token() {
        // Increment the reference count, marking the container as frozen.
        ++m_frozen_count;
        // Mark the container as sorted
        m_sorted = true;
        // Return a token that calls decrease_frozen_count() at the end of its lifetime
        return sorted_token_type{static_cast<Storage&>(*this)};
    }

    /**
     * @brief Tell the container it is no longer sorted.
     *
     * The meaning of being sorted is externally defined, and it is possible
     * that some external change to an input of the (external) algorithm
     * defining the sort order can mean that the data are no longer considered
     * sorted, even if nothing has actually changed inside this container.
     */
    void mark_as_unsorted() {
        mark_as_unsorted_impl<false>();
    }

    /**
     * @brief Set the callback that is invoked when the container becomes unsorted.
     *
     * This is invoked by mark_as_unsorted() and when a container operation
     * (insertion, permutation, deletion) causes the container to transition
     * from being sorted to being unsorted.
     */
    void set_unsorted_callback(std::function<void()> unsorted_callback) {
        m_unsorted_callback = std::move(unsorted_callback);
    }

    /**
     * @brief Query if the underlying vectors are still "sorted".
     *
     * See the documentation of get_sorted_token() for an explanation of what
     * this means.
     */
    [[nodiscard]] bool is_sorted() const {
        return m_sorted;
    }

    // The following methods are defined in soa_container_impl.hpp because they
    // require Boost and range::v3 headers.
    template <typename Range>
    inline void apply_reverse_permutation(Range permutation);
    template <typename Rng>
    inline void check_permutation_vector(Rng const& range);
    template <typename Permutation>
    inline void permute_zip(Permutation&& permutation);
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

    /**
     * @brief Create a new entry and return an identifier that owns it.
     *
     * Calling this method increases size() by one. Destroying (modulo move
     * operations) the returned identifier, which has the semantics of a
     * unique_ptr, decreases size() by one.
     *
     * Note that this has different semantics to standard library container
     * methods such as emplace_back(), push_back(), insert() and so on. Because
     * the returned identifier manages the lifetime of the newly-created entry,
     * discarding the return value will cause the new entry to immediately be
     * deleted.
     *
     * This is a low-level call that is useful for the implementation of the
     * owning_identifier template. The returned owning identifier is typically
     * wrapped inside an owning handle type that adds data-structure-specific
     * methods (e.g. v(), v_handle(), set_v() for a Node).
     */
    [[nodiscard]] owning_identifier<Storage> acquire_owning_identifier() {
        if (m_frozen_count) {
            throw_error("acquire_owning_identifier() called on a frozen structure");
        }
        // The .emplace_back() methods we are about to call can trigger
        // reallocation and, therefore, invalidation of pointers. At present,
        // "sorted" is defined to mean that pointers have not been invalidated.
        // There are two reasonable changes that could be made here:
        //  - possibly for release builds, we could only mark unsorted if a
        //    reallocation *actually* happens
        //  - "sorted" could be defined to mean that indices have not been
        //    invalidated -- adding a new entry to the end of the container
        //    never invalidates indices
        mark_as_unsorted_impl<true>();
        // Append to all of the vectors
        for_all_vectors(*this, [](auto const& tag, auto& vec) {
            using Tag = std::decay_t<decltype(tag)>;
            if constexpr (detail::has_default_value_v<Tag>) {
                vec.emplace_back(tag.default_value());
            } else {
                vec.emplace_back();
            }
        });
        // Important that this comes after the m_frozen_count check
        owning_identifier<Storage> index{static_cast<Storage&>(*this), size() - 1};
        // Update the pointer-to-row-number in m_indices so it refers to the
        // same thing as index
        m_indices.back() = static_cast<non_owning_identifier<Storage>>(index);
        return index;
    }

  public:
    /**
     * @brief Get a non-owning identifier to the offset-th entry.
     */
    [[nodiscard]] non_owning_identifier<Storage> at(std::size_t offset) {
        return {static_cast<Storage*>(this), m_indices.at(offset)};
    }

    /**
     * @brief Get the instance of the tag type Tag.
     */
    template <typename Tag>
    [[nodiscard]] constexpr Tag const& get_tag() const {
        return std::get<Tag>(m_tags);
    }

    template <typename Tag>
    static constexpr bool has_tag_v = detail::type_in_pack_v<Tag, Tags...>;

    /**
     * @brief Get the offset-th element of the column named by Tag.
     *
     * Because this is returning a single value, it is permitted even in
     * read-only mode. The container being in read only mode means that
     * operations that would invalidate iterators/pointers are forbidden, not
     * that actual data values cannot change.
     */
    template <typename Tag>
    [[nodiscard]] typename Tag::type& get(std::size_t offset) {
        static_assert(has_tag_v<Tag>);
        static_assert(!detail::has_num_instances_v<Tag>);
        return std::get<tag_index_v<Tag>>(m_data).at(offset);
    }

    /**
     * @brief Get the offset-th element of the column named by Tag.
     */
    template <typename Tag>
    [[nodiscard]] typename Tag::type const& get(std::size_t offset) const {
        static_assert(has_tag_v<Tag>);
        static_assert(!detail::has_num_instances_v<Tag>);
        return std::get<tag_index_v<Tag>>(m_data).at(offset);
    }

  private:
    // Helper that unifies different [non-]const 1/2-parameter versions of get_field_instance
    template <typename Tag, typename This>
    static decltype(auto) get_field_instance_helper(This& this_ref, std::size_t field_index) {
        static_assert(has_tag_v<Tag>);
        static_assert(detail::has_num_instances_v<Tag>);
        assert(field_index < this_ref.template get_tag<Tag>().num_instances());
        decltype(auto) vector_of_vectors = std::get<tag_index_v<Tag>>(this_ref.m_data);
        assert(vector_of_vectors.size() == this_ref.template get_tag<Tag>().num_instances());
        return vector_of_vectors[field_index];
    }

  public:
    /**
     * @brief Get the field_index-th instance of the column named by Tag.
     */
    template <typename Tag>
    std::vector<typename Tag::type>& get_field_instance(std::size_t field_index) {
        if (m_frozen_count) {
            throw_error("non-const get_field_instance(index) called on frozen structure");
        }
        return get_field_instance_helper<Tag>(*this, field_index);
    }

    template <typename Tag>
    std::vector<typename Tag::type> const& get_field_instance(std::size_t field_index) const {
        return get_field_instance_helper<Tag>(*this, field_index);
    }

    /**
     * @brief Get the offset-th element of the field_index-th instance of the column named by Tag.
     */
    template <typename Tag>
    typename Tag::type& get_field_instance(std::size_t field_index, std::size_t offset) {
        return get_field_instance_helper<Tag>(*this, field_index)[offset];
    }

    /**
     * @brief Get the offset-th element of the field_index-th instance of the column named by Tag.
     */
    template <typename Tag>
    typename Tag::type const& get_field_instance(std::size_t field_index,
                                                 std::size_t offset) const {
        return get_field_instance_helper<Tag>(*this, field_index)[offset];
    }

    /**
     * @brief Get the column container named by Tag.
     * @todo Consider deprecating and removing this as it exposes details of the
     *       underlying storage (e.g. which allocator is used for the vector).
     */
    template <typename Tag>
    [[nodiscard]] std::vector<typename Tag::type>& get() {
        static_assert(has_tag_v<Tag>);
        static_assert(!detail::has_num_instances_v<Tag>);
        if (m_frozen_count) {
            throw_error("non-const get() called on frozen structure");
        }
        return std::get<tag_index_v<Tag>>(m_data);
    }

    /**
     * @brief Get the column container named by Tag.
     * @todo Consider deprecating and removing this as it exposes details of the
     *       underlying storage (e.g. which allocator is used for the vector).
     */
    template <typename Tag>
    [[nodiscard]] std::vector<typename Tag::type> const& get() const {
        static_assert(has_tag_v<Tag>);
        static_assert(!detail::has_num_instances_v<Tag>);
        return std::get<tag_index_v<Tag>>(m_data);
    }

    /**
     * @brief Return a permutation-stable handle if ptr is inside us.
     * @todo Check const-correctness. Presumably a const version would return
     *       data_handle<T const>, which would hold a pointer-to-const for the
     *       container?
     */
    template <typename T>
    [[nodiscard]] neuron::container::data_handle<T> find_data_handle(T* ptr) {
        neuron::container::data_handle<T> handle{};
        // Don't go via non-const get<T>() because of the m_frozen_count check
        find_data_handle(handle, m_indices, ptr) ||
            (find_data_handle(handle, std::get<tag_index_v<Tags>>(m_data), ptr) || ...);
        return handle;
    }

  private:
    template <typename Tag, typename T>
    constexpr bool find_container_info(utils::storage_info& info,
                                       std::vector<T> const& cont1,
                                       void const* cont2,
                                       int field = -1) const {
        if (&cont1 != cont2) {
            return false;
        }
        if constexpr (detail::has_name_v<Storage>) {
            info.container = static_cast<Storage const&>(*this).name();
        }
        info.field = cxx_demangle(typeid(Tag).name());
        info.size = cont1.size();
        constexpr std::string_view prefix{"neuron::container::"};
        if (std::string_view{info.field}.substr(0, prefix.size()) == prefix) {
            info.field.erase(0, prefix.size());
            if (field >= 0) {
                info.field.append(1, '#');
                info.field.append(std::to_string(field));
            }
        }
        return true;
    }
    template <typename Tag, typename T>
    constexpr bool find_container_info(utils::storage_info& info,
                                       std::vector<std::vector<T>> const& conts,
                                       void const* cont2) const {
        for (auto field = 0; field < conts.size(); ++field) {
            if (find_container_info<Tag>(info, conts[field], cont2, field)) {
                return true;
            }
        }
        return false;
    }

  public:
    [[nodiscard]] std::optional<utils::storage_info> find_container_info(void const* cont) const {
        utils::storage_info info{};
        // FIXME: generate a proper tag type for the index column?
        if (find_container_info<non_owning_identifier<Storage>>(info, m_indices, cont) ||
            (find_container_info<Tags>(info, std::get<tag_index_v<Tags>>(m_data), cont) || ...)) {
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
                handle = neuron::container::data_handle<T>{at(row), container};
                assert(handle.refers_to_a_modern_data_structure());
                return true;
            } else {
                return false;
            }
        } else if constexpr (std::is_same_v<std::vector<T>, U>) {
            // Handle the case where U=std::vector<T> because we have a
            // runtime-variable number of copies of a column
            return std::any_of(container.begin(), container.end(), [&](auto& vec) {
                return find_data_handle(handle, vec, ptr);
            });
        } else {
            return false;
        }
    }

    [[noreturn]] void throw_error(std::string_view message) const {
        std::ostringstream oss;
        oss << cxx_demangle(typeid(Storage).name()) << "[frozen_count=" << m_frozen_count
            << ",sorted=" << std::boolalpha << m_sorted << "]: " << message;
        throw std::runtime_error(std::move(oss).str());
    }

    /**
     * @brief Flag for get_sorted_token(), mark_as_unsorted() and is_sorted().
     */
    bool m_sorted{false};

    /**
     * @brief Reference count for tokens guaranteeing the container is in frozen mode.
     *
     * In principle this would be called before multiple worker threads spin up,
     * but in practice as a transition measure then handles are acquired inside
     * worker threads.
     */
    std::atomic<std::size_t> m_frozen_count{};

    /**
     * @brief Pointers to identifiers that record the current physical row.
     */
    std::vector<non_owning_identifier_without_container> m_indices{};

    /**
     * @brief Storage type for this Tag.
     *
     * If the tag implements a num_instances() method then it is duplicated a
     * runtime-determined number of times and get_field_instance<Tag>(i) returns
     * the i-th element of the outer vector (of length num_instances()) of
     * vectors. If is no num_instances() method then the outer vector can be
     * omitted and get<Tag>() returns a vector of values.
     */
    template <typename Tag>
    using storage_t = std::conditional_t<detail::has_num_instances_v<Tag>,
                                         std::vector<std::vector<typename Tag::type>>,
                                         std::vector<typename Tag::type>>;

    /**
     * @brief Collection of data columns.
     */
    std::tuple<storage_t<Tags>...> m_data{};

    /**
     * @brief Callback that is invoked when the container becomes unsorted.
     */
    std::function<void()> m_unsorted_callback{};

    /**
     * @brief Instances of the tag types.
     */
    std::tuple<Tags...> m_tags{};
};
}  // namespace neuron::container
