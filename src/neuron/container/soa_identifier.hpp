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
/**
 * @brief The vector in which dying owning_identifier std::size_t's live.
 *
 * This is defined in container.cpp to avoid multiple-definition issues.
 */
extern std::vector<std::unique_ptr<std::size_t>> garbage;
inline constexpr std::size_t invalid_row = std::numeric_limits<std::size_t>::max();
}  // namespace detail

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

    /**
     * @brief Does the identifier refer to a valid entry?
     *
     * The row can be invalid because the identifier was always null, or it can
     * have become invalid if the relevant entry was deleted after the
     * identifier was created.
     */
    [[nodiscard]] explicit operator bool() const {
        return m_ptr && (*m_ptr != detail::invalid_row);
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
        assert(value != detail::invalid_row);
        return value;
    }

    /**
     * @brief Did the identifier use to refer to a valid entry?
     *
     */
    [[nodiscard]] bool was_once_valid() const {
        return m_ptr && *m_ptr == detail::invalid_row;
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    non_owning_identifier_without_container const& id) {
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
    friend bool operator==(non_owning_identifier_without_container lhs,
                           non_owning_identifier_without_container rhs) {
        return lhs.m_ptr == rhs.m_ptr || (!lhs && !rhs);
    }

    friend bool operator!=(non_owning_identifier_without_container lhs,
                           non_owning_identifier_without_container rhs) {
        return !(lhs == rhs);
    }

  protected:
    // Needed to convert owning_identifier<T> to non_owning_identifier<T>
    template <typename>
    friend struct owning_identifier;

    template <typename, template <typename> typename, typename...>
    friend struct soa;
    friend struct std::hash<non_owning_identifier_without_container>;
    non_owning_identifier_without_container(std::size_t* ptr)
        : m_ptr{ptr} {}
    void set_current_row(std::size_t row) {
        assert(m_ptr);
        *m_ptr = row;
    }

  private:
    std::size_t* m_ptr{};
};

/**
 * @brief A non-owning permutation-stable identifier for a entry in a container.
 * @tparam Storage The type of the referred-to container. This might be a type
 *                 derived from @ref neuron::container::soa<...>, or a plain
 *                 container like std::vector<T> in the case of @ref
 *                 neuron::container::data_handle<T>.
 *
 * A (non-owning) handle to that row combines an instance of that class with an
 * interface that is specific to Storage. non_owning_identifier<Storage> wraps
 * non_owning_identifier_without_container so as to provide the same interface
 * as owning_identifier<Storage>.
 */
template <typename Storage>
struct non_owning_identifier: non_owning_identifier_without_container {
    non_owning_identifier(Storage* storage, non_owning_identifier_without_container id)
        : non_owning_identifier_without_container{std::move(id)}
        , m_storage{storage} {}

    /**
     * @brief Return a reference to the container in which this entry lives.
     */
    Storage& underlying_storage() {
        assert(m_storage);
        return *m_storage;
    }

    /**
     * @brief Return a const reference to the container in which this entry lives.
     */
    Storage const& underlying_storage() const {
        assert(m_storage);
        return *m_storage;
    }

  private:
    // TODO clean up this friend stuff...
    // template <typename>
    // friend struct data_handle;
    // friend struct generic_data_handle;
    // template <typename, template <typename> typename, typename...>
    // friend struct soa;
    // This is so
    // template <typename>
    // friend struct owning_identifier;
    // friend struct std::hash<identifier_base>;
    /** This is needed for converting owning to non-owning handles, and is also
     *  useful for assertions.
     */
    // non_owning_identifier(Storage& storage, std::size_t* ptr) : m_storage{storage}, m_ptr{ptr} {}
    // void set_current_row(std::size_t new_row) {
    //     assert(m_ptr);
    //     *m_ptr = new_row;
    // }
    Storage* m_storage;
};

