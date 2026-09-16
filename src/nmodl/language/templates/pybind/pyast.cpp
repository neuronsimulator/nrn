/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#include "pybind/pyast.hpp"
#include "pybind/docstrings.hpp"
#include "visitors/visitor.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

/**
 * \file
 * \brief All AST classes for Python bindings
 */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

namespace nmodl {
namespace ast {
namespace pybind {
{% for setup_pybind_method in setup_pybind_methods %
}
void{{setup_pybind_method}}(nanobind::module_&);
{ % endfor % }
}  // namespace pybind
}  // namespace ast
}  // namespace nmodl

namespace nb = nanobind;
using namespace nmodl::ast;
using namespace nanobind::literals;
namespace docstring = nmodl::docstring;


void init_ast_module(nb::module_& m) {
    nb::module_ m_ast = m.def_submodule("ast",
                                        "Abstract Syntax Tree (AST) related implementations");

    nb::enum_<BinaryOp>(m_ast, "BinaryOp", docstring::binary_op_enum())
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

    nb::enum_<AstNodeType>(m_ast, "AstNodeType", docstring::ast_nodetype_enum())
    // clang-format off
    {% for node in nodes %}
            .value("{{ node.class_name|snake_case|upper }}", AstNodeType::{{ node.class_name|snake_case|upper }}, "AST node of type ast.{{ node.class_name}}")
    {% endfor %}
            .export_values();
    // clang-format on

    nb::class_<Ast>(m_ast, "Ast", docstring::ast_class())
        .def("visit_children",
             static_cast<void (Ast::*)(nmodl::visitor::Visitor&)>(&Ast::visit_children),
             "v"_a,
             docstring::visit_children_method())
        .def("accept",
             static_cast<void (Ast::*)(nmodl::visitor::Visitor&)>(&Ast::accept),
             "v"_a,
             docstring::accept_method())
        .def("accept",
             static_cast<void (Ast::*)(nmodl::visitor::ConstVisitor&) const>(&Ast::accept),
             "v"_a,
             docstring::accept_method())
        .def("get_node_type", &Ast::get_node_type, docstring::get_node_type_method())
        .def("get_node_type_name", &Ast::get_node_type_name, docstring::get_node_type_name_method())
        .def("get_node_name", &Ast::get_node_name, docstring::get_node_name_method())
        .def("get_nmodl_name", &Ast::get_nmodl_name, docstring::get_nmodl_name_method())
        .def("get_token", &Ast::get_token, docstring::get_token_method())
        .def("get_symbol_table",
             &Ast::get_symbol_table,
             nb::rv_policy::reference,
             docstring::get_symbol_table_method())
        .def("get_statement_block",
             &Ast::get_statement_block,
             docstring::get_statement_block_method())
        .def("clone", &Ast::clone, docstring::clone_method())
        .def("negate", &Ast::negate, docstring::negate_method())
        .def("set_name", &Ast::set_name, docstring::set_name_method())
        .def("is_ast", &Ast::is_ast, docstring::is_ast_method())
    // clang-format off
    {% for node in nodes %}
    .def("is_{{ node.class_name | snake_case }}", &Ast::is_{{ node.class_name | snake_case }}, "Check if node is of type ast.{{ node.class_name}}")
    {% endfor %}
        .def_prop_rw("parent", &Ast::get_parent, &Ast::set_parent, docstring::parent_property());

    {% for setup_pybind_method in setup_pybind_methods %}
    nmodl::ast::pybind::{{setup_pybind_method}}(m_ast);
    {% endfor %}
}

#pragma clang diagnostic pop
