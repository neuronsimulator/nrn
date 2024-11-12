#pragma once
#include "backtrace_utils.h"
#include "neuron/container/data_handle.hpp"
#include "neuron/container/non_owning_soa_identifier.hpp"

#include <cstddef>
#include <cstring>
#include <typeinfo>
#include <type_traits>

namespace neuron::container {
/**
 * @brief Non-template stable handle to a generic value.
 *
 * This is a type-erased version of data_handle<T>, with the additional feature
 * that it can store values of POD types no larger than a pointer (typically int
 * and float). It stores (at runtime) the type of the value it contains, and is
 * therefore type-safe, but this increases sizeof(generic_data_handle) by 50% so
 * it may be prudent to view this as useful for validation/debugging but not
 * something to become too dependent on.
 *
 * There are several states that instances of this class can be in:
 * - null, no value is contained, any type can be assigned without any type
 *   mismatch error (m_type=null, m_container=null, m_offset=null)
 * - wrapping an instance of a small, trivial type T (m_type=&typeid(T),
 *   m_container=the_value, m_offset=null)
 * - wrapping a data handle<T> (m_type=&typeid(T*), m_container=the_container,
 *   m_offset=ptr_to_row)
 *
 * @todo Consider whether this should be made more like std::any (with a maximum
 *       2*sizeof(void*) and a promise never to allocate memory dynamically) so
 *       it actually has a data_handle<T> subobject. Presumably that would mean
 *       data_handle<T> would need to have a trivial destructor. This might make
 *       it harder in future to have some vector_of_generic_data_handle type
 *       that hoists out the pointer-to-container and typeid parts that should
 *       be the same for all rows.
 */
struct generic_data_handle {
  private:
    // The exact criteria could be refined, it is definitely not possible to
    // store types with non-trivial destructors.
    template <typename T>
    static constexpr bool can_be_stored_literally_v =
        std::is_trivial_v<T> && !std::is_pointer_v<T> && sizeof(T) <= sizeof(void*);

  public:
    /** @brief Construct a null data handle.
     */
    generic_data_handle() = default;

    /** @brief Construct a null data handle.
     */
    generic_data_handle(std::nullptr_t) {}

    /**
     * @brief Construct a generic data handle that holds a small literal value.
     *
     * This is explicit to avoid things like operator<<(ostream&, generic_data_handle const&) being
     * considered when printing values like size_t.
     */
    template <typename T, std::enable_if_t<can_be_stored_literally_v<T>, int> = 0>
    explicit generic_data_handle(T value)
        : m_type{&typeid(T)} {
        std::memcpy(&m_container, &value, sizeof(T));
    }

    /**
     * @brief Assign a small literal value to a generic data handle.
     *
     * This is important when generic_data_handle is used as the Datum type for "pointer" variables
     * in MOD files.
     */
    template <typename T, std::enable_if_t<can_be_stored_literally_v<T>, int> = 0>
    generic_data_handle& operator=(T value) {
        return *this = generic_data_handle{value};
    }

    /**
     * @brief Store a pointer inside this generic data handle.
     *
     * Explicit handling of pointer types (with redirection via data_handle<T>) ensures that
     * `some_generic_handle = some_ptr_to_T` promotes the raw `T*` to a modern `data_handle<T>` that
     * is stable to permutation.
     */
    template <typename T, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
    generic_data_handle& operator=(T value) {
        return *this = generic_data_handle{data_handle<std::remove_pointer_t<T>>{value}};
    }

    template <typename T>
    generic_data_handle(do_not_search_t dns, T* raw_ptr)
        : generic_data_handle{data_handle<T>{dns, raw_ptr}} {}

    /**
     * @brief Wrap a data_handle<void> in a generic data handle.
     *
     * Note that data_handle<void> is always wrapping a raw void* and is never
     * in "modern" mode
     */
    template <typename T, std::enable_if_t<std::is_same_v<T, void>, int> = 0>
    generic_data_handle(data_handle<T> const& handle)
        : m_container{handle.m_raw_ptr}
        , m_type{&typeid(T*)} {
        static_assert(std::is_same_v<T, void>);
    }

    /**
     * @brief Wrap a data_handle<T != void> in a generic data handle.
     */
    template <typename T, std::enable_if_t<!std::is_same_v<T, void>, int> = 0>
    generic_data_handle(data_handle<T> const& handle)
        : m_offset{handle.m_offset}
        , m_container{handle.m_container_or_raw_ptr}
        , m_type{&typeid(T*)}
        , m_array_dim{handle.m_array_dim}
        , m_array_index{handle.m_array_index} {
        static_assert(!std::is_same_v<T, void>);
    }

