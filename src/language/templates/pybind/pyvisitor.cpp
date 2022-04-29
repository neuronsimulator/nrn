/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#include "pybind/pyvisitor.hpp"

#include <memory>
#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "ast/all.hpp"
#include "pybind/pybind_utils.hpp"
#include "visitors/constant_folder_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/kinetic_block_visitor.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/lookup_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/sympy_conductance_visitor.hpp"
#include "visitors/sympy_solver_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

/**
 * \file
 * \brief Visitor related Python bindings
 */

namespace nmodl {
namespace docstring {

static const char* visitor_class = R"(
    Base Visitor class
)";

static const char* ast_visitor_class = R"(
    AstVisitor class
)";

static const char* ast_lookup_visitor_class = R"(
    AstLookupVisitor class

    Attributes:
        types (list of :class:`AstNodeType`): node types to search in the AST
        nodes (list of AST): matching nodes found in the AST
)";

static const char* nmodl_print_visitor_class = R"(
    NmodlPrintVisitor class
)";

static const char* constant_folder_visitor_class = R"(
    ConstantFolderVisitor class
)";

static const char* inline_visitor_class = R"(
    InlineVisitor class
)";

static const char* kinetic_block_visitor_class = R"(
    KineticBlockVisitor class
)";

static const char* local_var_rename_visitor_class = R"(
    LocalVarRenameVisitor class
)";

static const char* sympy_conductance_visitor_class = R"(
    SympyConductanceVisitor class
)";

static const char* sympy_solver_visitor_class = R"(
    SympySolverVisitor class
)";

}  // namespace docstring
}  // namespace nmodl


using namespace pybind11::literals;
namespace py = pybind11;


// clang-format off
{% macro var(node) -%}
    {{ node.class_name | snake_case }}_
{%- endmacro -%}

{% macro args(children) %}
    {% for c in children %} {{ c.get_typename() }} {%- if not loop.last %}, {% endif %} {% endfor %}
{%- endmacro -%}


{% for node in nodes %}
void PyVisitor::visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}& node) {
    PYBIND11_OVERLOAD_PURE(void, Visitor, visit_{{ node.class_name|snake_case }}, std::ref(node));
}
{% endfor %}

{% for node in nodes %}
void PyAstVisitor::visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}& node) {
    PYBIND11_OVERLOAD(void, AstVisitor, visit_{{ node.class_name|snake_case }}, std::ref(node));
}
{% endfor %}

{% for node in nodes %}
void PyConstVisitor::visit_{{ node.class_name|snake_case }}(const ast::{{ node.class_name }}& node) {
    PYBIND11_OVERLOAD_PURE(void, ConstVisitor, visit_{{ node.class_name|snake_case }}, std::cref(node));
}
{% endfor %}

{% for node in nodes %}
void PyConstAstVisitor::visit_{{ node.class_name|snake_case }}(const ast::{{ node.class_name }}& node) {
    PYBIND11_OVERLOAD(void, ConstAstVisitor, visit_{{ node.class_name|snake_case }}, std::cref(node));
}
{% endfor %}
// clang-format on


/**
 * \brief Class mirroring nmodl::NmodlPrintVisitor for Python bindings
 *
 * \details \copydetails nmodl::NmodlPrintVisitor
 *
 * This class is used to interface nmodl::NmodlPrintVisitor with the Python
 * world using `pybind11`.
 */
class PyNmodlPrintVisitor: private VisitorOStreamResources, public NmodlPrintVisitor {
  public:
    using NmodlPrintVisitor::NmodlPrintVisitor;
    using VisitorOStreamResources::flush;

    PyNmodlPrintVisitor() = default;

    PyNmodlPrintVisitor(std::string filename)
        : NmodlPrintVisitor(filename){};

    PyNmodlPrintVisitor(py::object object)
        : VisitorOStreamResources(object)
        , NmodlPrintVisitor(*ostream){};

    // clang-format off
    {% for node in nodes %}
    void visit_{{ node.class_name|snake_case }}(const ast::{{ node.class_name }}& node) override {
        NmodlPrintVisitor::visit_{{ node.class_name|snake_case }}(node);
        flush();
    }
    {% endfor %}
    // clang-format on
};


