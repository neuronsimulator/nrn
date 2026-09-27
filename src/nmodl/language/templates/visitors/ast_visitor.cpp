/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

