#pragma once
#include "neuron/container/soa_identifier.hpp"

#include <vector>

namespace neuron::container {

/** @brief Stable handle to a generic value.
 *
 *  Without this type one can already hold a Node::handle `foo` and call
 *  something like `foo.v()` to get that Node's voltage in a way that is stable
 *  against permutations of the underlying data. The generic_handle concept is
 *  intended to be used if we want to erase the detail that the quantity is a
 *  voltage, or that it belongs to a Node, and simply treat it as a
 *  floating-point value that we may want to dereference later -- essentially a
 *  substitute for double*.
 *
 *  Implementation: like Node::handle we can store a std::size_t* that we
 *  dereference to find out either:
 *  - the current offset in the underlying container, or
 *  - that the object being referred to (e.g. a Node) no longer exists
 *
 *  Assuming nothing has been invalidated, we have an offset, a type, and the
 *  fundamental assumption behind all of this that the underlying data are
 *  contiguous -- we "just" need to know the address of the start of the
 *  underlying storage vector without adding specific type information (like
 *  Node) to this class. The simplest way of doing this is to assume that the
 *  underlying storage is always std::vector<T> (or a custom allocator that is
 *  always the same type in neuron::container::*). Note that storing T* or
 *  span<T> would not work if the underlying storage is reallocated.
 */
template <typename T>
struct generic_handle {
    generic_handle() = default;
    /** @brief Construct a dummy generic_handle that wraps a plain pointer.
     *  @todo Fix all usage of this and then remove it. It is purely meant as an
     *  intermediate step, if you use it then the guarantees above will be broken.
     */
    generic_handle(T* raw_ptr)
        : m_raw_ptr{raw_ptr} {}
    generic_handle(ElementHandle offset, std::vector<T>& container)
        : m_offset{std::move(offset)}
        , m_container{&container} {}

    explicit operator bool() const {
        return m_raw_ptr ? static_cast<bool>(m_raw_ptr) : bool{m_offset};
    }

    T& operator*() {
        if (m_raw_ptr) {
            assert(m_raw_ptr);
            return *m_raw_ptr;
        } else {
            assert(m_container && m_offset);
            return m_container->at(m_offset.current_row());
        }
    }

    T const& operator*() const {
        if (m_raw_ptr) {
            assert(m_raw_ptr);
            return *m_raw_ptr;
        } else {
            assert(m_container && m_offset);
            return m_container->at(m_offset.current_row());
        }
    }

    explicit operator T*() {
        if (m_raw_ptr) {
            return m_raw_ptr;
        } else {
            assert(m_container && m_offset);
            return std::next(m_container->data(), m_offset.current_row());
        }
    }

    explicit operator T const *() const {
        if (m_raw_ptr) {
            return m_raw_ptr;
        } else {
            assert(m_container && m_offset);
            return std::next(m_container->data(), m_offset.current_row());
        }
    }

    friend bool operator==(generic_handle const& lhs, generic_handle const& rhs) {
        return lhs.m_offset == rhs.m_offset && lhs.m_container == rhs.m_container &&
               lhs.m_raw_ptr == rhs.m_raw_ptr;
    }

    friend bool operator!=(generic_handle const& lhs, generic_handle const& rhs) {
        return !(lhs == rhs);
    }

  private:
    friend struct std::hash<generic_handle>;
    ElementHandle m_offset{};
    // This should be std::reference_wrapper and never null, only use a plain
    // pointer because of the compatibility mode that wraps a raw pointer.
    std::vector<T>* m_container{};
    // std::reference_wrapper<std::vector<T>> m_container;
    T* m_raw_ptr{};
};

}  // namespace neuron::container

// Enable generic_handle<T> as a key type in std::unordered_map
template <typename T>
struct std::hash<neuron::container::generic_handle<T>> {
    std::size_t operator()(neuron::container::generic_handle<T> const& s) const noexcept {
        static_assert(sizeof(std::size_t) == sizeof(T const*));
        if (s.m_raw_ptr) {
            return reinterpret_cast<std::size_t>(s.m_raw_ptr);
        } else {
            // The hash should not include the current row number, but rather the
            // std::size_t* that is dereferenced to *get* the current row number,
            // and which container this generic value lives in.
            return std::hash<ElementHandle>{}(s.m_offset) ^
                   reinterpret_cast<std::size_t>(s.m_container);
        }
    }
};
