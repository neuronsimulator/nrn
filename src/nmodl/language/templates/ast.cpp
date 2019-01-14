#include "ast/ast.hpp"
#include "symtab/symbol_table.hpp"

{% macro arglist(members) -%}
    {%- for type, var in members %} {{ type }} {{ var }} {%- if not loop.last %}, {% endif %} {% endfor -%}
{%- endmacro %}

{% macro initlist(members) -%}
{%- for type, var in members %} {{ var }}({{ var }}) {%- if not loop.last %}, {% endif %} {% endfor -%}
{%- endmacro %}

namespace ast {

    {% for node in nodes %}

    /* visit method for {{ node.class_name }} ast node */
    void {{ node.class_name }}::visit_children(Visitor* v) {
    {% for child in node.non_base_members %}
        {% if child.is_vector %}
        for (auto& item : this->{{ child.varname }}) {
            item->accept(v);
        }
        {% elif child.optional %}
        if (this->{{ child.varname }}) {
            this->{{ child.varname }}->accept(v);
        }
        {% elif child.is_pointer_node() %}
        {{ child.varname }}->accept(v);
        {% else %}
        {{ child.varname }}.accept(v);
        {% endif %}
        (void)v;
    {% endfor %}
    }

    {% if node.members %}
    /* constructor for {{ node.class_name }} ast node */
    {{ node.class_name }}::{{ node.class_name }}({{ arglist(node.members) }})
    :  {{ initlist(node.members) }} {}

    /* copy constructor for {{ node.class_name }} ast node */
    {{ node.class_name }}::{{ node.class_name }}(const {{ node.class_name }}& obj) {

        {% for child in node.children %}
            {% if child.is_vector %}
            for (auto& item : obj.{{ child.varname }}) {
                this->{{ child.varname }}.emplace_back(item->clone());
            }
            {% elif child.is_pointer_node() or child.optional %}
            if (obj.{{ child.varname }}) {
                this->{{ child.varname }}.reset(obj.{{ child.varname }}->clone());
            }
            {% else %}
            this->{{ child.varname }} = obj.{{ child.varname }};
            {% endif %}
        {% endfor %}
        {% if node.has_token %}
        if (obj.token) {
            this-> token = std::shared_ptr<ModToken>(obj.token->clone());
        }
        {% endif %}
    }

    {% endif %}

    {% endfor %}

}
