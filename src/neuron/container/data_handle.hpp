#pragma once
#include "backtrace_utils.h"
#include "neuron/container/soa_identifier.hpp"
#include "neuron/model_data_fwd.hpp"

#include <ostream>
#include <sstream>
#include <vector>

namespace neuron::container {
struct do_not_search_t {};
inline constexpr do_not_search_t do_not_search{};

namespace detail {
template <typename T, typename = void>
inline constexpr bool has_output_operator_v = false;
template <typename T>
inline constexpr bool
    has_output_operator_v<T, std::void_t<decltype(std::cout << std::declval<T>())>> = true;
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
            if (!m_offset) {
                std::ostringstream oss;
                oss << *this << " invalid data_handle(T*)";
                throw std::runtime_error(oss.str());
            }
        } else {
            // If that didn't work, just save the plain pointer value. This is unsafe
            // and should be removed. It is purely meant as an intermediate step, if
            // you use it then the guarantees above will be broken.
            m_container_or_raw_ptr = raw_ptr;
        }
    }

    data_handle(do_not_search_t, T* raw_ptr)
        : m_container_or_raw_ptr{raw_ptr} {}

    [[nodiscard]] bool refers_to_a_modern_data_structure() const {
        return bool{m_offset} || m_offset.was_once_valid();
    }

    // TODO a const-ness cleanup. It should be possible to get
    // data_handle<T> from a view into a frozen container, even though it
    // isn't possible to get std::vector<T>& from a frozen container. And
    // data_handle<T const> should forbid writing to the data value.
    data_handle(non_owning_identifier_without_container offset, std::vector<T> const& container)
        : m_offset{std::move(offset)}
        , m_container_or_raw_ptr{&const_cast<std::vector<T>&>(container)} {}

    explicit operator bool() const {
        if (bool{m_offset}) {
            // valid, modern
            return true;
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
    bool refers_to(Container const& container) const {
        static_assert(Container::template has_tag_v<Tag>);
        if (bool{m_offset} || m_offset.was_once_valid()) {
            // basically in modern mode (possibly the entry we refer to has
            // died)
            return container.template is_storage_pointer<Tag>(container_ptr());
        } else {
            // raw-ptr mode or null
            return false;
        }
    }

    [[nodiscard]] std::size_t current_row() const {
        assert(refers_to_a_modern_data_structure());
        assert(m_offset);
        return m_offset.current_row();
    }

  private:
    // Try and cover the different operator* and operator T* cases with/without
    // const in a more composable way
    T* raw_ptr() {
        return static_cast<T*>(m_container_or_raw_ptr);
    }
    T const* raw_ptr() const {
        return static_cast<T const*>(m_container_or_raw_ptr);
    }
    std::vector<T>* container_ptr() {
        return static_cast<std::vector<T>*>(m_container_or_raw_ptr);
    }
    std::vector<T> const* container_ptr() const {
        return static_cast<std::vector<T> const*>(m_container_or_raw_ptr);
    }
    template <typename This>
    static auto get_ptr_helper(This& this_ref) {
        if (this_ref.m_offset.has_always_been_null()) {
            // null or raw pointer
            return this_ref.raw_ptr();
        }
        if (this_ref.m_offset) {
            // valid, modern mode
            return this_ref.container_ptr()->data() + this_ref.m_offset.current_row();
        }
        // no longer valid, modern mode
        return decltype(this_ref.raw_ptr()){nullptr};
    }

  public:
    T& operator*() {
        auto* const ptr = get_ptr_helper(*this);
        if (ptr) {
            return *ptr;
        } else {
            std::ostringstream oss;
            oss << *this << " attempt to dereference [T& operator*]";
            throw std::runtime_error(oss.str());
        }
    }

    T const& operator*() const {
        auto* const ptr = get_ptr_helper(*this);
        if (ptr) {
            return *ptr;
        } else {
            std::ostringstream oss;
            oss << *this << " attempt to dereference [T const& operator*]";
            throw std::runtime_error(oss.str());
        }
    }

    explicit operator T*() {
        return get_ptr_helper(*this);
    }

    explicit operator T const *() const {
        return get_ptr_helper(*this);
    }

    friend std::ostream& operator<<(std::ostream& os, data_handle const& dh) {
        os << "data_handle<" << cxx_demangle(typeid(T).name()) << ">{";
        if (auto const valid = dh.m_offset; valid || dh.m_offset.was_once_valid()) {
            auto const maybe_info = utils::find_container_info(dh.container_ptr());
            if (maybe_info) {
                if (!maybe_info->container.empty()) {
                    os << "cont=" << maybe_info->container << ' ';
                }
                os << maybe_info->field << ' ' << dh.m_offset << '/' << maybe_info->size;
            } else {
                os << "cont=unknown " << dh.m_offset << "/unknown";
            }
            // print the value if it has an output operator
            if constexpr (detail::has_output_operator_v<T>) {
                if (valid) {
                    os << " val=" << *dh;
                }
            }
        } else if (dh.m_container_or_raw_ptr) {
            os << "raw=" << dh.m_container_or_raw_ptr;
        } else {
            os << "nullptr";
        }
        return os << '}';
    }

    // TODO should a "modern" handle that has become invalid compare equal to a
    // null handle that was never valid? Perhaps yes, as both evaluate to
    // boolean false, but their string representations are different.
    friend bool operator==(data_handle const& lhs, data_handle const& rhs) {
        return lhs.m_offset == rhs.m_offset &&
               lhs.m_container_or_raw_ptr == rhs.m_container_or_raw_ptr;
    }

    friend bool operator!=(data_handle const& lhs, data_handle const& rhs) {
        return !(lhs == rhs);
    }

  private:
    friend struct generic_data_handle;
    friend struct std::hash<data_handle>;
    non_owning_identifier_without_container m_offset{};  // basically std::size_t*
    // If m_offset is/was valid for a modern container, this is std::vector<T>*,
    // otherwise it is possibly-null T*
    void* m_container_or_raw_ptr{};
};

/**
 * @brief Explicit specialisation data_handle<void>.
 *
 * This is convenient as it allows void* to be stored in generic_data_handle.
 * The default implementation of data_handle<T> would otherwise need several
 * special cases to avoid instantiating std::vector<void> and forming references
 * to void. The "modern style" data handles that hold a reference to a container
 * and a way of determining an offset into that container do not make sense with
 * a void value type, so this only supports the "legacy" mode where a data
 * handle wraps a plain pointer.
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
