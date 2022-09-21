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
using storage = soa<identifier, field::Voltage>;

/** @brief Owning identifier for a row in the Node storage;
 */
using owning_identifier = owning_identifier_base<storage, identifier>;
}  // namespace neuron::container::Node