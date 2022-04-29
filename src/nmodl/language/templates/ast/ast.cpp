/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#include "ast/all.hpp"
#include "symtab/symbol_table.hpp"

/**
 * \file
 * \brief Auto generated AST classes implementations
 */

namespace nmodl {
namespace ast {

///
///  Ast member function definition
///

Ast *Ast::clone() const { throw std::logic_error("clone not implemented"); }

std::string Ast::get_node_name() const {
  throw std::logic_error("get_node_name() not implemented");
}

const std::shared_ptr<StatementBlock>& Ast::get_statement_block() const {
  throw std::runtime_error("get_statement_block not implemented");
}

const ModToken *Ast::get_token() const { return nullptr; }

symtab::SymbolTable *Ast::get_symbol_table() const {
  throw std::runtime_error("get_symbol_table not implemented");
}

void Ast::set_symbol_table(symtab::SymbolTable * /*symtab*/) {
  throw std::runtime_error("set_symbol_table not implemented");
}

void Ast::set_name(const std::string & /*name*/) {
  throw std::runtime_error("set_name not implemented");
}

void Ast::negate() { throw std::runtime_error("negate not implemented"); }

std::shared_ptr<Ast> Ast::get_shared_ptr() {
  return std::static_pointer_cast<Ast>(shared_from_this());
}

std::shared_ptr<const Ast> Ast::get_shared_ptr() const {
  return std::static_pointer_cast<const Ast>(shared_from_this());
}

bool Ast::is_ast() const noexcept { return true; }

{% for node in nodes %}
bool Ast::is_{{ node.class_name | snake_case }} () const noexcept { return false; }

{% endfor %}

Ast* Ast::get_parent() const {
    return parent;
}

void Ast::set_parent(Ast* p) {
    parent = p;
}


    {% for node in nodes %}

    ///
    /// {{node.class_name}} member functions definition
    ///

    {% for child in node.children %}
      {{ child.get_add_methods_definition(node) }}

      {{ child.get_node_name_method_definition(node) }}
    {% endfor %}

    void {{ node.class_name }}::visit_children(visitor::Visitor& v) {
    {% for child in node.non_base_members %}
        {% if child.is_vector %}
        /// visit each element of vector
        for (auto& item : this->{{ child.varname }}) {
            item->accept(v);
        }
        {% elif child.optional %}
        /// optional member could be nullptr
        if (this->{{ child.varname }}) {
            this->{{ child.varname }}->accept(v);
        }
        {% elif child.is_pointer_node %}
        /// use -> for pointer member
        {{ child.varname }}->accept(v);
        {% else %}
        /// use . for object member
        {{ child.varname }}.accept(v);
        {% endif %}
        (void)v;
    {% endfor %}
    }

    void {{ node.class_name }}::visit_children(visitor::ConstVisitor& v) const {
    {% for child in node.non_base_members %}
        {% if child.is_vector %}
        /// visit each element of vector
        for (auto& item : this->{{ child.varname }}) {
            item->accept(v);
        }
        {% elif child.optional %}
        /// optional member could be nullptr
        if (this->{{ child.varname }}) {
            this->{{ child.varname }}->accept(v);
        }
        {% elif child.is_pointer_node %}
        /// use -> for pointer member
        {{ child.varname }}->accept(v);
        {% else %}
        /// use . for object member
        {{ child.varname }}.accept(v);
        {% endif %}
        (void)v;
    {% endfor %}
    }

    void {{ node.class_name }}::accept(visitor::Visitor& v) {
        v.visit_{{ node.class_name | snake_case }}(*this);
    }

    void {{ node.class_name }}::accept(visitor::ConstVisitor& v) const {
        v.visit_{{ node.class_name | snake_case }}(*this);
    }

  {% if node.children %}

    {{ node.ctor_definition() }}

    {% if node.has_ptr_children() %}
        {{ node.ctor_shrptr_definition() }}
    {% endif %}

    /// copy constructor implementation
    {{ node.class_name }}::{{ node.class_name }}(const {{ node.class_name }}& obj) {
        {% for child in node.children %}
            {% if child.is_vector %}
            /// copy each element of vector
            for (auto& item : obj.{{ child.varname }}) {
                this->{{ child.varname }}.emplace_back(item->clone());
            }
            {% elif child.is_pointer_node or child.optional %}
            /// pointer member must be reseted with the new copy
            if (obj.{{ child.varname }}) {
                this->{{ child.varname }}.reset(obj.{{ child.varname }}->clone());
            }
            {% else %}
            /// object member can be just copied by value
            this->{{ child.varname }} = obj.{{ child.varname }};
            {% endif %}
        {% endfor %}

        {% if node.has_token %}
        /// if there is a token, make copy
        if (obj.token) {
            this-> token = std::shared_ptr<ModToken>(obj.token->clone());
        }
        {% endif %}

        /// set parents
        set_parent_in_children();
    }

    {% if node.is_name_node %}
    void {{ node.class_name }}::set_name(const std::string& name) {
        value->set(name);
    }
    {% endif %}

    /// set this parent in the children
    void {{ node.class_name }}::set_parent_in_children() {

        {% for child in node.non_base_members %}
            {% if child.is_vector %}
            /// set parent for each element of the vector
            for (auto& item : {{ child.varname }}) {
                {% if child.optional %}
                if (item) {
                    item->set_parent(this);
                }
                {% else %}
                    item->set_parent(this);
                {%  endif %}

            }
            {% elif child.is_pointer_node or child.optional %}
            /// optional member could be nullptr
            if ({{ child.varname }}) {
                {{ child.varname }}->set_parent(this);
            }
            {% else %}
                {{ child.varname }}.set_parent(this);
            {% endif %}
        {% endfor %}

    }

  {% endif %}

  {% for child in node.children %}
    {{ child.get_setter_method_definition(node.class_name) }}
  {% endfor %}

  {% endfor %}

}  // namespace ast
}  // namespace nmodl

