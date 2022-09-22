#pragma once
#include "backtrace_utils.h"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>
#include <vector>

namespace neuron::container {

namespace detail {
/** @brief Get the vector in which dying owning_identifier_bases should live.
 *
 * @todo Should this be templated on the container type? Or should the garbage
 *       vector live inside the relevant soa<...> instance? If it did, how
 *       would that work for the use-case of cloning/swapping those instances?
 * @todo Do we end up with duplicate definitions of this vector because of
 *       dlopen() etc. ?
 */
inline std::vector<std::unique_ptr<std::size_t>>& garbage() {
    static std::vector<std::unique_ptr<std::size_t>> x{};
    return x;
}

inline constexpr std::size_t invalid_row = std::numeric_limits<std::size_t>::max();
template <bool>
struct generic_data_handle;
struct generic_data_handle_proxy;
}  // namespace detail


template <typename, typename...>
struct soa;

template <typename, typename>
struct owning_identifier_base;

/** @brief A non-owning permutation-stable identifier for a row in another container.
 *
 *  A (non-owning) handle to that row probably consists of an instance of a
 *  subclass of this struct and some kind of reference to the actual container.
 */
struct identifier_base {
    /** @brief Construct a null handle.
     */
    identifier_base() = default;

    /** @brief Does the handle refer to a valid row?
     */
    [[nodiscard]] explicit operator bool() const {
        return m_ptr && (*m_ptr != detail::invalid_row);
    }

    /** @brief What is the current row?
     *
     *  The returned value is invalidated by any deletions from the underlying
     *  container, and by any permutations of the underlying container.
     */
    [[nodiscard]] std::size_t current_row() const {
        assert(m_ptr);
        auto const value = *m_ptr;
        assert(value != detail::invalid_row);
        return value;
    }

    friend std::ostream& operator<<(std::ostream& os, identifier_base const& id) {
        if (!id.m_ptr) {
            return os << "null";
        } else if (*id.m_ptr == detail::invalid_row) {
            return os << "died";
        } else {
            return os << "row=" << *id.m_ptr;
        }
    }

    /** @brief Test if two handles are both null or refer to the same valid row.
     */
    friend bool operator==(identifier_base lhs, identifier_base rhs) {
        return lhs.m_ptr == rhs.m_ptr || (!lhs && !rhs);
    }

    friend bool operator!=(identifier_base lhs, identifier_base rhs) {
        return !(lhs == rhs);
    }

  private:
    template <bool>
    friend struct detail::generic_data_handle;
    friend struct detail::generic_data_handle_proxy;
    template <typename, typename...>
    friend struct soa;
    template <typename, typename>
    friend struct owning_identifier_base;
    friend struct std::hash<identifier_base>;
    /** This is needed for converting owning to non-owning handles, and is also
     *  useful for assertions.
     */
    identifier_base(std::size_t* ptr)
        : m_ptr{ptr} {}
    void set_current_row(std::size_t new_row) {
        assert(m_ptr);
        *m_ptr = new_row;
    }
    std::size_t* m_ptr{};
};

/** @brief Base class for a owning_handle to a row in a soa<...> container.
 *  @tparam DataContainer neuron::container::soa<...> type that holds the SOA-format data.
 *  @tparam NonOwningElementHandle Non-owning equivalent identifier/handle.
 *
 *  When this is destroyed the corresponding row in the container is deleted.
 *  @todo Figure out what the NonOwningElementHandle type is from DataContainer?
 */
template <typename DataContainer, typename NonOwningElementHandle>
struct owning_identifier_base {
    owning_identifier_base(DataContainer& data_container)
        : m_ptr{new std::size_t, data_container} {}
    /** @brief Does the handle refer to a valid row?
     */
    [[nodiscard]] explicit operator bool() const {
        return m_ptr && (*m_ptr != detail::invalid_row);
    }
    [[nodiscard]] operator NonOwningElementHandle() const {
        return {m_ptr.get()};
    }
    void swap(owning_identifier_base& other) noexcept {
        m_ptr.swap(other.m_ptr);
    }
    DataContainer& data_container() const {
        return m_ptr.get_deleter().m_data_ref;
    }
    /** @brief What is the current row?
     */
    [[nodiscard]] std::size_t current_row() const {
        assert(m_ptr);
        return *m_ptr;
    }

  private:
    struct deleter {
        deleter(DataContainer& data_container)
            : m_data_ref{data_container} {}
        void operator()(std::size_t* p) const {
            DataContainer& data_container = m_data_ref;
            assert(p);
            // We should still be a valid reference at this point.
            assert(*p < data_container.size());
            // Prove that the bookkeeping works.
            assert(data_container.identifier(*p) == p);
            bool terminate{false};
            // Delete the corresponding row from `data_container`
            try {
                data_container.erase(*p);
            } catch (std::exception const& e) {
                // Cannot throw from unique_ptr release/reset/destructor, this
                // is the best we can do. Most likely what has happened is
                // something like:
                //   auto const read_only_token = node_data.sorted_token();
                //   list_of_nodes.pop_back();
                // which tries to delete a row from a container in read-only mode.
                std::cerr << "neuron::container::owning_identifier_base<"
                          << cxx_demangle(typeid(DataContainer).name()) << ", "
                          << cxx_demangle(typeid(NonOwningElementHandle).name())
                          << "> destructor could not delete from the underlying storage: "
                          << e.what() << " [" << cxx_demangle(typeid(e).name())
                          << "]. This is not recoverable, aborting." << std::endl;
                terminate = true;
            }
            if (terminate) {
                std::terminate();
            }
            // We don't know how many people know the pointer `p`, so write a sentinel
            // value to it and transfer ownership "elsewhere".
            *p = detail::invalid_row;
            // This is sort-of formalising a memory leak. In principle we could
            // cleanup garbage() by scanning all our data structures and finding
            // references to the pointers that it contains. In practice it seems
            // unlikely that either this, or using std::shared_ptr just to avoid it,
            // would be worth it.
            detail::garbage().emplace_back(p);
        }
        std::reference_wrapper<DataContainer> m_data_ref;
    };
    std::unique_ptr<std::size_t, deleter> m_ptr;
    template <typename, typename...>
    friend struct soa;
    void set_current_row(std::size_t new_row) {
        assert(m_ptr);
        *m_ptr = new_row;
    }
};

}  // namespace neuron::container
template <>
struct std::hash<neuron::container::identifier_base> {
    std::size_t operator()(neuron::container::identifier_base const& h) noexcept {
        return reinterpret_cast<std::size_t>(h.m_ptr);
    }
};
