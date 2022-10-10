#pragma once
#include "membdef.h"
#include "neuron/container/node_identifier.hpp"
#include "neuron/container/soa_container.hpp"

namespace neuron::container::Node {
namespace field {
/** @brief Area in um^2 but see treesetup.cpp.
 */
struct Area {
    using type = double;
    static constexpr type default_value = 100.;
};

/** @brief Membrane potential.
 */
struct Voltage {
    using type = double;
    static constexpr type default_value = DEF_vrest;
};
}  // namespace field

/** @brief Underlying storage for all Nodes.
 */
using storage = soa<identifier, field::Area, field::Voltage>;

/** @brief Owning identifier for a row in the Node storage;
 */
using owning_identifier = owning_identifier_base<storage, identifier>;
}  // namespace neuron::container::Node