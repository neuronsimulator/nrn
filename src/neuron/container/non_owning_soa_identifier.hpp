#pragma once
#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <ostream>
#include <vector>

namespace neuron::container {
/**
 * @brief Struct used to index SoAoS data, such as array range variables.
 */
struct field_index {
    int field{}, array_index{};
};

inline constexpr std::size_t invalid_row = std::numeric_limits<std::size_t>::max();

/**
 * @brief A non-owning permutation-stable identifier for an entry in a container.
 *
 * The container type is not specified. This is essentially a wrapper for
 * std::size_t* that avoids using that naked type in too many places.
 */
struct non_owning_identifier_without_container {
    /**
     * @brief Create a null identifier.
     */
    non_owning_identifier_without_container() = default;

    non_owning_identifier_without_container(const non_owning_identifier_without_container& other) =
        default;
    non_owning_identifier_without_container(non_owning_identifier_without_container&& other) =
        default;

    non_owning_identifier_without_container& operator=(
        const non_owning_identifier_without_container&) = default;
    non_owning_identifier_without_container& operator=(non_owning_identifier_without_container&&) =
        default;

    ~non_owning_identifier_without_container() = default;


    /**
     * @brief Does the identifier refer to a valid entry?
     *
     * The row can be invalid because the identifier was always null, or it can
     * have become invalid if the relevant entry was deleted after the
     * identifier was created.
     */
    [[nodiscard]] explicit operator bool() const {
        return m_ptr && (*m_ptr != invalid_row);
    }

    /**
     * @brief What is the current row?
     *
     * The returned value is invalidated by any deletions from the underlying
     * container, and by any permutations of the underlying container.
     */
    [[nodiscard]] std::size_t current_row() const {
        assert(m_ptr);
        auto const value = *m_ptr;
        assert(value != invalid_row);
        return value;
    }

    /**
     * @brief Did the identifier use to refer to a valid entry?
     */
    [[nodiscard]] bool was_once_valid() const {
        return m_ptr && *m_ptr == invalid_row;
    }

    /**
     * @brief Has the identifier always been null.
     *
     * has_always_been_null() --> "null"
     * !has_always_been_null && was_once_valid --> "died"
     * !has_always_been_null && !was_once_valid --> "row=X"
     */
    [[nodiscard]] bool has_always_been_null() const {
        return !m_ptr;
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    non_owning_identifier_without_container const& id) {
        if (!id.m_ptr) {
            return os << "null";
        } else if (*id.m_ptr == invalid_row) {
            return os << "died";
        } else {
            return os << "row=" << *id.m_ptr;
        }
    }

    /** @brief Test if two handles are both null or refer to the same valid row.
     */
    friend bool operator==(non_owning_identifier_without_container lhs,
                           non_owning_identifier_without_container rhs) {
        return lhs.m_ptr == rhs.m_ptr || (!lhs && !rhs);
    }

    friend bool operator!=(non_owning_identifier_without_container lhs,
                           non_owning_identifier_without_container rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(non_owning_identifier_without_container lhs,
                          non_owning_identifier_without_container rhs) {
        return lhs.m_ptr < rhs.m_ptr;
    }

  protected:
    // Needed to convert owning_identifier<T> to non_owning_identifier<T>
    template <typename>
    friend struct owning_identifier;

    template <typename, typename...>
    friend struct soa;
    friend struct std::hash<non_owning_identifier_without_container>;
    explicit non_owning_identifier_without_container(std::shared_ptr<std::size_t> ptr)
        : m_ptr{std::move(ptr)} {}
    void set_current_row(std::size_t row) {
        assert(m_ptr);
        *m_ptr = row;
    }

    explicit non_owning_identifier_without_container(size_t row)
        : m_ptr(std::make_shared<size_t>(row)) {}

  private:
    std::shared_ptr<std::size_t> m_ptr{};
};
}  // namespace neuron::container
template <>
struct std::hash<neuron::container::non_owning_identifier_without_container> {
    std::size_t operator()(
        neuron::container::non_owning_identifier_without_container const& h) noexcept {
        return reinterpret_cast<std::size_t>(h.m_ptr.get());
    }
};
