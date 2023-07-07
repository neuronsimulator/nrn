/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visitors/var_usage_visitor.hpp"

#include <utility>

#include "ast/name.hpp"


namespace nmodl {
namespace visitor {

/// rename matching variable
void VarUsageVisitor::visit_name(const ast::Name& node) {
    const auto& name = node.get_node_name();
    if (name == var_name) {
        used = true;
    }
}

bool VarUsageVisitor::variable_used(const ast::Node& node, std::string name) {
    used = false;
    var_name = std::move(name);
    node.visit_children(*this);
    return used;
}

}  // namespace visitor
}  // namespace nmodl
