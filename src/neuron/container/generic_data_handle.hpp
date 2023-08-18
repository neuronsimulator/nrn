#pragma once
#include "backtrace_utils.h"
#include "neuron/container/data_handle.hpp"
#include "neuron/container/non_owning_soa_identifier.hpp"

#include <cstring>
#include <typeinfo>
#include <type_traits>

namespace neuron::container {
/**
 * @brief A variant type of small trivial types and a type-erased data_handle.
 *
 * This is a variant type for
 *   * type-erased data_handle<T>,
 *   * small (`sizeof(T) <= sizeof(void*)`), trivial types such as `int` or `float`,
 *   * and raw pointers.
 *
 * In this context raw pointer is a regular C pointer, i.e. just and address.
 * In contrast a `data_handle` refers to an entry in a `soa` container and is
 * stable w.r.t. sorting rows within the `soa` container.
 *
 * The type of the value stored in the `generic_data_handle` at runtime. This
 * enables type-checking and therefore type-safety at runtime. However, this
 * increases sizeof(generic_data_handle) by 50 percent. Hence, type-checking
 * should be considered useful for validation and debugging only; and may be
 * removed in the future in optimized builds.
 *
 * There are several states that instances of this class can be in:
 * - null, no value is contained, any type can be assigned without any type
 *   mismatch error (m_type=null, m_container=null, m_offset=null)
 * - wrapping an instance of a small, trivial type T (m_type=&typeid(T),
 *   m_container=the_value, m_offset=null), this includes raw pointers.
 * - wrapping a data handle<T> (m_type=&typeid(T*), m_container=the_container,
 *   m_offset=ptr_to_row)
 *
 * Note, the term "literal" doesn't refer to the common notion of literals in
 * C/C++, e.g. `1` or the string literal `"foo"`. Rather, it means a copy of
 * the value is stored bitwise inside the `generic_data_handle`.
 */
// TODO Consider whether this should be made more like std::any (with a maximum
//      2*sizeof(void*) and a promise never to allocate memory dynamically) so
//      it actually has a data_handle<T> subobject. Presumably that would mean
//      data_handle<T> would need to have a trivial destructor. This might make
//      it harder in future to have some vector_of_generic_data_handle type
//      that hoists out the pointer-to-container and typeid parts that should
//      be the same for all rows.
//
struct generic_data_handle {
  private:
    // Sufficiently small (<= sizeof(void*)) trivial types and pointers can
    // be stored literally. Note, that pointers can be stored as raw pointers,
    // in which case they're considered to be literal, or as data_handle in
    // which case they're not literal.
    template <typename T>
    static constexpr bool can_be_stored_literally_v =
        (std::is_trivial_v<T> && sizeof(T) <= sizeof(void*)) || std::is_pointer_v<T>;

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
    template <typename T,
              std::enable_if_t<!std::is_pointer_v<T> && can_be_stored_literally_v<T>, int> = 0>
    explicit generic_data_handle(T value) {
        literal_value<T>() = value;
    }

    /**
     * @brief Assign a small literal value to a generic data handle.
     *
     * This is important when generic_data_handle is used as the Datum type for "pointer" variables
     * in MOD files.
     */
    template <typename T,
              std::enable_if_t<!std::is_pointer_v<T> && can_be_stored_literally_v<T>, int> = 0>
    generic_data_handle& operator=(T value) {
        // assert_types_match<T>();
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
        // assert_types_match<T>();
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

    template <class T>
    generic_data_handle const& operator=(const data_handle<T>& handle) {
        // assert_types_match<T>();
        return *this = generic_data_handle(handle);
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
            assert(!m_container);
            return {};
        }
        assert_types_match<T*>();
        if (!refers_to_a_modern_data_structure()) {
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
     *  If the instance holds a value literally, that value is returned.
     *
     *  If the instance holds a `data_handle` then `generic_handle.get<T*>()`
     *  will return `static_cast<double*>(handle)`, i.e. the current address of
     *  the element the `data_handle` points to.
     *
     *  Note that the `generic_data_handle` remains valid if the rows of the
     *  `soa` are shuffled. In contrast to the pointer returned by `get` which
     *  is invalidated by operations that change the layout of rows in memory,
     *  such as shuffling or growing the underlying `soa`.
     *
     *  It might be interesting in future to explore dropping m_type in
     *  optimised builds, in which case we should aim to avoid predicating
     *  important logic on exceptions thrown by this function.
     */
    template <typename T>
    T get() const {
        if (refers_to_a_modern_data_structure()) {
            if constexpr (std::is_pointer_v<T>) {
                // If T=U* (i.e. T is a pointer type) then we might be in modern
                // mode, go via data_handle<U>
                return static_cast<T>(static_cast<data_handle<std::remove_pointer_t<T>>>(*this));
            }
            throw_error("For non-literal `generic_data_handle`s T must be a pointer, got " +
                        cxx_demangle(typeid(T).name()));
        } else {
            return literal_value<T>();
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

    /** @brief Obtain a reference to the literal value held by this handle.
     *
     *  Storing literal values is incompatible with storing data_handle<T>. If
     *  the handle stores data_handle<T> then calling this method throws an
     *  exception. If the handle is null, this sets the stored type to be T and
     *  returns a reference to it. If the handle already holds a literal value
     *  of type T then a reference to it is returned.
     *
     *  Note that, literal_value<double*>() will fail if the handle contains
     *  data_handle<double>, as in that case there is no persistent double*
     *  that could be referred to.
     *
     *  It might be interesting in future to explore dropping m_type in
     *  optimised builds, in which case we should aim to avoid predicating
     *  important logic on exceptions thrown by this function.
     */
    template <typename T>
    [[nodiscard]] T& literal_value() {
        assert_is_literal_value<T>();

        // This is a data handle in backwards-compatibility mode, wrapping a
        // raw pointer, or a null data handle. Using raw_ptr() on a typeless_null
        // (default-constructed) handle turns it into a legacy handle-to-T.
        if (!m_type) {
            m_type = &typeid(T);
        } else {
            assert_types_match<T>();
        }
        return *reinterpret_cast<T*>(&m_container);
    }

    template <typename T>
    [[nodiscard]] T const& literal_value() const {
        assert_is_literal_value<T>();
        assert_types_match<T>();

        return *reinterpret_cast<T const*>(&m_container);
    }

  private:
    /** @brief The demangled name of the value's type stored in this handle.
     *
     *  If the handle contains data_handle<T>, this will be T*. For a literal
     *  value of type `T` or raw pointer of type `T*` this returns the
     *  demangled type name if `T` and `T*`, respectively.
     */
    // It might be interesting in future to explore dropping m_type in
    // optimised builds. Hence, keep this method private.
    [[nodiscard]] std::string type_name() const {
        return m_type ? cxx_demangle(m_type->name()) : "typeless_null";
    }

    // Types match, if they're the same, or if the `generic_data_handle`
    // is a `nullptr`.
    template <class T>
    void assert_types_match() const {
        if (std::is_pointer_v<T> && !m_type && m_container == nullptr) {
            // `nullptr` doesn't need to be typed.
            return;
        }

        if (!holds<T>()) {
            throw_error(" incompatible type T = " + cxx_demangle(typeid(T).name()));
        }
    }

    template <class T>
    void assert_is_literal_value() const {
        static_assert(can_be_stored_literally_v<T>,
                      "generic_data_handle can only hold non-pointer types that are trivial "
                      "and no larger than a pointer");

        if (refers_to_a_modern_data_structure()) {
            throw_error("can't be used to store literal value, because it stores 'data_handle's.");
        }
    }

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
