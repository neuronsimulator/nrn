/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <algorithm>
#include "visitors/lookup_visitor.hpp"


namespace nmodl {

using namespace ast;

{% for node in nodes %}
void AstLookupVisitor::visit_{{ node.class_name|snake_case }}({{ node.class_name }}* node) {
    auto type = node->get_node_type();
    if(std::find(types.begin(), types.end(), type) != types.end()) {
        nodes.push_back(node->get_shared_ptr());
    }
    node->visit_children(this);
}

{% endfor %}


std::vector<std::shared_ptr<ast::AST>> AstLookupVisitor::lookup(AST* node, std::vector<AstNodeType>& _types) {
    nodes.clear();
    types = _types;
    node->accept(this);
    return nodes;
}


std::vector<std::shared_ptr<ast::AST>> AstLookupVisitor::lookup(AST* node, AstNodeType type) {
    nodes.clear();
    types.clear();
    types.push_back(type);
    node->accept(this);
    return nodes;
}

}  // namespace nmodl