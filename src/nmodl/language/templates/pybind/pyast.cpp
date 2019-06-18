/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "pybind/pyast.hpp"
#include "visitors/json_visitor.hpp"


/**
 * \file
 * \brief All AST classes for Python bindings
 */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"


// clang-format off
{% macro var(node) -%}
{{ node.class_name | snake_case }}_
{%- endmacro -%}

{% macro args(children) %}
{% for c in children %} {{ c.get_shared_typename() }} {%- if not loop.last %}, {% endif %} {% endfor %}
{%- endmacro -%}
// clang-format on

namespace nmodl {
namespace docstring {

static const char* binary_op_enum = R"(
    Enum type for binary operators in NMODL

    NMODL support different binary operators and this
    type is used to store their value in the AST. See
    nmodl::ast::Ast for details.
)";

static const char* ast_nodetype_enum = R"(
    Enum type for every AST node type

    Every node in the ast has associated type represented by
    this enum class. See nmodl::ast::AstNodeType for details.

)";

static const char* ast_class = R"(
    Base class for all Abstract Syntax Tree node types

    Every node in the Abstract Syntax Tree is inherited from base class
    Ast. This class provides base properties and functions that are implemented
    by base classes.
)";

static const char* accept_method = R"(
    Accept (or visit) the current AST node using current visitor

    Instead of visiting children of AST node, like Ast::visit_children,
    accept allows to visit the current node itself using the concrete
    visitor provided.

    Args:
        v (Visitor):  Concrete visitor that will be used to recursively visit node
)";

static const char* visit_children_method = R"(
    Visit children i.e. member of current AST node using provided visitor

    Different nodes in the AST have different members (i.e. children). This method
    recursively visits children using provided concrete visitor.

    Args:
        v (Visitor):  Concrete visitor that will be used to recursively visit node
)";

static const char* get_node_type_method = R"(
    Return type (ast.AstNodeType) of the ast node
)";

static const char* get_node_type_name_method = R"(
    Return type (ast.AstNodeType) of the ast node as string
)";

static const char* get_node_name_method = R"(
    Return name of the node

    Some ast nodes have a member designated as node name. For example,
    in case of ast.FunctionCall, name of the function is returned as a node
    name. Note that this is different from ast node type name and not every
    ast node has name.
)";

static const char* get_nmodl_name_method = R"(
    Return nmodl statement of the node

    Some ast nodes have a member designated as nmodl name. For example,
    in case of "NEURON { }" the statement of NMODL which is stored as nmodl
    name is "NEURON". This function is only implemented by node types that
    have a nmodl statement.
)";


static const char* clone_method = R"(
    Create a copy of the AST node
)";

static const char* get_token_method = R"(
    Return associated token for the AST node
)";

static const char* get_symbol_table_method = R"(
    Return associated symbol table for the AST node

    Certain ast nodes (e.g. inherited from ast.Block) have associated
    symbol table. These nodes have nmodl.symtab.SymbolTable as member
    and it can be accessed using this method.
)";

static const char* get_statement_block_method = R"(
    Return associated statement block for the AST node

    Top level block nodes encloses all statements in the ast::StatementBlock.
    For example, ast.BreakpointBlock has all statements in curly brace (`{ }`)
    stored in ast.StatementBlock :

    BREAKPOINT {
        SOLVE states METHOD cnexp
        gNaTs2_t = gNaTs2_tbar*m*m*m*h
        ina = gNaTs2_t*(v-ena)
    }

    This method return enclosing statement block.
)";

static const char* negate_method = R"(
    Negate the value of AST node
)";

static const char* set_name_method = R"(
    Set name for the AST node
)";

static const char* is_ast_method = R"(
    Check if current node is of type ast.Ast
)";

static const char* eval_method = R"(
    Return value of the ast node
)";

}  // namespace docstring
}  // namespace nmodl


namespace py = pybind11;
using namespace nmodl::ast;
using nmodl::visitor::JSONVisitor;
using pybind11::literals::operator""_a;


