#pragma once
#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>
/** @brief Get the vector in which dying OwningElementHandles should live.
 *
 * @todo Should this be templated on the container type? Or should the garbage
 *       vector live inside the relevant SOAContainer instance? If it did, how
 *       would that work for the use-case of cloning/swapping those instances?
 */
inline std::vector<std::unique_ptr<std::size_t>>& garbage() {
    static std::vector<std::unique_ptr<std::size_t>> x{};
    return x;
}

inline constexpr std::size_t invalid_row = std::numeric_limits<std::size_t>::max();

template <typename, typename...>
struct SOAContainer;
template <typename, typename>
struct OwningElementHandle;

/** @brief Base class for a handle to a row in an SOAContainer container.
 */
struct ElementHandle {
    /** @brief Construct a null handle.
     */
    ElementHandle() = default;

    /** @brief Does the handle refer to a valid row?
     */
    explicit operator bool() const {
        return m_ptr && (*m_ptr != invalid_row);
    }

    /** @brief What is the current row?
     *
     *  The returned value is invalidated by any deletions from the underlying
     *  container, and by any permutations of the underlying container.
     */
    [[nodiscard]] std::size_t current_row() const {
        assert(m_ptr);
        auto const value = *m_ptr;
        assert(value != invalid_row);
        return value;
    }

    /** @brief Test if two handles are both null or refer to the same valid row.
     */
    friend bool operator==(ElementHandle lhs, ElementHandle rhs) {
        return lhs.m_ptr == rhs.m_ptr || (!lhs && !rhs);
    }

    friend bool operator!=(ElementHandle lhs, ElementHandle rhs) {
        return !(lhs == rhs);
    }

  private:
    template <typename, typename...>
    friend struct SOAContainer;
    template <typename, typename>
    friend struct OwningElementHandle;
    friend struct std::hash<ElementHandle>;
    /** This is needed for converting owning to non-owning handles, and is also
     *  useful for assertions.
     */
    ElementHandle(std::size_t* ptr)
        : m_ptr{ptr} {}
    void set_current_row(std::size_t new_row) {
        assert(m_ptr);
        *m_ptr = new_row;
    }
    std::size_t* m_ptr{};
};

/** @brief Base class for a owning handle to a row in a SOAContainer container.
 *  @tparam DataContainer SOAContainer type that holds the SOA-format data.
 *  @tparam NonOwningElementHandle Non-owning equivalent identifier/handle.
 *
 *  When this is destroyed the corresponding row in the container is deleted.
 *  @todo Figure out what the NonOwningElementHandle type is from DataContainer?
 */
template <typename DataContainer, typename NonOwningElementHandle>
struct OwningElementHandle {
    OwningElementHandle(DataContainer& data_container)
        : m_ptr{new std::size_t, data_container} {}
    /** @brief Does the handle refer to a valid row?
     */
    explicit operator bool() const {
        return m_ptr && (*m_ptr != invalid_row);
    }
    operator NonOwningElementHandle() const {
        return {m_ptr.get()};
    }
    void swap(OwningElementHandle& other) noexcept {
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
            // Delete the corresponding row from `data_container`
            data_container.erase(*p);
            // We don't know how many people know the pointer `p`, so write a sentinel
            // value to it and transfer ownership "elsewhere".
            *p = invalid_row;
            // This is sort-of formalising a memory leak. In principle we could
            // cleanup garbage() by scanning all our data structures and finding
            // references to the pointers that it contains. In practice it seems
            // unlikely that either this, or using std::shared_ptr just to avoid it,
            // would be worth it.
            garbage().emplace_back(p);
        }
        std::reference_wrapper<DataContainer> m_data_ref;
    };
    std::unique_ptr<std::size_t, deleter> m_ptr;
};

template <>
struct std::hash<ElementHandle> {
    std::size_t operator()(ElementHandle const& o) noexcept {
        return reinterpret_cast<std::size_t>(o.m_ptr);
    }
};
