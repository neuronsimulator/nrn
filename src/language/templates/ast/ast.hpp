/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ast/ast_decl.hpp"
#include "ast/ast_common.hpp"
#include "lexer/modtoken.hpp"
#include "utils/common_utils.hpp"
#include "visitors/visitor.hpp"

{% macro virtual(node) -%}
    {% if node.is_abstract %} virtual {% endif %}
{% endmacro %}

{% macro override(node) %}
    {% if not node.is_abstract %} override {% endif %}
{% endmacro %}


namespace nmodl {
namespace ast {

    {% for node in nodes %}
    /**
     * \brief {{ node.brief }}
     *
     * {{- node.get_description() -}}
     */
    class {{ node.class_name }} : public {{ node.base_class }} {
    {% if node.private_members() %}
      private:
        {% for member in node.private_members() %}
        {{ '/// ' + member[2] }}
        {{ member[0] }} {{ member[1] }};
        {% endfor %}
    {% endif %}
      public:
        {% for member in node.public_members() %}
        {{ '/// ' + member[2] }}
        {{ member[0] }} {{ member[1] }};
        {% endfor %}

        {% if node.children %}
        {{ node.ctor_declaration() }}
        {% if node.has_ptr_children() %}
            {{ node.ctor_shrptr_declaration() }}
        {% endif %}
        {{ node.class_name }}(const {{ node.class_name }}& obj);
        {% endif %}

        {% if node.requires_default_constructor %}
        {{ node.class_name}}() = default;
        {% endif %}

        virtual ~{{ node.class_name }}() {}

        {% for child in node.children %}
        {{ child.get_add_methods() }}

        {{ child.get_node_name_method() }}

        {{ child.get_getter_method() }}

        {{ child.get_setter_method() }}

        {% endfor %}

        {{ virtual(node) }} void visit_children (Visitor* v) override;

        {{ virtual(node) }} void accept(Visitor* v) override { v->visit_{{ node.class_name | snake_case }}(this); }

        {{ virtual(node) }} {{ node.class_name }}* clone() override { return new {{ node.class_name }}(*this); }

        {{ virtual(node) }} std::shared_ptr<AST> get_shared_ptr() override {
            return std::static_pointer_cast<{{ node.class_name }}>(shared_from_this());
        }

        {{ virtual(node) }} AstNodeType get_node_type() override { return AstNodeType::{{ node.ast_enum_name }}; }

        {{ virtual(node) }} std::string get_node_type_name() override { return "{{ node.class_name }}"; }

        bool is_{{ node.class_name | snake_case }} () override { return true; }

        {% if node.has_token %}
        {{ virtual(node) }}ModToken* get_token(){{ override(node) }} { return token.get(); }

        void set_token(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
        {% endif %}

        {% if node.is_symtab_needed %}
        void set_symbol_table(symtab::SymbolTable* newsymtab) override { symtab = newsymtab; }

        symtab::SymbolTable* get_symbol_table() override { return symtab; }
        {% endif %}

        {% if node.is_program_node %}
        symtab::ModelSymbolTable* get_model_symbol_table() { return &model_symtab; }
        {% endif %}

        {% if node.is_base_class_number_node %}
        void negate() override { value = {{ node.negation }}value; }

        double to_double() override { return value; }
        {% endif %}

        {% if node.is_name_node %}
        {{ virtual(node) }}void set_name(std::string name){{ override(node) }} { value->set(name); }
        {% endif %}

        {% if node.is_number_node %}
        {{ virtual(node) }}double to_double() { throw std::runtime_error("to_double not implemented"); }
        {% endif %}

        {% if node.is_base_block_node %}
        virtual ArgumentVector get_parameters() { throw std::runtime_error("get_parameters not implemented"); }
        {% endif %}

        {% if node.is_data_type_node %}
            {# if node is of enum type then return enum value #}
            {% if node.is_enum_node %}
                std::string eval() { return {{ node.get_data_type_name() }}Names[value]; }
            {# But if basic data type then eval return their value #}
            {% else %}
                {{ node.get_data_type_name() }} eval() { return value; }

                void set({{ node.get_data_type_name() }} _value) { value = _value; }
            {% endif %}
        {% endif %}
    };

    {% endfor %}

}  // namespace ast
}  // namespace nmodl
