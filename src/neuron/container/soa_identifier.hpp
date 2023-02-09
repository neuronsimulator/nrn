#pragma once
#include "backtrace_utils.h"
#include "neuron/container/non_owning_soa_identifier.hpp"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>
#include <vector>

namespace neuron::container {
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

    using storage_type = Storage;

  private:
    Storage* m_storage;
};

namespace detail {
void notify_handle_dying(non_owning_identifier_without_container);
}

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
        : owning_identifier{} {
        // The default constructor has finished, so *this is a valid object.
        auto tmp = storage.acquire_owning_identifier();
        swap(*this, tmp);
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
        std::swap(first.m_ptr, second.m_ptr);
    }

    friend std::ostream& operator<<(std::ostream& os, owning_identifier const& oi) {
        return os << "owning " << non_owning_identifier<Storage>{oi};
    }

    using storage_type = Storage;

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
            assert(data_container.at(*p) == p);
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
            *p = invalid_row;
            // This is to provide compatibility with NEURON's old nrn_notify_when_double_freed and
            // nrn_notify_when_void_freed methods.
            detail::notify_handle_dying(p);
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
    template <typename, typename...>
    friend struct soa;
    void set_current_row(std::size_t new_row) {
        assert(m_ptr);
        *m_ptr = new_row;
    }
    /**
     * @brief Create a non-null owning identifier that owns the given row.
     *
     * This is used inside
     * neuron::container::soa<...>::acquire_owning_identifier() and should not
     * be used without great care.
     */
    owning_identifier(Storage& storage, std::size_t row)
        : m_ptr{new std::size_t, storage} {
        *m_ptr = row;
    }
};
}  // namespace neuron::container