/**
 * @brief An owning permutation-stable identifier for a entry in a container.
 * @tparam Storage The type of the referred-to container, which is expected to
 *                 be a type derived from @ref neuron::container::soa<...>
 *
 * This identifier has owning semantics, meaning that when it is destroyed the
 * corresponding entry in the container is deleted.
 */
template <typename Storage>
struct owning_identifier {
    /**
     * @brief Create a non-null owning identifier by creating a new entry.
     */
    owning_identifier(Storage& storage)
        : owning_identifier() {
        auto tmp = storage.acquire_owning_identifier();
        using std::swap;
        swap(*this, tmp);
    }

    /**
     * @brief Create a non-null owning identifier that owns the given row.
     */
    owning_identifier(Storage& storage, std::size_t row)
        : m_ptr{new std::size_t, storage} {
        *m_ptr = row;
    }

    /**
     * @brief Return a reference to the container in which this entry lives.
     */
    Storage& underlying_storage() {
        return *m_ptr.get_deleter().m_data_ptr;
    }

    /**
     * @brief Return a const reference to the container in which this entry lives.
     */
    Storage const& underlying_storage() const {
        return *m_ptr.get_deleter().m_data_ptr;
    }

    [[nodiscard]] operator non_owning_identifier<Storage>() const {
        return {const_cast<Storage*>(&underlying_storage()), m_ptr.get()};
    }

    [[nodiscard]] operator non_owning_identifier_without_container() const {
        return static_cast<non_owning_identifier<Storage>>(*this);
    }

    /**
     * @brief What is the current row?
     *
     * The returned value is invalidated by any deletions from the underlying
     * container, and by any permutations of the underlying container.
     */
    [[nodiscard]] std::size_t current_row() const {
        assert(m_ptr);
        return *m_ptr;
    }

    friend void swap(owning_identifier& first, owning_identifier& second) {
        using std::swap;
        swap(first.m_ptr, second.m_ptr);
    }

    friend std::ostream& operator<<(std::ostream& os, owning_identifier const& oi) {
        return os << "owning " << non_owning_identifier<Storage>{oi};
    }

  private:
    owning_identifier() = default;
    struct deleter {
        deleter() = default;
        deleter(Storage& data_container)
            : m_data_ptr{&data_container} {}
        void operator()(std::size_t* p) const {
            assert(m_data_ptr);
            auto& data_container = *m_data_ptr;
            assert(p);
            // We should still be a valid reference at this point.
            assert(*p < data_container.size());
            // Prove that the bookkeeping works.
            assert(data_container.at(*p).id() == p);
            bool terminate{false};
            // Delete the corresponding row from `data_container`
            try {
                data_container.erase(*p);
            } catch (std::exception const& e) {
                // Cannot throw from unique_ptr release/reset/destructor, this
                // is the best we can do. Most likely what has happened is
                // something like:
                //   auto const read_only_token = node_data.get_sorted_token();
                //   list_of_nodes.pop_back();
                // which tries to delete a row from a container in read-only mode.
                std::cerr << "neuron::container::owning_identifier<"
                          << cxx_demangle(typeid(Storage).name())
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
            // cleanup garbage by scanning all our data structures and finding
            // references to the pointers that it contains. In practice it seems
            // unlikely that either this, or using std::shared_ptr just to avoid it,
            // would be worth it.
            detail::garbage.emplace_back(p);
        }
        Storage* m_data_ptr{};
    };
    std::unique_ptr<std::size_t, deleter> m_ptr;
    template <typename, template <typename> typename, typename...>
    friend struct soa;
    void set_current_row(std::size_t new_row) {
        assert(m_ptr);
        *m_ptr = new_row;
    }
};

}  // namespace neuron::container
template <>
struct std::hash<neuron::container::non_owning_identifier_without_container> {
    std::size_t operator()(
        neuron::container::non_owning_identifier_without_container const& h) noexcept {
        return reinterpret_cast<std::size_t>(h.m_ptr);
    }
};