void init_visitor_module(py::module& m) {
    py::module m_visitor = m.def_submodule("visitor");

    py::class_<Visitor, PyVisitor> visitor(m_visitor, "Visitor", docstring::visitor_class);
    visitor.def(py::init<>())
    // clang-format off
    {% for node in nodes %}
        .def("visit_{{ node.class_name | snake_case }}", &Visitor::visit_{{ node.class_name | snake_case }})
        {% if loop.last -%};{% endif %}
    {% endfor %}  // clang-format on

    py::class_<ConstVisitor, PyConstVisitor> const_visitor(m_visitor, "ConstVisitor", docstring::visitor_class);
    const_visitor.def(py::init<>())
    // clang-format off
    {% for node in nodes %}
    .def("visit_{{ node.class_name | snake_case }}", &ConstVisitor::visit_{{ node.class_name | snake_case }})
    {% if loop.last -%};{% endif %}
    {% endfor %}  // clang-format on

    py::class_<ConstAstVisitor, ConstVisitor, PyConstAstVisitor>
        const_ast_visitor(m_visitor, "ConstAstVisitor", docstring::ast_visitor_class);
    const_ast_visitor.def(py::init<>())
    // clang-format off
    {% for node in nodes %}
        .def("visit_{{ node.class_name | snake_case }}", &ConstAstVisitor::visit_{{ node.class_name | snake_case }})
        {% if loop.last -%};{% endif %}
    {% endfor %} // clang-format on

    py::class_<AstVisitor, Visitor, PyAstVisitor>
            ast_visitor(m_visitor, "AstVisitor", docstring::ast_visitor_class);
    ast_visitor.def(py::init<>())
    // clang-format off
    {% for node in nodes %}
    .def("visit_{{ node.class_name | snake_case }}", &AstVisitor::visit_{{ node.class_name | snake_case }})
    {% if loop.last -%};{% endif %}
    {% endfor %}
    // clang-format on

    py::class_<PyNmodlPrintVisitor, ConstVisitor>
        nmodl_visitor(m_visitor, "NmodlPrintVisitor", docstring::nmodl_print_visitor_class);
    nmodl_visitor.def(py::init<std::string>());
    nmodl_visitor.def(py::init<py::object>());
    nmodl_visitor.def(py::init<>())
    // clang-format off
    {% for node in nodes %}
        .def("visit_{{ node.class_name | snake_case }}", &PyNmodlPrintVisitor::visit_{{ node.class_name | snake_case }})
        {% if loop.last -%};{% endif %}
    {% endfor %}  // clang-format on

    py::class_<AstLookupVisitor, Visitor>
        lookup_visitor(m_visitor, "AstLookupVisitor", docstring::ast_lookup_visitor_class);
    // clang-format off
    lookup_visitor.def(py::init<>())
        .def(py::init<ast::AstNodeType>())
        .def("get_nodes", &AstLookupVisitor::get_nodes)
        .def("clear", &AstLookupVisitor::clear)
        .def("lookup", static_cast<const std::vector<std::shared_ptr<ast::Ast>>& (AstLookupVisitor::*)(ast::Ast&)>(&AstLookupVisitor::lookup))
        .def("lookup", static_cast<const std::vector<std::shared_ptr<ast::Ast>>& (AstLookupVisitor::*)(ast::Ast&, ast::AstNodeType)>(&AstLookupVisitor::lookup))
        .def("lookup", static_cast<const std::vector<std::shared_ptr<ast::Ast>>& (AstLookupVisitor::*)(ast::Ast&, const std::vector<ast::AstNodeType>&)>(&AstLookupVisitor::lookup));

    py::class_<ConstantFolderVisitor, AstVisitor> constant_folder_visitor(m_visitor, "ConstantFolderVisitor", docstring::constant_folder_visitor_class);
    constant_folder_visitor.def(py::init<>())
        .def("visit_program", &ConstantFolderVisitor::visit_program);

    py::class_<InlineVisitor, AstVisitor> inline_visitor(m_visitor, "InlineVisitor", docstring::inline_visitor_class);
    inline_visitor.def(py::init<>())
        .def("visit_program", &InlineVisitor::visit_program);

    py::class_<KineticBlockVisitor, AstVisitor> kinetic_block_visitor(m_visitor, "KineticBlockVisitor", docstring::kinetic_block_visitor_class);
    kinetic_block_visitor.def(py::init<>())
        .def("visit_program", &KineticBlockVisitor::visit_program);

    py::class_<LocalVarRenameVisitor, AstVisitor> local_var_rename_visitor(m_visitor, "LocalVarRenameVisitor", docstring::local_var_rename_visitor_class);
    local_var_rename_visitor.def(py::init<>())
        .def("visit_program", &LocalVarRenameVisitor::visit_program);

    py::class_<SympyConductanceVisitor, AstVisitor> sympy_conductance_visitor(m_visitor, "SympyConductanceVisitor", docstring::sympy_conductance_visitor_class);
    sympy_conductance_visitor.def(py::init<>())
        .def("visit_program", &SympyConductanceVisitor::visit_program);

    py::class_<SympySolverVisitor, AstVisitor> sympy_solver_visitor(m_visitor, "SympySolverVisitor", docstring::sympy_solver_visitor_class);
    sympy_solver_visitor.def(py::init<bool>(), py::arg("use_pade_approx")=false)
        .def("visit_program", &SympySolverVisitor::visit_program);
    // clang-format on
}

#pragma clang diagnostic pop

