#pragma once
#include "backtrace_utils.h"
#include "neuron/container/non_owning_soa_identifier.hpp"
#include "neuron/model_data_fwd.hpp"

#include <ostream>
#include <sstream>

namespace neuron::container {
struct do_not_search_t {};
inline constexpr do_not_search_t do_not_search{};

namespace detail {
// 3rd argument ensures the no-op implementation has lower precedence than the one that prints val.
// This implementation avoids passing a T through ... in the no-output-operator case, which is
// important if T is a non-trivial type without an output operator.
template <typename T>
std::ostream& print_value_impl(std::ostream& os, T const&, ...) {
    return os;
}
template <typename T>
auto print_value_impl(std::ostream& os, T const& val, std::nullptr_t) -> decltype(os << val) {
    return os << " val=" << val;
}
template <typename T>
std::ostream& print_value(std::ostream& os, T const& val) {
    return print_value_impl(os, val, nullptr);
}
}  // namespace detail

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
 *
 *  @todo Const correctness -- data_handle should be like span:
 *  data_handle<double> can read + write the value, data_handle<double const>
 *  can only read the value. const applied to the data_handle itself should just
 *  control whether or not it can be rebound to refer elsewhere.
 */
template <typename T>
struct data_handle {
    data_handle() = default;

    /** @brief Construct a data_handle from a plain pointer.
     */
    explicit data_handle(T* raw_ptr) {
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
        } else {
            // If that didn't work, just save the plain pointer value. This is unsafe
            // and should be removed. It is purely meant as an intermediate step, if
            // you use it then the guarantees above will be broken.
            m_container_or_raw_ptr = raw_ptr;
        }
    }

    /**
     * @brief Create a data_handle<T> wrapping the raw T*
     *
     * Unlike the constructor taking T*, this does *not* attempt to promote raw pointers to modern
     * data_handles.
     */
    data_handle(do_not_search_t, T* raw_ptr)
        : m_container_or_raw_ptr{raw_ptr} {}

    /**
     * @brief Get a data handle to a different element of the same array variable.
     *
     * Given an array variable a[N], this method allows a handle to a[i] to yield a handle to a[j]
     * within the same logical row. If the handle is wrapping a raw pointer T*, the shift is applied
     * to that raw pointer.
     */
    [[nodiscard]] data_handle next_array_element(int shift = 1) const {
        if (refers_to_a_modern_data_structure()) {
            int const new_array_index{m_array_index + shift};
            if (new_array_index < 0 || new_array_index >= m_array_dim) {
                std::ostringstream oss;
                oss << *this << " next_array_element(" << shift << "): out of range";
                throw std::runtime_error(oss.str());
            }
            return {m_offset,
                    static_cast<T* const*>(m_container_or_raw_ptr),
                    m_array_dim,
                    new_array_index};
        } else {
            return {do_not_search, static_cast<T*>(m_container_or_raw_ptr) + shift};
        }
    }

    /**
     * @brief Query whether this data handle is in "modern" mode.
     * @return true if the handle was created as a permutation-stable handle to an soa<...> data
     *         structure, otherwise false.
     *
     * Note that this does *not* mean that the handle is still valid. The referred-to row and/or
     * column may have gone away in the meantime.
     */
    [[nodiscard]] bool refers_to_a_modern_data_structure() const {
        return bool{m_offset} || m_offset.was_once_valid();
    }

    // TODO a const-ness cleanup. It should be possible to get
    // data_handle<T> from a view into a frozen container, even though it
    // isn't possible to get std::vector<T>& from a frozen container. And
    // data_handle<T const> should forbid writing to the data value.
    data_handle(non_owning_identifier_without_container offset,
                T* const* container,
                int array_dim,
                int array_index)
        : m_offset{std::move(offset)}
        , m_container_or_raw_ptr{const_cast<T**>(container)}
        , m_array_dim{array_dim}
        , m_array_index{array_index} {}

    [[nodiscard]] explicit operator bool() const {
        if (bool{m_offset}) {
            // valid, modern identifier (i.e. row is valid)
            return container_data();  // also check if the column is valid
        } else if (m_offset.was_once_valid()) {
            // once-valid, modern. no longer valid
            return false;
        } else {
            // null or raw pointer
            return m_container_or_raw_ptr;
        }
    }