    /**
     * @brief Create data_handle<T> from a generic data handle.
     *
     * The conversion will succeed, yielding a null data_handle<T>, if the
     * generic data handle is null. If the generic data handle is not null then
     * the conversion will succeed if the generic data handle actually holds a
     * data_handle<T> or a literal T*.
     *
     * It might be interesting in future to explore dropping m_type in optimised
     * builds, in which case we should aim to avoid predicating important logic
     * on exceptions thrown by this function.
     */
    template <typename T>
    explicit operator data_handle<T>() const {
        // Either the type has to match or the generic_data_handle needs to have
        // never been given a type
        if (!m_type) {
            // A (typeless / default-constructed) null generic_data_handle can
            // be converted to any (null) data_handle<T>.
            return {};
        }
        if (typeid(T*) != *m_type) {
            throw_error(" cannot be converted to data_handle<" + cxx_demangle(typeid(T).name()) +
                        ">");
        }
        if (m_offset.has_always_been_null()) {
            // This is a data handle in backwards-compatibility mode, wrapping a
            // raw pointer of type T*, or a null handle that has always been null (as opposed to a
            // handle that became null). Passing do_not_search prevents the data_handle<T>
            // constructor from trying to find the raw pointer in the NEURON data structures.
            return {do_not_search, static_cast<T*>(m_container)};
        }
        // Nothing after here is reachable with T=void, as data_handle<void> is always either null
        // or in backwards-compatibility mode. the branch structure has been chosen to try and
        // minimise compiler warnings and maximise reported coverage...
        if constexpr (!std::is_same_v<T, void>) {
            if (!m_offset.was_once_valid()) {
                // A real and still-valid data handle. This cannot be instantiated with T=void
                // because data_handle<void> does not have a 4-argument constructor.
                assert(m_container);
                return {m_offset, static_cast<T* const*>(m_container), m_array_dim, m_array_index};
            }
        }
        // Reaching here should mean T != void && was_once_valid == true, i.e. this used to be a
        // valid data handle, but it has since been invalidated. Invalid data handles never become
        // valid again, so we can safely produce a "fully null" data_handle<T>.
        return {};
    }

    /** @brief Explicit conversion to any T.
     *
     *  It might be interesting in future to explore dropping m_type in
     *  optimised builds, in which case we should aim to avoid predicating
     *  important logic on exceptions thrown by this function.
     *
     *  Something like static_cast<double*>(generic_handle) will work both if
     *  the Datum holds a literal double* and if it holds a data_handle<double>.
     *
     *  @todo Consider conversion to bool and whether this means not-null or to
     *  obtain a literal, wrapped bool value
     */
    template <typename T>
    T get() const {
        if constexpr (std::is_pointer_v<T>) {
            // If T=U* (i.e. T is a pointer type) then we might be in modern
            // mode, go via data_handle<U>
            return static_cast<T>(static_cast<data_handle<std::remove_pointer_t<T>>>(*this));
        } else {
            // Getting a literal value saved in m_container
            static_assert(can_be_stored_literally_v<T>,
                          "generic_data_handle can only hold non-pointer types that are trivial "
                          "and smaller than a pointer");
            if (!m_offset.has_always_been_null()) {
                throw_error(" conversion to " + cxx_demangle(typeid(T).name()) +
                            " not possible for a handle [that was] in modern mode");
            }
            if (typeid(T) != *m_type) {
                throw_error(" does not hold a literal value of type " +
                            cxx_demangle(typeid(T).name()));
            }
            T ret{};
            std::memcpy(&ret, &m_container, sizeof(T));
            return ret;
        }
    }

    // Defined elsewhere to optimise compile times.
    friend std::ostream& operator<<(std::ostream& os, generic_data_handle const& dh);

    /** @brief Check if this handle refers to the specific type.
     *
     *  This could be a small, trivial type (e.g. T=int) or a pointer type (e.g.
     *  T=double*). holds<double*>() == true could either indicate that a
     *  data_handle<double> is held or that a literal double* is held.
     *
     *  It might be interesting in future to explore dropping m_type in
     *  optimised builds, in which case we should aim to avoid predicating
     *  important logic on this function.
     */
    template <typename T>
    [[nodiscard]] bool holds() const {
        return m_type && typeid(T) == *m_type;
    }