void init_ast_module(py::module& m) {
    py::module m_ast = m.def_submodule("ast", "Abstract Syntax Tree (AST) related implementations");

    py::enum_<BinaryOp>(m_ast, "BinaryOp", docstring::binary_op_enum)
        .value("BOP_ADDITION", BinaryOp::BOP_ADDITION)
        .value("BOP_SUBTRACTION", BinaryOp::BOP_SUBTRACTION)
        .value("BOP_MULTIPLICATION", BinaryOp::BOP_MULTIPLICATION)
        .value("BOP_DIVISION", BinaryOp::BOP_DIVISION)
        .value("BOP_POWER", BinaryOp::BOP_POWER)
        .value("BOP_AND", BinaryOp::BOP_AND)
        .value("BOP_OR", BinaryOp::BOP_OR)
        .value("BOP_GREATER", BinaryOp::BOP_GREATER)
        .value("BOP_LESS", BinaryOp::BOP_LESS)
        .value("BOP_GREATER_EQUAL", BinaryOp::BOP_GREATER_EQUAL)
        .value("BOP_LESS_EQUAL", BinaryOp::BOP_LESS_EQUAL)
        .value("BOP_ASSIGN", BinaryOp::BOP_ASSIGN)
        .value("BOP_NOT_EQUAL", BinaryOp::BOP_NOT_EQUAL)
        .value("BOP_EXACT_EQUAL", BinaryOp::BOP_EXACT_EQUAL)
        .export_values();

    py::enum_<AstNodeType>(m_ast, "AstNodeType", docstring::ast_nodetype_enum)
    // clang-format off
    {% for node in nodes %}
            .value("{{ node.class_name|snake_case|upper }}", AstNodeType::{{ node.class_name|snake_case|upper }}, "AST node of type ast.{{ node.class_name}}")
    {% endfor %}
            .export_values();
    // clang-format on

    py::class_<Ast, PyAst, std::shared_ptr<Ast>> ast_(m_ast, "Ast", docstring::ast_class);
    ast_.def(py::init<>())
        .def("visit_children", &Ast::visit_children, "v"_a, docstring::visit_children_method)
        .def("accept", &Ast::accept, "v"_a, docstring::accept_method)
        .def("get_node_type", &Ast::get_node_type, docstring::get_node_type_method)
        .def("get_node_type_name", &Ast::get_node_type_name, docstring::get_node_type_name_method)
        .def("get_node_name", &Ast::get_node_name, docstring::get_node_name_method)
        .def("get_nmodl_name", &Ast::get_nmodl_name, docstring::get_nmodl_name_method)
        .def("get_token", &Ast::get_token, docstring::get_token_method)
        .def("get_symbol_table",
             &Ast::get_symbol_table,
             py::return_value_policy::reference,
             docstring::get_symbol_table_method)
        .def("get_statement_block",
             &Ast::get_statement_block,
             docstring::get_statement_block_method)
        .def("clone", &Ast::clone, docstring::clone_method)
        .def("negate", &Ast::negate, docstring::negate_method)
        .def("set_name", &Ast::set_name, docstring::set_name_method)
        .def("is_ast", &Ast::is_ast, docstring::is_ast_method)
    // clang-format off
    {% for node in nodes %}
    .def("is_{{ node.class_name | snake_case }}", &Ast::is_{{ node.class_name | snake_case }}, "Check if node is of type ast.{{ node.class_name}}")
    {% if loop.last -%};{% endif %}
    {% endfor %}

    {% for node in nodes %}
    py::class_<{{ node.class_name }}, std::shared_ptr<{{ node.class_name }}>> {{ var(node) }}(m_ast, "{{ node.class_name }}", {{ node.base_class | snake_case }}_);
    {{ var(node) }}.doc() = "{{ node.brief }}";
    {% if node.children %}
    {{ var(node) }}.def(py::init<{{ args(node.children) }}>());
    {% endif %}
    {% if node.is_program_node or node.is_ptr_excluded_node %}
    {{ var(node) }}.def(py::init<>());
    {% endif %}
    // clang-format on

    {{var(node)}}
        .def("__repr__", []({{node.class_name}} & n) {
            std::stringstream ss;
            JSONVisitor v(ss);
            v.compact_json(true);
            n.accept(&v);
            return ss.str();
        });

    // clang-format off
    {% for member in node.public_members() %}
    {{ var(node) }}.def_readwrite("{{ member[1] }}", &{{ node.class_name }}::{{ member[1] }});
    {% endfor %}

    {% for member in node.properties() %}
    {% if member[2] == True %}
        {{ var(node) }}.def_property("{{ member[1] }}", &{{ node.class_name }}::get_{{ member[1] }},
            &{{ node.class_name }}::set_{{ member[1] }});
    {% else %}
        {{ var(node) }}.def_property("{{ member[1] }}", &{{ node.class_name }}::get_{{ member[1] }},
                static_cast<void ({{ node.class_name }}::*)(const {{ member[0] }}&)>(&{{ node.class_name }}::set_{{ member[1] }}));
    {% endif %}
    {% endfor %}

    {{ var(node) }}.def("visit_children", &{{ node.class_name }}::visit_children, docstring::visit_children_method)
    .def("accept", &{{ node.class_name }}::accept, docstring::accept_method)
    .def("clone", &{{ node.class_name }}::clone, docstring::clone_method)
    .def("get_node_type", &{{ node.class_name }}::get_node_type, docstring::get_node_type_method)
    .def("get_node_type_name", &{{ node.class_name }}::get_node_type_name, docstring::get_node_type_name_method)
    {% if node.nmodl_name %}
    .def("get_nmodl_name", &{{ node.class_name }}::get_nmodl_name, docstring::get_nmodl_name_method)
    {% endif %}
    {% if node.is_data_type_node %}
    .def("eval", &{{ node.class_name }}::eval, docstring::eval_method)
    {% endif %}
    .def("is_{{ node.class_name | snake_case }}", &{{ node.class_name }}::is_{{ node.class_name | snake_case }}, "Check if node is of type ast.{{ node.class_name}}");

    {% endfor %}
    // clang-format on
}

#pragma clang diagnostic pop
