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

    VectorMemoryUsage compute_total() const {
        auto total = heavy_data;
        total += stable_identifiers;

        return total;
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

    VectorMemoryUsage compute_total() const {
        auto total = nodes.compute_total();
        total += mechanisms.compute_total();

        return total;
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

    VectorMemoryUsage compute_total() const {
        auto total = threads;
        total += mechanisms;

        return total;
    }
};
}  // namespace cache

/** @brief Overall SoA datastructures related memory usage. */
struct MemoryUsage {
    ModelMemoryUsage model{};
    cache::ModelMemoryUsage cache_model{};
    VectorMemoryUsage stable_pointers{};

    const MemoryUsage& operator+=(const MemoryUsage& other) {
        model += other.model;
        cache_model += other.cache_model;
        stable_pointers += other.stable_pointers;

        return *this;
    }

    VectorMemoryUsage compute_total() const {
        auto total = model.compute_total();
        total += cache_model.compute_total();
        total += stable_pointers;

        return total;
    }
};

struct MemoryUsageSummary {
    /** @brief Data that are part of the algorithm. */
    size_t required{};

    /** @brief Any memory that's (currently) required to run NEURON.
     *
     *  This includes things like the live stable identifiers in each `soa`, the
     *  `cache::Model` and similar things that are needed to implement NEURON
     *  correctly, but are not required by the simulation.
     *
     *  This category covers memory that needed to solve a computer science
     *  problem rather than a neuroscience problem. Hence, this category
     *  could potentially be optimized. It's not obvious how much this category
     *  can be optimized.
     */
    size_t convenient{};

    /** @brief Wasted memory due to the difference of `size` and `capacity`. */
    size_t oversized{};

    /** @brief Essentially leaked memory.
     *
     *  The current implementation doesn't know when it's safe to delete stable
     *  identifiers. Hence, when the owning identifier is deallocated the stable
     *  identifier is kept alive and leaked into a global collector.
     */
    size_t leaked{};

    MemoryUsageSummary(const MemoryUsage& memory_usage) {
        add(memory_usage.model);
        add(memory_usage.cache_model);
        add(leaked, memory_usage.stable_pointers);
    }

  private:
    void add(size_t& accumulator, const VectorMemoryUsage& increment) {
        oversized += increment.capacity - increment.size;
        accumulator += increment.size;
    }

    void add(const StorageMemoryUsage& increment) {
        add(required, increment.heavy_data);
        add(convenient, increment.stable_identifiers);
    }

    void add(const ModelMemoryUsage& model) {
        add(model.nodes);
        add(model.mechanisms);
    }

    void add(const cache::ModelMemoryUsage& model) {
        add(convenient, model.mechanisms);
        add(convenient, model.threads);
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
