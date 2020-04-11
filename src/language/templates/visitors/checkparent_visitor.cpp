/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <string>
#include <fmt/format.h>

#include "visitors/checkparent_visitor.hpp"

namespace nmodl {
namespace visitor {
namespace test {

using namespace fmt::literals;
using namespace ast;

int CheckParentVisitor::check_ast(Ast* node) {

    parent = nullptr;

    node->accept(*this);

    return 0;
}

void CheckParentVisitor::check_parent(ast::Ast* node) const {
    if (!parent) {
        if (is_root_with_null_parent && node->get_parent()) {
            std::string node_type = (node == nullptr) ? "nullptr" : node->get_node_type_name();
            throw std::runtime_error("root->parent: {} is set when it should be nullptr"_format(node_type));
        }
    } else {
        if (parent != node->get_parent()) {
            std::string parent_type = (parent  == nullptr) ? "nullptr" : parent->get_node_type_name();
            std::string node_parent_type = (node->get_parent() == nullptr) ? std::string("nullptr") : node->get_parent()->get_node_type_name();
            throw std::runtime_error("parent: {} and child->parent: {} missmatch"_format(parent_type, node_parent_type));
        }
    }
}


{% for node in nodes %}
void CheckParentVisitor::visit_{{ node.class_name|snake_case }}({{ node.class_name }}* node) {

    // Set this node as parent. and go down the tree
    parent = node;

    // visit its children
    node->visit_children(*this);

    // I am done with these children, I go up the tree. The parent of this node is the parent
    parent = node->get_parent();
}

{% endfor %}

}  // namespace test
}  // namespace visitor
}  // namespace nmodl
