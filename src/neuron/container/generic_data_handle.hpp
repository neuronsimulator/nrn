#pragma once
#include "backtrace_utils.h"
#include "neuron/container/soa_identifier.hpp"

#include <typeindex>
#include <vector>

namespace neuron::container {

template <typename>
struct data_handle;

/** @brief Non-template stable handle to a generic value.
 *
 *  This is a type-erased version of data_handle<T>.
 */
struct generic_data_handle {
    template <typename T>
    explicit generic_data_handle(data_handle<T> const& handle)
        : m_offset{handle.m_raw_ptr ? nullptr : handle.m_offset.m_ptr}
        , m_container{handle.m_raw_ptr ? static_cast<void*>(handle.m_raw_ptr)
                                       : static_cast<void*>(handle.m_container)}
        , m_type{typeid(T)} {}

    template <typename T>
    explicit operator data_handle<T> const() {
        if (std::type_index{typeid(T)} != m_type) {
            throw std::runtime_error("Cannot convert generic_data_handle(" +
                                     cxx_demangle(m_type.name()) + ") to data_handle<" +
                                     cxx_demangle(typeid(T).name()) + ">");
        }
        if (m_offset) {
            // A real and still-valid data handle
            return {m_offset, *static_cast<std::vector<T>*>(m_container)};
        } else if (m_offset.m_ptr) {
            // This used to be a valid data handle, but it has since been
            // invalidated. Invalid data handles never become valid again.
            return {};
        } else {
            // This is a data handle in backwards-compatibility mode, wrapping a
            // raw pointer, or a null data handle.
            return {static_cast<T*>(m_container)};
        }
    }

    [[nodiscard]] std::string type_name() const {
        return cxx_demangle(m_type.name());
    }

  private:
    // Offset into the underlying storage container. If this generic_data_handle
    // refers to a data_handle<T*> in backwards-compatibility mode, where it
    // just wraps a plain T*, then m_offset is null.
    identifier_base m_offset{};
    // std::vector<T>* for the T encoded in m_type if m_offset is non-null,
    // otherwise T*
    void* m_container{};
    // Reference to typeid(T) for the
    std::type_index m_type;
};

}  // namespace neuron::container