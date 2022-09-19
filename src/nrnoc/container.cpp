#include "neuron/model_data.hpp"
#include "neuron/container/node_data.hpp"
#include "neuron/container/soa_container_impl.hpp"

// see model_data.hpp
namespace neuron::detail {
Model model_data{};
}
// see soa_container.hpp, this needs to be explicitly instantiated for soa<...>
// types that we use
template void neuron::container::Node::storage::erase(std::size_t);
