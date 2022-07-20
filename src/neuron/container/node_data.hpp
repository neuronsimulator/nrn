#pragma once
#include "neuron/container/node_identifier.hpp"
#include "neuron/container/soa_container.hpp"

namespace neuron::container::Node {
namespace field {
struct Voltage {
    using type = double;
};
}  // namespace field

/** @brief Underlying storage for all Nodes.
 */
struct storage: SOAContainer<identifier, field::Voltage> {};
}  // namespace neuron::container::Node