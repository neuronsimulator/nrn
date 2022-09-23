#include "neuron/cache/model_data.hpp"
#include "neuron/model_data.hpp"

#include <optional>

// see model_data.hpp
namespace neuron::detail {
Model model_data{};
std::optional<ModelCache> model_cache{};
}  // namespace neuron::detail
