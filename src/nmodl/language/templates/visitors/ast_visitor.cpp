/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#include "visitors/ast_visitor.hpp"

#include "ast/all.hpp"


namespace nmodl {
namespace visitor {

using namespace ast;

{% for node in nodes %}
void AstVisitor::visit_{{ node.class_name|snake_case }}({{ node.class_name }}& node) {
    node.visit_children(*this);
}
{% endfor %}

{% for node in nodes %}
void ConstAstVisitor::visit_{{ node.class_name|snake_case }}(const {{ node.class_name }}& node) {
    node.visit_children(*this);
}
{% endfor %}

}  // namespace visitor
}  // namespace nmodl

