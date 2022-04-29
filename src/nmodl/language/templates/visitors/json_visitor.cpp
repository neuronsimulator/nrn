/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#include "visitors/json_visitor.hpp"

#include "ast/all.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {

using namespace ast;

{% for node in nodes %}
void JSONVisitor::visit_{{ node.class_name|snake_case }}(const {{ node.class_name }}& node) {
    {% if node.has_children() %}
    printer->push_block(node.get_node_type_name());
    if (embed_nmodl) {
        printer->add_block_property("nmodl", to_nmodl(node));
    }
    node.visit_children(*this);
    {% if node.is_data_type_node %}
            {% if node.is_integer_node %}
    if(!node.get_macro()) {
        std::stringstream ss;
        ss << node.eval();
        printer->add_node(ss.str());
    }
            {% else %}
    std::stringstream ss;
    ss << node.eval();
    printer->add_node(ss.str());
            {% endif %}
        {% endif %}
    printer->pop_block();
        {% if node.is_program_node %}
    if (node.get_parent() == nullptr) {
        flush();
    }
        {% endif %}
    {% else %}
    (void)node;
    printer->add_node("{{ node.class_name }}");
    {% endif %}
}

{% endfor %}

}  // namespace visitor
}  // namespace nmodl

