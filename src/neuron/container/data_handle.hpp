#pragma once
#include "backtrace_utils.h"
#include "neuron/container/soa_identifier.hpp"
#include "neuron/model_data_fwd.hpp"

#include <ostream>
#include <vector>

namespace neuron::container {

/** @brief Stable handle to a generic value.
 *
 *  Without this type one can already hold a Node::handle `foo` and call
 *  something like `foo.v()` to get that Node's voltage in a way that is stable
 *  against permutations of the underlying data. The data_handle concept is
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
struct data_handle {
    data_handle() = default;

    /** @brief Construct a data_handle from a plain pointer.
     */
    data_handle(T* raw_ptr) {
        // Null pointer -> null handle.
        if (!raw_ptr) {
            return;
        }
        // First see if we can find a neuron::container that contains the current
        // value of `raw_ptr` and promote it into a container/handle pair. This is
        // ugly and inefficient; you should prefer using the other constructor.
        auto needle = utils::find_data_handle(raw_ptr);
        if (needle) {
            *this = std::move(needle);
            assert(m_container);
            assert(m_offset);
        } else {
            // If that didn't work, just save the plain pointer value. This is unsafe
            // and should be removed. It is purely meant as an intermediate step, if
            // you use it then the guarantees above will be broken.
            m_raw_ptr = raw_ptr;
        }
    }

    bool refers_to_a_modern_data_structure() const {
        return !m_raw_ptr;
    }

    data_handle(identifier_base offset, std::vector<T>& container)
        : m_offset{std::move(offset)}
        , m_container{&container} {
        assert(m_container);
        assert(m_offset);
    }

    explicit operator bool() const {
        return m_raw_ptr ? true : bool{m_offset};
    }

    /** Query whether this generic handle points to a value from the `Tag` field
     *  of the given container.
     */
    template <typename Tag, typename Container>
    bool refers_to(Container const& container) const {
        static_assert(Container::template has_tag_v<Tag>);
        if (m_raw_ptr) {
            return false;
        } else {
            return m_container == &(container.template get<Tag>()) &&
                   m_offset.current_row() < m_container->size();
        }
    }

    T& operator*() {
        if (m_raw_ptr) {
            return *m_raw_ptr;
        } else {
            assert(m_container);
            assert(m_offset);
            return m_container->at(m_offset.current_row());
        }
    }

    T const& operator*() const {
        if (m_raw_ptr) {
            return *m_raw_ptr;
        } else {
            assert(m_container);
            assert(m_offset);
            return m_container->at(m_offset.current_row());
        }
    }

    explicit operator T*() {
        if (m_raw_ptr) {
            return m_raw_ptr;
        } else if (!m_offset) {
            // Constructed with a null raw pointer, default constructed, or
            // refers to a row that was deleted
            return nullptr;
        } else {
            assert(m_container);
            assert(m_offset);
            return std::next(m_container->data(), m_offset.current_row());
        }
    }

    explicit operator T const *() const {
        if (m_raw_ptr) {
            return m_raw_ptr;
        } else if (!m_offset) {
            // Constructed with a null raw pointer, default constructed, or
            // refers to a row that was deleted
            return nullptr;
        } else {
            assert(m_container);
            assert(m_offset);
            return std::next(m_container->data(), m_offset.current_row());
        }
    }

    friend std::ostream& operator<<(std::ostream& os, data_handle const& dh) {
        os << "data_handle<" << cxx_demangle(typeid(T).name()) << ">{";
        if (dh.m_container) {
            os << "cont=" << utils::find_container_name(*dh.m_container) << " " << dh.m_offset
               << '/' << dh.m_container->size();
        } else {
            os << "raw=" << dh.m_raw_ptr;
        }
        return os << '}';
    }

    friend bool operator==(data_handle const& lhs, data_handle const& rhs) {
        return lhs.m_offset == rhs.m_offset && lhs.m_container == rhs.m_container &&
               lhs.m_raw_ptr == rhs.m_raw_ptr;
    }

    friend bool operator!=(data_handle const& lhs, data_handle const& rhs) {
        return !(lhs == rhs);
    }

  private:
    friend struct std::hash<data_handle>;
    identifier_base m_offset{};
    // This should be std::reference_wrapper and never null, only use a plain
    // pointer because of the compatibility mode that wraps a raw pointer.
    std::vector<T>* m_container{};
    // std::reference_wrapper<std::vector<T>> m_container;
    T* m_raw_ptr{};
};

}  // namespace neuron::container

// Enable data_handle<T> as a key type in std::unordered_map
template <typename T>
struct std::hash<neuron::container::data_handle<T>> {
    std::size_t operator()(neuron::container::data_handle<T> const& s) const noexcept {
        static_assert(sizeof(std::size_t) == sizeof(T const*));
        if (s.m_raw_ptr) {
            return reinterpret_cast<std::size_t>(s.m_raw_ptr);
        } else {
            // The hash should not include the current row number, but rather the
            // std::size_t* that is dereferenced to *get* the current row number,
            // and which container this generic value lives in.
            return std::hash<neuron::container::identifier_base>{}(s.m_offset) ^
                   reinterpret_cast<std::size_t>(s.m_container);
        }
    }
};
