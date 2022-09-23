#include "neuron/cache/model_data.hpp"
#include "neuron/model_data.hpp"

#include <optional>

// see model_data.hpp
namespace neuron::detail {
Model model_data{};
std::optional<ModelCache> model_cache{};
}  // namespace neuron::detail

namespace neuron {
void invalidate_model_cache() {
    detail::model_cache = std::nullopt;
}
Model::Model() {
    m_node_data.set_unsorted_callback(invalidate_model_cache);
}
}  // namespace neuron