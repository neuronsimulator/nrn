#pragma once
#include "membdef.h"
#include "neuron/container/node.hpp"
#include "neuron/container/node_identifier.hpp"
#include "neuron/container/soa_container.hpp"

namespace neuron::container::Node {
/** @brief Underlying storage for all Nodes.
 */
struct storage: soa<storage, interface, field::Area, field::Voltage> {};

/**
 * @brief Non-owning handle to a Node.
 */
using owning_handle = storage::owning_handle;


/**
 * @brief Owning handle to a Node.
 */
using owning_handle = storage::owning_handle;
}  // namespace neuron::container::Node