    /** Query whether this generic handle points to a value from the `Tag` field
     *  of the given container.
     */
    template <typename Tag, typename Container>
    [[nodiscard]] bool refers_to(Container const& container) const {
        static_assert(Container::template has_tag_v<Tag>);
        if (bool{m_offset} || m_offset.was_once_valid()) {
            // basically in modern mode (possibly the entry we refer to has
            // died)
            return container.template is_storage_pointer<Tag>(container_data());
        } else {
            // raw-ptr mode or null
            return false;
        }
    }

    /**
     * @brief Get the current logical row number.
     */
    [[nodiscard]] std::size_t current_row() const {
        assert(refers_to_a_modern_data_structure());
        assert(m_offset);
        return m_offset.current_row();
    }

  private:
    // Try and cover the different operator* and operator T* cases with/without
    // const in a more composable way
    [[nodiscard]] T* raw_ptr() {
        return static_cast<T*>(m_container_or_raw_ptr);
    }
    [[nodiscard]] T const* raw_ptr() const {
        return static_cast<T const*>(m_container_or_raw_ptr);
    }
    [[nodiscard]] T* container_data() {
        return *static_cast<T* const*>(m_container_or_raw_ptr);
    }
    [[nodiscard]] T const* container_data() const {
        return *static_cast<T const* const*>(m_container_or_raw_ptr);
    }
    template <typename This>
    [[nodiscard]] static auto get_ptr_helper(This& this_ref) {
        if (this_ref.m_offset.has_always_been_null()) {
            // null or raw pointer
            return this_ref.raw_ptr();
        }
        if (this_ref.m_offset) {
            // valid, modern mode *identifier*; i.e. we know what offset into a vector we're
            // supposed to be looking at. It's also possible that the vector doesn't exist anymore,
            // either because the whole soa<...> container was deleted, or because an optional field
            // was toggled off in an existing soa<...> container. In that case, the base pointer
            // will be null.
            if (auto* const base_ptr = this_ref.container_data(); base_ptr) {
                // the array still exists
                return base_ptr + this_ref.m_array_dim * this_ref.m_offset.current_row() +
                       this_ref.m_array_index;
            }
            // the vector doesn't exist anymore => return nullptr
            return decltype(this_ref.raw_ptr()){nullptr};
        }
        // no longer valid, modern mode
        return decltype(this_ref.raw_ptr()){nullptr};
    }

  public:
    [[nodiscard]] T& operator*() {
        auto* const ptr = get_ptr_helper(*this);
        if (ptr) {
            return *ptr;
        } else {
            std::ostringstream oss;
            oss << *this << " attempt to dereference [T& operator*]";
            throw std::runtime_error(oss.str());
        }
    }

    [[nodiscard]] T const& operator*() const {
        auto* const ptr = get_ptr_helper(*this);
        if (ptr) {
            return *ptr;
        } else {
            std::ostringstream oss;
            oss << *this << " attempt to dereference [T const& operator*]";
            throw std::runtime_error(oss.str());
        }
    }

    [[nodiscard]] explicit operator T*() {
        return get_ptr_helper(*this);
    }

    [[nodiscard]] explicit operator T const *() const {
        return get_ptr_helper(*this);
    }

    friend std::ostream& operator<<(std::ostream& os, data_handle const& dh) {
        os << "data_handle<" << cxx_demangle(typeid(T).name()) << ">{";
        if (auto const valid = dh.m_offset; valid || dh.m_offset.was_once_valid()) {
            auto* const container_data = dh.container_data();
            if (auto const maybe_info = utils::find_container_info(container_data)) {
                if (!maybe_info->container().empty()) {
                    os << "cont=" << maybe_info->container() << ' ';
                }
                // the printout will show the logical row number, but we have the physical size.
                // these are different in case of array variables. convert the size to a logical
                // one, but add some printout showing what we did
                auto size = maybe_info->size();
                assert(dh.m_array_dim >= 1);
                assert(dh.m_array_index < dh.m_array_dim);
                assert(size % dh.m_array_dim == 0);
                size /= dh.m_array_dim;
                os << maybe_info->field();
                if (dh.m_array_dim > 1) {
                    os << '[' << dh.m_array_index << '/' << dh.m_array_dim << ']';
                }
                os << ' ' << dh.m_offset << '/' << size;
            } else {
                os << "cont=" << (container_data ? "unknown " : "deleted ") << dh.m_offset
                   << "/unknown";
            }
            // print the value if it exists and has an output operator
            if (valid) {
                // if the referred-to *column* was deleted but the referred-to *row* is still valid,
                // valid == true but ptr == nullptr.
                if (auto* const ptr = get_ptr_helper(dh); ptr) {
                    detail::print_value(os, *ptr);
                }
            }
        } else if (dh.m_container_or_raw_ptr) {
            os << "raw=" << dh.m_container_or_raw_ptr;
        } else {
            os << dh.m_offset;
        }
        return os << '}';
    }

