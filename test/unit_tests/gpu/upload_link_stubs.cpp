#include "coreneuron/permute/cellorder.hpp"
#include "neuron/container/soa_identifier.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/model_data.hpp"

// Standalone GPU unit tests link upload.cpp without libnrniv/container.cpp.
namespace neuron {
Model::Model() {
    m_node_data.set_unsorted_callback([]() {
        cache::model.reset();
        gpu::invalidate_device_state();
    });
}
Model::~Model() = default;
}  // namespace neuron

namespace neuron::detail {
Model model_data;
}  // namespace neuron::detail

namespace neuron::container::detail {
void notify_handle_dying(non_owning_identifier_without_container /*handle*/) {}
}  // namespace neuron::container::detail

extern int nrn_nthread = 0;

namespace neuron {
int interleave_permute_type = 0;
InterleaveInfo* interleave_info = nullptr;

#if defined(NRN_ENABLE_GPU)
int interleave_ncell_for_thread(int /*ith*/) {
    return 0;
}
#endif
}  // namespace neuron