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
Model model_data;  // neuron/model_data.hpp
}  // namespace neuron::detail
namespace neuron::cache {
std::optional<Model> model{};
}