    // TODO should a "modern" handle that has become invalid compare equal to a
    // null handle that was never valid? Perhaps yes, as both evaluate to
    // boolean false, but their string representations are different.
    [[nodiscard]] friend bool operator==(data_handle const& lhs, data_handle const& rhs) {
        return lhs.m_offset == rhs.m_offset &&
               lhs.m_container_or_raw_ptr == rhs.m_container_or_raw_ptr &&
               lhs.m_array_dim == rhs.m_array_dim && lhs.m_array_index == rhs.m_array_index;
    }

    [[nodiscard]] friend bool operator!=(data_handle const& lhs, data_handle const& rhs) {
        return !(lhs == rhs);
    }

    /**
     * @brief Get the identifier used by this handle.
     *
     * This is likely to only be useful for the (hopefully temporary) method
     * neuron::container::notify_when_handle_dies.
     */
    [[nodiscard]] non_owning_identifier_without_container identifier() const {
        return m_offset;
    }

  private:
    friend struct generic_data_handle;
    friend struct std::hash<data_handle>;
    non_owning_identifier_without_container m_offset{};  // basically std::size_t*
    // If m_offset is/was valid for a modern container, this is a pointer to a value containing the
    // start of the underlying contiguous storage (i.e. the return value of std::vector<T>::data())
    // otherwise it is possibly-null T*
    void* m_container_or_raw_ptr{};
    // These are needed for "modern" handles to array variables, where the offset
    // yielded by m_offset needs to be scaled/shifted by an array dimension/index
    // before being applied to m_container_or_raw_ptr
    int m_array_dim{1}, m_array_index{};
};

/**
 * @brief Explicit specialisation data_handle<void>.
 *
 * This is convenient as it allows void* to be stored in generic_data_handle.
 * The "modern style" data handles that hold a reference to a container and a way of determining an
 * offset into that container do not make sense with a void value type, so this only supports the
 * "legacy" mode where a data handle wraps a plain pointer.
 */
template <>
struct data_handle<void> {
    data_handle() = default;
    data_handle(void* raw_ptr)
        : m_raw_ptr{raw_ptr} {}
    data_handle(do_not_search_t, void* raw_ptr)
        : m_raw_ptr{raw_ptr} {}
    [[nodiscard]] bool refers_to_a_modern_data_structure() const {
        return false;
    }
    explicit operator bool() const {
        return m_raw_ptr;
    }
    explicit operator void*() {
        return m_raw_ptr;
    }
    explicit operator void const *() const {
        return m_raw_ptr;
    }
    friend std::ostream& operator<<(std::ostream& os, data_handle<void> const& dh) {
        return os << "data_handle<void>{raw=" << dh.m_raw_ptr << '}';
    }
    friend bool operator==(data_handle<void> const& lhs, data_handle<void> const& rhs) {
        return lhs.m_raw_ptr == rhs.m_raw_ptr;
    }

  private:
    friend struct generic_data_handle;
    friend struct std::hash<data_handle<void>>;
    void* m_raw_ptr;
};

}  // namespace neuron::container

// Enable data_handle<T> as a key type in std::unordered_map
template <typename T>
struct std::hash<neuron::container::data_handle<T>> {
    std::size_t operator()(neuron::container::data_handle<T> const& s) const noexcept {
        static_assert(sizeof(std::size_t) == sizeof(T const*));
        if (s.m_offset || s.m_offset.was_once_valid()) {
            // The hash should not include the current row number, but rather the
            // std::size_t* that is dereferenced to *get* the current row number,
            // and which container this generic value lives in.
            return std::hash<neuron::container::non_owning_identifier_without_container>{}(
                       s.m_offset) ^
                   reinterpret_cast<std::size_t>(s.m_container_or_raw_ptr);
        } else {
            return reinterpret_cast<std::size_t>(s.m_container_or_raw_ptr);
        }
    }
};

template <>
struct std::hash<neuron::container::data_handle<void>> {
    std::size_t operator()(neuron::container::data_handle<void> const& s) const noexcept {
        static_assert(sizeof(std::size_t) == sizeof(void*));
        return reinterpret_cast<std::size_t>(s.m_raw_ptr);
    }
};
