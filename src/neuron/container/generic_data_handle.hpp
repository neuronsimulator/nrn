#pragma once
#include "backtrace_utils.h"
#include "neuron/container/data_handle.hpp"
#include "neuron/container/soa_identifier.hpp"
#include "neuron/model_data_fwd.hpp"

#include <typeindex>
#include <type_traits>
#include <vector>

namespace neuron::container {

template <typename>
struct data_handle;

/** @brief Helper type used to flag when the generic_data_handle contains a null value.
 */
struct typeless_null {};

/** @brief Non-template stable handle to a generic value.
 *
 *  This is a type-erased version of data_handle<T>, with the additional feature
 *  that it can store values of POD types no larger than a pointer (typically
 *  int and float). It stores (at runtime) the type of the value it contains,
 *  and is therefore type-safe, but this increases sizeof(generic_data_handle)
 *  by 50% so it may be prudent to view this as useful for validation/debugging
 *  but not something to become too dependent on.
 */
struct generic_data_handle {
  private:
    // The exact criteria could be refined, it is definitely not possible to
    // store types with non-trivial destructors.
    template <typename T>
    static constexpr bool can_be_stored_literally_v = std::is_trivial_v<T> &&
                                                      sizeof(T) <= sizeof(void*);

  public:
    /** @brief Construct a null data handle.
     */
    generic_data_handle() = default;

    /** @brief Construct a null data handle.
     */
    generic_data_handle(std::nullptr_t) {}

    /** @brief Construct a generic data handle that holds a small literal value.
     */
    template <typename T, std::enable_if_t<can_be_stored_literally_v<T>, int> = 0>
    generic_data_handle(T value)
        : m_type{typeid(T)} {
        std::memcpy(&m_container, &value, sizeof(T));
    }

    /** @brief Wrap a data_handle<void> in a generic data handle.
     */
    template <typename T, std::enable_if_t<std::is_same_v<T, void>, int> = 0>
    generic_data_handle(data_handle<T> const& handle)
        : m_container{handle.m_raw_ptr}
        , m_type{typeid(T*)} {
        static_assert(std::is_same_v<T, void>);
        // data_handle<void> cannot refer to a modern data structure, it can
        // only wrap a void*
        assert(!m_offset);
        assert(!m_offset.m_ptr);
    }

    /** @brief Wrap a data_handle<T != void> in a generic data handle.
     */
    template <typename T, std::enable_if_t<!std::is_same_v<T, void>, int> = 0>
    generic_data_handle(data_handle<T> const& handle)
        : m_offset{handle.m_raw_ptr ? nullptr : handle.m_offset.m_ptr}
        , m_container{handle.m_raw_ptr ? static_cast<void*>(handle.m_raw_ptr)
                                       : static_cast<void*>(handle.m_container)}
        , m_type{typeid(T*)} {
        static_assert(!std::is_same_v<T, void>);
    }

    /** @brief Create data_handle<T> from a generic data handle.
     *
     *  The conversion will succeed, yielding a null data_handle<T>, if the
     *  generic data handle is null. If the generic data handle is not null then
     *  the conversion will succeed if the generic data handle actually holds a
     *  data_handle<T> or a literal T*.
     *
     *  It might be interesting in future to explore dropping m_type in
     *  optimised builds, in which case we should aim to avoid predicating
     *  important logic on exceptions thrown by this function.
     */
    template <typename T>
    explicit operator data_handle<T>() const {
        // Either the type has to match or the generic_data_handle needs to have
        // never been given a type
        if (holds<typeless_null>()) {
            // A (typeless / default-constructed) null generic_data_handle can
            // be converted to any (null) data_handle<T>.
            return {};
        }
        if (std::type_index{typeid(T*)} != m_type) {
            throw_error(" cannot be converted to data_handle<" + cxx_demangle(typeid(T).name()) +
                        ">");
        }
        if (m_offset) {
            if constexpr (std::is_same_v<T, void>) {
                // This branch is needed to avoid forming references-to-void if
                // T=void.
                throw_error(" should not be in modern mode because it is referring to void");
            } else {
                // A real and still-valid data handle
                assert(m_container);
                return {m_offset, *static_cast<std::vector<T>*>(m_container)};
            }
        } else if (m_offset.m_ptr) {
            // This used to be a valid data handle, but it has since been
            // invalidated. Invalid data handles never become valid again, so we
            // can safely produce a "fully null" data_handle<T>.
            return {};
        } else {
            // This is a data handle in backwards-compatibility mode, wrapping a
            // raw pointer of type T*. Passing do_not_search prevents the
            // data_handle<T> constructor from trying to find the raw pointer in
            // the NEURON data structures.
            return {do_not_search, static_cast<T*>(m_container)};
        }
    }

