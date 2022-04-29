/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#include "visitors/nmodl_visitor.hpp"

#include "ast/all.hpp"
#include "visitors/nmodl_visitor_helper.ipp"


namespace nmodl {
namespace visitor {

using namespace ast;

{% macro add_element(name) -%}
{% if name %}
    printer->add_element("{{ name }}");
{% endif -%}
{%- endmacro -%}


{%- macro guard(pre, post) -%}
{{ add_element(pre)|trim }}
{{ caller() -}}
{{ add_element(post)|trim }}
{% endmacro -%}


{%- macro is_program(n) -%}
{{ "true" if n.is_program_node else "false" }}
{%- endmacro %}


{%- macro is_statement(n, c) -%}
{{ "true" if c.is_statement_node or n.is_unit_block else "false" }}
{%- endmacro -%}


{%- macro add_vector_child(node, child) -%}
{% call guard(child.prefix, child.suffix) %}
    visit_element(node.get_{{ child.varname }}(),"{{ child.separator }}",{{ is_program(node) }},{{ is_statement(node, child) }});
{% endcall %}
{%- endmacro -%}


{%- macro add_child(node, child) -%}
    {% if node.class_name == node_info.INCLUDE_NODE and child.varname == "blocks" %}
        {# ignore the "blocks" field of Include node #}
    {% elif child.nmodl_name %}
        if(node.get_{{ child.varname }}()->eval()) {
            printer->add_element("{{ child.nmodl_name }}");
        }
    {% elif child.is_vector %}
        {%- if child.prefix or child.suffix %}
            if (!node.get_{{ child.varname }}().empty()) {
                {{ add_vector_child(node, child)|trim }}
            }
        {%- else %}
            {{- add_vector_child(node, child) }}
        {%- endif %}
    {% elif node.is_prime_node and child.varname == node_info.ORDER_VAR_NAME %}
        auto order = node.get_{{ child.varname }}()->eval();
        const std::string symbol(order, '\'');
        printer->add_element(symbol);
    {% elif node.class_name == node_info.BINARY_EXPRESSION_NODE and child.varname == node_info.BINARY_OPERATOR_NAME %}
        std::string op = node.get_op().eval();
        if(op == "=" || op == "&&" || op == "||" || op == "==")
            op = " " + op + " ";
        printer->add_element(op);
    {% else %}
        {% call guard(child.prefix, child.suffix) %}
    {% if child.is_pointer_node %}
        node.get_{{ child.varname }}()->accept(*this);
    {% else %}
        {{ child.class_name }}(node.get_{{ child.varname }}()).accept(*this);
    {%- endif %}
        {% endcall %}
    {%- endif %}
{%- endmacro -%}


{%- for node in nodes %}
void NmodlPrintVisitor::visit_{{ node.class_name|snake_case}}(const {{ node.class_name }}& node) {
    if (is_exclude_type(node.get_node_type())) {
        return;
    }
    {{ add_element(node.nmodl_name) -}}
    {% call guard(node.prefix, node.suffix) -%}
    {% if node.is_block_node %}
        printer->push_level();
    {% endif %}
    {% if node.is_data_type_node %}
        {% if node.is_integer_node %}
            if(node.get_macro() == nullptr) {
                printer->add_element(std::to_string(node.eval()));
            }
        {% elif node.is_float_node or node.is_double_node %}
            std::stringstream ss;
            ss << std::setprecision(16);
            ss << node.eval();
            printer->add_element(ss.str());
        {% else %}
            std::stringstream ss;
            ss << node.eval();
            printer->add_element(ss.str());
        {% endif %}
    {% endif %}
    {% for child in node.children %}
        {% call guard(child.force_prefix, child.force_suffix) -%}
        {% if child.is_base_type_node %}
        {% else %}
            {% if child.optional or child.is_statement_block_node %}
                if(node.get_{{ child.varname }}()) {
                    {{ add_child(node, child)|trim }}
                }
            {% else %}
                {{ add_child(node, child)|trim }}
            {% endif %}
        {% endif %}
        {% endcall -%}
    {% endfor %}
    {% endcall -%}
    {% if node.is_block_node -%}
        printer->pop_level();
    {% endif -%}
}

{% endfor %}

}  // namespace visitor
}  // namespace nmodl

