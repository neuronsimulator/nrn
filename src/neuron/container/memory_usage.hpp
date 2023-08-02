#pragma once

#include <cstdio>
#include <vector>
#include <ostream>
#include <cmath>
#include <sstream>

namespace neuron::container {
/** @brief Size and capacity in bytes. */
struct VectorMemoryUsage {
    VectorMemoryUsage() = default;
    VectorMemoryUsage(size_t size, size_t capacity)
        : size(size)
        , capacity(capacity) {}

    /** Compute the memory requirements of the `std::vector`.
     *
     * Note, this returns the size and capacity of the memory allocated by
     * the `std::vector`. If the element allocate memory, that memory
     * isn't included.
     *
     * Essentially,
     *
     *     size = vec.size() * sizeof(T);
     *     capacity = vec.capacity() * sizeof(T);
     */
    template <class T, class A>
    VectorMemoryUsage(const std::vector<T, A>& vec)
        : size(vec.size() * sizeof(T))
        , capacity(vec.capacity() * sizeof(T)) {}

    /// @brief Number of bytes used.
    size_t size{};

    /// @brief Number of bytes allocated.
    size_t capacity{};

    const VectorMemoryUsage& operator+=(const VectorMemoryUsage& other) {
        size += other.size;
        capacity += other.capacity;

        return *this;
    }
};


/** @brief Memory usage of a storage/soa container. */
struct StorageMemoryUsage {
    /// @brief The memory usage of the heavy data in a soa.
    VectorMemoryUsage heavy_data{};

    /// @brief The memory usage for the stable identifiers in a soa.
    VectorMemoryUsage stable_identifiers{};

    const StorageMemoryUsage& operator+=(const StorageMemoryUsage& other) {
        heavy_data += other.heavy_data;
        stable_identifiers += other.stable_identifiers;

        return *this;
    }
};

/** @brief Memory usage of a `neuron::Model`. */
struct ModelMemoryUsage {
    /// @brief The memory usage of the nodes related data.
    StorageMemoryUsage nodes{};

    /// @brief The memory usage of all mechanisms.
    StorageMemoryUsage mechanisms{};

    const ModelMemoryUsage& operator+=(const ModelMemoryUsage& other) {
        nodes += other.nodes;
        mechanisms += other.mechanisms;

        return *this;
    }
};

namespace cache {
/** @brief Memory usage of a `neuron::cache::Model`. */
struct ModelMemoryUsage {
    /** @brief Memory usage required for NRN threads. */
    VectorMemoryUsage threads{};

    /** @brief Memory usage related to caching mechanisms. */
    VectorMemoryUsage mechanisms{};

    const ModelMemoryUsage& operator+=(const ModelMemoryUsage& other) {
        threads += other.threads;
        mechanisms += other.mechanisms;

        return *this;
    }
};
}  // namespace cache

/** @brief Overall SoA datastructures related memory usage. */
struct MemoryUsage {
    ModelMemoryUsage model{};
    cache::ModelMemoryUsage cache_model{};
    VectorMemoryUsage stable_pointers{};
    VectorMemoryUsage stable_identifiers{};

    const MemoryUsage& operator+=(const MemoryUsage& other) {
        model += other.model;
        cache_model += other.cache_model;
        stable_pointers += other.stable_pointers;
        stable_identifiers += other.stable_identifiers;

        return *this;
    }
};

/** @brief */
struct MemoryStats {
    MemoryUsage total;
};

/** @brief Gather memory usage of this process. */
MemoryUsage local_memory_usage();

/** @brief Utility to format memory as a human readable string.
 *
 *  Note, this is currently tailored to it's use in `format_memory_usage`
 *  and is therefore very rigid in it's padding. Generalize when needed.
 *
 *  @internal
 */
std::string format_memory(size_t bytes);

/** @brief Aligned, human readable representation of `memory_usage`.
 *
 *  @internal
 */
std::string format_memory_usage(const VectorMemoryUsage& memory_usage);

/** @brief Create a string representation of `usage`. */
std::string format_memory_usage(const MemoryUsage& usage);

void print_memory_usage(const MemoryUsage& usage);

}  // namespace neuron::container
