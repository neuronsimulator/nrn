/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/ast_visitor.hpp"


namespace nmodl {

using namespace ast;

{% for node in nodes %}
void AstVisitor::visit_{{ node.class_name|snake_case }}({{ node.class_name }}* node) {
    node->visit_children(this);
}

{% endfor %}

}  // namespace nmodl