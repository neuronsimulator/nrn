/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

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
