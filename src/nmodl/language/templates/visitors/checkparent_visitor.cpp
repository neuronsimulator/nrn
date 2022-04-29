/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#include "visitors/checkparent_visitor.hpp"

#include <string>

#include "ast/all.hpp"
#include "utils/logger.hpp"

namespace nmodl {
namespace visitor {
namespace test {

using namespace ast;

int CheckParentVisitor::check_ast(const Ast& node) {

    parent = nullptr;

    node.accept(*this);

    return 0;
}

void CheckParentVisitor::check_parent(const ast::Ast& node) const {
    if (!parent) {
        if (is_root_with_null_parent && node.get_parent()) {
            const auto& parent_type = parent->get_node_type_name();
            throw std::runtime_error(
                fmt::format("root->parent: {} is set when it should be nullptr", parent_type));
        }
    } else {
        if (parent != node.get_parent()) {
            const std::string parent_type = (parent  == nullptr) ? "nullptr" : parent->get_node_type_name();
            const std::string node_parent_type = (node.get_parent() == nullptr) ? "nullptr" : node.get_parent()->get_node_type_name();
            throw std::runtime_error(fmt::format("parent: {} and child->parent: {} missmatch",
                                                 parent_type,
                                                 node_parent_type));
        }
    }
}


{% for node in nodes %}
void CheckParentVisitor::visit_{{ node.class_name|snake_case }}(const {{ node.class_name }}& node) {
    // check the node
    check_parent(node);

    // Set this node as parent. and go down the tree
    parent = &node;

    // visit its children
    node.visit_children(*this);

    // I am done with these children, I go up the tree. The parent of this node is the parent
    parent = node.get_parent();
}

{% endfor %}

}  // namespace test
}  // namespace visitor
}  // namespace nmodl
