#pragma once
#include "neuron/container/node_identifier.hpp"
#include "neuron/container/soa_container.hpp"

namespace neuron::container::Node {
namespace field {
/** @brief Membrane potential.
 */
struct Voltage {
    using type = double;
};
}  // namespace field

/** @brief Underlying storage for all Nodes.
 */
struct storage: soa<identifier, field::Voltage> {};
}  // namespace neuron::container::Node