#pragma once
#include "membdef.h"
#include "neuron/container/node.hpp"
#include "neuron/container/soa_container.hpp"

namespace neuron::container::Node {
/** @brief Underlying storage for all Nodes.
 */
struct storage: soa<storage, field::Area, field::Voltage> {};

/**
 * @brief Non-owning handle to a Node.
 */
using handle = interface<non_owning_identifier<storage>>;

/**
 * @brief Owning handle to a Node.
 */
using owning_handle = interface<owning_identifier<storage>>;
}  // namespace neuron::container::Node