#pragma once
#include "section.h"

#include <vector>

namespace neuron::test {
std::vector<double> get_node_voltages(std::vector<Node> const& nodes);
std::tuple<std::vector<Node>, std::vector<double>> get_nodes_and_reference_voltages(
    std::size_t num_nodes = 10);
}  // namespace neuron::test