    /** @brief Explicit conversion to any T.
     *
     *  It might be interesting in future to explore dropping m_type in
     *  optimised builds, in which case we should aim to avoid predicating
     *  important logic on exceptions thrown by this function.
     */
    template <typename T>
    explicit operator T() const {
        if constexpr (std::is_pointer_v<T>) {
            // If T=U* (i.e. T is a pointer type) then we might be in modern
            // mode, go via data_handle<U>
            return static_cast<T>(static_cast<data_handle<std::remove_pointer_t<T>>>(*this));
        } else {
            // Getting a literal value saved in m_container
            static_assert(can_be_stored_literally_v<T>,
                          "generic_data_handle can only hold non-pointer types that are trivial "
                          "and smaller than a pointer");
            if (m_offset || m_offset.m_ptr) {
                throw_error(" conversion to " + cxx_demangle(typeid(T).name()) +
                            " not possible for a handle [that was] in modern mode");
            }
            if (std::type_index{typeid(T)} != m_type) {
                throw_error(" does not hold a literal value of type " +
                            cxx_demangle(typeid(T).name()));
            }
            T ret{};
            std::memcpy(&ret, &m_container, sizeof(T));
            return ret;
        }
    }

    friend std::ostream& operator<<(std::ostream& os, generic_data_handle const& dh) {
        auto const maybe_info = utils::find_container_info(dh.m_container);
        os << "generic_data_handle{";
        if (maybe_info) {
            os << "cont=" << maybe_info->name << ' ' << dh.m_offset << '/' << maybe_info->size;
        } else {
            os << "raw=";
            if (dh.m_container) {
                // This shouldn't crash, but it might contain some garbage if
                // we're wrapping a literal value
                os << dh.m_container;
            } else {
                os << "nullptr";
            }
        }
        return os << ", type=" << cxx_demangle(dh.m_type.name()) << '}';
    }

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
        return std::type_index{typeid(T)} == m_type;
    }

    /** @brief Check if this handle contains a data_handle<T> or just a literal.
     *
     *  This should match
     *  static_cast<data_handle<T>>(generic_handle).refers_to_a_modern_data_structure()
     *  if T is correct. This will return true if we hold a data_handle that
     *  refers to a since-deleted row.
     */
    [[nodiscard]] bool refers_to_a_modern_data_structure() const {
        return m_offset || m_offset.m_ptr;
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
        return cxx_demangle(m_type.name());
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
        if (m_offset || m_offset.m_ptr) {
            throw_error("::literal_value<" + cxx_demangle(typeid(T).name()) +
                        "> cannot be called on a handle [that was] in modern mode");
        } else {
            // This is a data handle in backwards-compatibility mode, wrapping a
            // raw pointer, or a null data handle.
            bool const is_typeless_null = holds<typeless_null>();
            // Using raw_ptr() on a typeless_null (default-constructed) handle
            // turns it into a legacy handle-to-T
            if (is_typeless_null) {
                m_type = typeid(T);
            } else if (std::type_index{typeid(T)} != m_type) {
                throw_error(" does not hold a literal value of type " +
                            cxx_demangle(typeid(T).name()));
            }
            return *reinterpret_cast<T*>(&m_container);  // Eww
        }
    }

  private:
    [[noreturn]] void throw_error(std::string message) const {
        std::ostringstream oss{};
        oss << *this << std::move(message);
        throw std::runtime_error(std::move(oss).str());
    }
    // Offset into the underlying storage container. If this handle is holding a
    // literal value, such as a raw pointer, then this will be null.
    identifier_base m_offset{};
    // std::vector<T>* for the T encoded in m_type if m_offset is non-null,
    // otherwise a literal value is stored in this space.
    void* m_container{};
    // Reference to typeid(T) for the wrapped type
    std::type_index m_type{typeid(typeless_null)};
};

template <typename T>
T get(generic_data_handle const& gh) {
    return static_cast<T>(gh);
}

}  // namespace neuron::container