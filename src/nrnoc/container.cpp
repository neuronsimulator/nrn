#include "neuron/model_data.hpp"

namespace {
void invalidate_cache() {
    neuron::cache::model.reset();
}
}  // namespace
namespace neuron {
Model::Model() {
    m_node_data.set_unsorted_callback(invalidate_cache);
}
void Model::set_unsorted_callback(container::Mechanism::storage& mech_data) {
    mech_data.set_unsorted_callback(invalidate_cache);
}
}  // namespace neuron
namespace neuron::detail {
// See neuron/model_data.hpp
Model model_data;
}  // namespace neuron::detail
namespace neuron::cache {
std::optional<Model> model{};
}
namespace neuron::container::detail {
// See neuron/container/soa_identifier.hpp
std::vector<std::unique_ptr<std::size_t>> garbage;
}  // namespace neuron::container::detail
