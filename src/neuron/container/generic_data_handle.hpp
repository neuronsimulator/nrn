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

namespace detail {
template <bool>
struct generic_data_handle;
}

/** @brief Type-erased data_handle<T> with stronger checks.
 */
using generic_data_handle = detail::generic_data_handle<false>;

/** @brief Type-erased data_handle<T> with weaker checks.
 */
using permissive_generic_data_handle = detail::generic_data_handle<true>;

namespace detail {
/** @brief Helper type to support backwards-compatibility hacks.
 *
 *  The (possibly misguided) goal here is to make (T**)(&_p_foo) work in
 *  translated MOD file code when _p_foo is a generic_data_handle. This pattern
 *  is (regrettably) common in VERBATIM blocks. It can only realistically be
 *  made to work if the generic_data_handle is holding a raw pointer. Using this
 *  pattern will leave _p_foo thinking that it refers to a raw pointer of type T.
 *
 *  The use of this pattern should be considered deprecated. The alternative:
 *    auto* my_foo = static_cast<T*>(_p_foo);
 *    my_foo = transform(my_foo);
 *    _p_foo = my_foo;
 *  Should be preferred.
 */
struct generic_data_handle_proxy {
    generic_data_handle_proxy(permissive_generic_data_handle& handle)
        : m_handle{handle} {}
    template <typename T>
    explicit operator T**() const;

  private:
    permissive_generic_data_handle& m_handle;
};

/** @brief Helper type used to flag when the generic_data_handle contains a null value.
 */
struct typeless_null {};

/** @brief Non-template stable handle to a generic value.
 *
 *  This is a type-erased version of data_handle<T>.
 *
 *  @tparam permissive Whether to enable less-safe backwards compatibility
 *  features or not. This is typically set for use inside translated MOD files
 *  where there is a lot of legacy VERBATIM code in the world.
 */
template <bool permissive = false>
struct generic_data_handle {
    generic_data_handle() = default;
    template <bool other_permissive>
    generic_data_handle(generic_data_handle<other_permissive> const& other)
        : m_offset{other.m_offset}
        , m_container{other.m_container}
        , m_type{other.m_type} {}
    // Avoid trying to access members of the data_handle<void> specialisation.
    template <typename T, std::enable_if_t<std::is_same_v<T, void>, int> = 0>
    explicit generic_data_handle(data_handle<T> const& handle)
        : m_container{handle.m_raw_ptr}
        , m_type{typeid(T)} {
        static_assert(std::is_same_v<T, void>);
        // data_handle<void> cannot refer to a modern data structure, it can
        // only wrap a void*
        assert(!m_offset);
        assert(!m_offset.m_ptr);
    }

    // Avoid trying to access members of the data_handle<void> specialisation.
    template <typename T, std::enable_if_t<!std::is_same_v<T, void>, int> = 0>
    explicit generic_data_handle(data_handle<T> const& handle)
        : m_offset{handle.m_raw_ptr ? nullptr : handle.m_offset.m_ptr}
        , m_container{handle.m_raw_ptr ? static_cast<void*>(handle.m_raw_ptr)
                                       : static_cast<void*>(handle.m_container)}
        , m_type{typeid(T)} {
        static_assert(!std::is_same_v<T, void>);
    }

    template <typename T>
    explicit operator data_handle<T>() const {
        bool const is_typeless_null{m_type == std::type_index{typeid(typeless_null)}};
        if (!is_typeless_null && std::type_index{typeid(T)} != m_type) {
            throw std::runtime_error("Cannot convert generic_data_handle(" +
                                     cxx_demangle(m_type.name()) + ") to data_handle<" +
                                     cxx_demangle(typeid(T).name()) + ">");
        }
        if (m_offset) {
            assert(!is_typeless_null);
            if constexpr (std::is_same_v<T, void>) {
                throw std::runtime_error(
                    "generic_data_handle referring to void should never be in 'modern' mode.");
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
            // raw pointer, or a null data handle.
            return {static_cast<T*>(m_container)};
        }
    }

    explicit operator bool() const {
        // Branches follow the data_handle<T> conversion just above
        return bool{m_offset} ? true : (m_offset.m_ptr ? false : static_cast<bool>(m_container));
    }

    /** @brief Explicit conversion to any T* in non-permissive mode.
     */
    template <typename T,
              bool local_permissive = permissive,
              std::enable_if_t<!local_permissive, int> = 0>
    explicit operator T*() const {
        return static_cast<T*>(static_cast<data_handle<T>>(*this));
    }

    /** @brief Explicit conversion to non-void T in permissive mode.
     */
    template <typename T,
              bool local_permissive = permissive,
              std::enable_if_t<local_permissive && !std::is_same_v<std::decay_t<T>, void>, int> = 0>
    explicit operator T*() const {
        return static_cast<T*>(static_cast<data_handle<T>>(*this));
    }

    /** @brief Implicit conversion to void* when in permissive mode.
     */
    template <typename T,
              bool local_permissive = permissive,
              std::enable_if_t<local_permissive && std::is_same_v<std::decay_t<T>, void>, int> = 0>
    [[deprecated(
        "You should prefer explicit conversions to definite types when using generic_data_handle")]]
    operator T*() const {
        return static_cast<T*>(static_cast<data_handle<T>>(*this));
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

    /** @brief Deploy a compatibility hack in permissive mode.
     *
     *  See "C.166: Overload unary & only as part of a system of smart pointers
     *  and references", this is not very nice and may be a bad idea.
     */
    template <bool local_permissive = permissive>
    [[deprecated(
        "this is only intended to support legacy patterns in VERBATIM "
        "blocks")]] std::enable_if_t<local_permissive, generic_data_handle_proxy>
    operator&() {
        return {*this};
    }

    friend std::ostream& operator<<(std::ostream& os, generic_data_handle const& dh) {
        auto const maybe_info = utils::find_container_info(dh.m_container);
        os << "generic_data_handle{";
        if (maybe_info) {
            os << "cont=" << maybe_info->name << ' ' << dh.m_offset << '/' << maybe_info->size;
        } else {
            os << "raw=";
            if (dh.m_container) {
                os << dh.m_container;
            } else {
                os << "nullptr";
            }
        }
        return os << ", type=" << cxx_demangle(dh.m_type.name()) << '}';
    }


    [[nodiscard]] std::string type_name() const {
        return cxx_demangle(m_type.name());
    }

  private:
    friend struct generic_data_handle<!permissive>;
    friend struct generic_data_handle_proxy;
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
generic_data_handle_proxy::operator T**() const {
    if (m_handle.m_offset || m_handle.m_offset.m_ptr) {
        throw std::runtime_error(
            "(T**)(&generic_data_handle) pattern only supported for generic_data_handle(" +
            cxx_demangle(m_handle.m_type.name()) + ") in legacy mode");
    }
    bool const is_typeless_null{m_handle.m_type == std::type_index{typeid(typeless_null)}};
    if (!is_typeless_null && std::type_index{typeid(T)} != m_handle.m_type) {
        throw std::runtime_error("Cannot convert &generic_data_handle(" +
                                 cxx_demangle(m_handle.m_type.name()) + ") to " +
                                 cxx_demangle(typeid(T).name()) + "**");
    }
    if (is_typeless_null) {
        // double** foo = (double**)(&some_handle)
        // leaves the handle referring to double
        m_handle.m_type = std::type_index{typeid(T)};
    }

    return reinterpret_cast<T**>(&(m_handle.m_container));
}

}  // namespace detail
}  // namespace neuron::container