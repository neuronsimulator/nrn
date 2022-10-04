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
 *  int and float).
 */
struct generic_data_handle {
  private:
    template <typename T>
    static constexpr bool can_be_stored_literally_v = std::is_trivial_v<T> &&
                                                      sizeof(T) <= sizeof(void*);

  public:
    generic_data_handle() = default;
    generic_data_handle(std::nullptr_t) {}

    template <typename T, std::enable_if_t<can_be_stored_literally_v<T>, int> = 0>
    generic_data_handle(T value)
        : m_type{typeid(T)} {
        std::memcpy(&m_container, &value, sizeof(T));
    }

    // Avoid trying to access members of the data_handle<void> specialisation.
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

    // Avoid trying to access members of the data_handle<void> specialisation.
    template <typename T, std::enable_if_t<!std::is_same_v<T, void>, int> = 0>
    generic_data_handle(data_handle<T> const& handle)
        : m_offset{handle.m_raw_ptr ? nullptr : handle.m_offset.m_ptr}
        , m_container{handle.m_raw_ptr ? static_cast<void*>(handle.m_raw_ptr)
                                       : static_cast<void*>(handle.m_container)}
        , m_type{typeid(T*)} {
        static_assert(!std::is_same_v<T, void>);
    }

    template <typename T>
    explicit operator data_handle<T>() const {
        bool const is_typeless_null = holds<typeless_null>();
        if (!is_typeless_null && std::type_index{typeid(T*)} != m_type) {
            throw_error(" cannot be converted to data_handle<" + cxx_demangle(typeid(T*).name()) +
                        ">");
        }
        if (m_offset) {
            assert(!is_typeless_null);
            if constexpr (std::is_same_v<T, void>) {
                throw_error(" should not be in modern mode because it is referring to void");
            } else {
                // A real and still-valid data handle
                assert(m_container);
                return {m_offset, *static_cast<std::vector<T>*>(m_container)};
            }
        } else if (m_offset.m_ptr) {
            assert(!is_typeless_null);
            // This used to be a valid data handle, but it has since been
            // invalidated. Invalid data handles never become valid again.
            return {};
        } else {
            // This is a data handle in backwards-compatibility mode, wrapping a
            // raw pointer, or a null data handle. Passing do_not_search
            // prevents the data_handle<T> constructor from trying to find the
            // raw pointer in the NEURON data structures.
            return {do_not_search, static_cast<T*>(m_container)};
        }
    }

    explicit operator bool() const {
        // Branches follow the data_handle<T> conversion just above
        return bool{m_offset} ? true : (m_offset.m_ptr ? false : static_cast<bool>(m_container));
    }

    /** @brief Explicit conversion to any T.
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

    /** @brief Reset the generic_data_handle to typeless null state.
     */
    generic_data_handle& operator=(std::nullptr_t) {
        return (*this = generic_data_handle{});
    }

    template <typename T>
    generic_data_handle& operator=(data_handle<T> const& rhs) {
        return (*this = static_cast<generic_data_handle>(rhs));
    }

    template <typename T>
    generic_data_handle& operator=(T* ptr) {
        if (ptr) {
            return (*this = data_handle<T>{ptr});
        } else {
            return (*this = nullptr);
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

    template <typename T>
    [[nodiscard]] bool holds() const {
        return std::type_index{typeid(T)} == m_type;
    }

    [[nodiscard]] std::string type_name() const {
        return cxx_demangle(m_type.name());
    }

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
    // Offset into the underlying storage container. If this generic_data_handle
    // refers to a data_handle<T*> in backwards-compatibility mode, where it
    // just wraps a plain T*, then m_offset is null.
    identifier_base m_offset{};
    // std::vector<T>* for the T encoded in m_type if m_offset is non-null,
    // otherwise T*
    void* m_container{};
    // Reference to typeid(T) for the wrapped type
    std::type_index m_type{typeid(typeless_null)};
};

template <typename T>
T get(generic_data_handle const& gh) {
    return static_cast<T>(gh);
}

template <typename T>
T& get_ref(generic_data_handle& gh) {
    return gh.literal_value<T>();
}

}  // namespace neuron::container