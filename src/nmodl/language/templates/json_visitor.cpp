/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/json_visitor.hpp"


namespace nmodl {

using namespace ast;

{% for node in nodes %}
void JSONVisitor::visit_{{ node.class_name|snake_case }}({{ node.class_name }}* node) {
    {% if node.has_children() %}
    printer->push_block(node->get_node_type_name());
    node->visit_children(this);
    {% if node.is_data_type_node %}
            {% if node.is_integer_node %}
    if(!node->get_macro()) {
        std::stringstream ss;
        ss << node->eval();
        printer->add_node(ss.str());
    }
            {% else %}
    std::stringstream ss;
    ss << node->eval();
    printer->add_node(ss.str());
            {% endif %}
        {% endif %}
    printer->pop_block();
        {% if node.is_program_node %}
    flush();
        {% endif %}
    {% else %}
    (void)node;
    printer->add_node("{{ node.class_name }}");
    {% endif %}
}

{% endfor %}

}  // namespace nmodl