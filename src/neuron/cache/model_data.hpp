#pragma once
#include <optional>
#include <vector>

#include "neuron/container/memory_usage.hpp"

// Forward declare Datum
namespace neuron::container {
struct generic_data_handle;
}
using Datum = neuron::container::generic_data_handle;
namespace neuron::cache {
struct Mechanism {
    /**
     * @brief Raw pointers into pointer data for use during simulation.
     *
     * pdata_ptr_cache contains pointers to the start of the storage for each pdata variable that is
     * flattened into the pdata member of this struct, and nullptr elsewhere. Compared to using
     * pdata directly this avoids exposing details such as the container used.
     */
    std::vector<double* const*> pdata_ptr_cache{};
    std::vector<std::vector<double*>> pdata{};      // raw pointers for use during simulation
    std::vector<std::vector<Datum*>> pdata_hack{};  // temporary storage used when populating pdata;
                                                    // should go away when pdata are SoA
};
struct Thread {
    /**
     * @brief Offset into global Node storage for this thread.
     */
    std::size_t node_data_offset{};
    /**
     * @brief Offsets into global mechanism storage for this thread (one per mechanism)
     */
    std::vector<std::size_t> mechanism_offset{};
};
struct Model {
    std::vector<Thread> thread{};
    std::vector<Mechanism> mechanism{};
};
extern std::optional<Model> model;
}  // namespace neuron::cache
namespace neuron::container {
cache::ModelMemoryUsage memory_usage(const std::optional<neuron::cache::Model>& model);
cache::ModelMemoryUsage memory_usage(const neuron::cache::Model& model);
}  // namespace neuron::container