    /** @brief Check if this handle contains a data_handle<T> or just a literal.
     *
     *  This should match
     *  static_cast<data_handle<T>>(generic_handle).refers_to_a_modern_data_structure()
     *  if T is correct. This will return true if we hold a data_handle that
     *  refers to a since-deleted row.
     */
    [[nodiscard]] bool refers_to_a_modern_data_structure() const {
        return !m_offset.has_always_been_null();
    }

    /** @brief Return the demangled name of the type this handle refers to.
     *
     *  If the handle contains data_handle<T>, this will be T*. If a literal
     *  value or raw pointer is being wrapped, that possibly-pointer type will
     *  be returned.
     *
     *  It might be interesting in future to explore dropping m_type in
     *  optimised builds, in which case we should aim to avoid predicating
     *  important logic on this function.
     */
    [[nodiscard]] std::string type_name() const {
        return m_type ? cxx_demangle(m_type->name()) : "typeless_null";
    }

    /** @brief Obtain a reference to the literal value held by this handle.
     *
     *  Storing literal values is incompatible with storing data_handle<T>. If
     *  the handle stores data_handle<T> then calling this method throws an
     *  exception. If the handle is null, this sets the stored type to be T and
     *  returns a reference to it. If the handle already holds a literal value
     *  of type T then a reference to it is returned.
     *
     *  Note that, unlike converting to double*, literal_value<double*>() will
     *  fail if the handle contains data_handle<double>, as in that case there
     *  is no persistent double* that could be referred to.
     *
     *  It might be interesting in future to explore dropping m_type in
     *  optimised builds, in which case we should aim to avoid predicating
     *  important logic on exceptions thrown by this function.
     */
    template <typename T>
    [[nodiscard]] T& literal_value() {
        if (!m_offset.has_always_been_null()) {
            throw_error("::literal_value<" + cxx_demangle(typeid(T).name()) +
                        "> cannot be called on a handle [that was] in modern mode");
        } else {
            // This is a data handle in backwards-compatibility mode, wrapping a
            // raw pointer, or a null data handle. Using raw_ptr() on a typeless_null
            // (default-constructed) handle turns it into a legacy handle-to-T.
            if (!m_type) {
                m_type = &typeid(T);
            } else if (typeid(T) != *m_type) {
                throw_error(" does not hold a literal value of type " +
                            cxx_demangle(typeid(T).name()));
            }
            return *reinterpret_cast<T*>(&m_container);
        }
    }

    /**
     * @brief Is the generic data handle a data_handle and invalid?
     *
     * If it contains a value (convertible to) a `data_handle<double>`, then
     * it's valid if and only if it's a valid `data_handle<double>`.
     *
     * If it doesn't contain a data handle, return false.
     */
    bool is_invalid_handle() const {
        if (!m_type) {
            // Empty default initialized.
            return true;
        }

        return holds<double*>() ? !bool(data_handle<double>(*this)) : false;
    }

  private:
    [[noreturn]] void throw_error(std::string message) const {
        std::ostringstream oss{};
        oss << *this << std::move(message);
        throw std::runtime_error(std::move(oss).str());
    }
    // Offset into the underlying storage container. If this handle is holding a
    // literal value, such as a raw pointer, then this will be null.
    non_owning_identifier_without_container m_offset{};
    // T* const* for the T encoded in m_type if m_offset is non-null,
    // otherwise a literal value is stored in this space.
    void* m_container{};
    // Pointer to typeid(T) for the wrapped type
    std::type_info const* m_type{};
    // Extra information required for data_handle<T> to point at array variables
    int m_array_dim{1}, m_array_index{};
};

namespace utils {
namespace detail {
/**
 * @brief Try and promote a generic_data_handle wrapping a raw pointer.
 *
 * If the raw pointer can be found in the model data structures, a "modern" permutation-stable
 * handle to it will be returned. If it cannot be found, a null generic_data_handle will be
 * returned.
 */
[[nodiscard]] generic_data_handle promote_or_clear(generic_data_handle);
}  // namespace detail
// forward declared in model_data_fwd.hpp
template <typename T>
[[nodiscard]] data_handle<T> find_data_handle(T* ptr) {
    return static_cast<data_handle<T>>(detail::promote_or_clear({do_not_search, ptr}));
}
}  // namespace utils
}  // namespace neuron::container
