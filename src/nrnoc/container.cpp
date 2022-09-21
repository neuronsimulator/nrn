#include "neuron/cache/model_data.hpp"
#include "neuron/model_data.hpp"

// see model_data.hpp
namespace neuron::detail {
Model model_data{};
}
namespace neuron::cache::detail {
std::optional<Model> model_cache{};
}
