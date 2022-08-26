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
 *
 *  @todo does this need to support the generic raw-pointer-wrapping
 *  compatibility mode of data_handle<T>?
 */
struct generic_data_handle {
    template <typename T>
    explicit generic_data_handle(data_handle<T> const& handle)
        : m_offset{handle.m_offset}
        , m_container{handle.m_container}
        , m_type{typeid(T)} {}

    template <typename T>
    explicit operator data_handle<T> const() {
        if (std::type_index{typeid(T)} != m_type) {
            throw std::runtime_error("Cannot convert generic_data_handle(" +
                                     cxx_demangle(m_type.name()) + ") to data_handle<" +
                                     cxx_demangle(typeid(T).name()) + ">");
        }
        return {m_offset, *static_cast<std::vector<T>*>(m_container)};
    }

  private:
    identifier_base m_offset{};
    // std::vector<T>* for the T encoded in m_type
    void* m_container{};
    // Reference to typeid(T) for the
    std::type_index m_type;
};

}  // namespace neuron::container