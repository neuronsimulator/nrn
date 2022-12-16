#pragma once
#include <optional>
#include <vector>
// Forward declare Datum
namespace neuron::container {
struct generic_data_handle;
}
using Datum = neuron::container::generic_data_handle;
namespace neuron::cache {
struct Mechanism {
    std::vector<std::vector<double*>> pdata{};      // raw pointers for use during simulation
    std::vector<std::vector<Datum*>> pdata_hack{};  // temporary storage used when populating pdata;
                                                    // should go away when pdata are SoA
};
struct Thread {
    std::size_t node_data_offset{};
    std::vector<std::size_t> mechanism_offset{};
};
struct Model {
    std::vector<Thread> thread{};
    std::vector<Mechanism> mechanism{};
};
extern std::optional<Model> model;
}  // namespace neuron::cache
