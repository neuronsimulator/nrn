/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <memory>
#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "pybind/pybind_utils.hpp"
#include "pybind/pyvisitor.hpp"
#include "visitors/lookup_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/sympy_conductance_visitor.hpp"
#include "visitors/sympy_solver_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using pybind11::literals::operator""_a;

namespace docstring {

static const char *visitor_class = R"(
    Visitor class
)";

static const char *astvisitor_class = R"(
    AstVisitor class
)";

static const char *astlookupvisitor_class = R"(
    AstLookupVisitor class

    Attributes:
        types (list of :class:`AstNodeType`): node types to search in the AST
        nodes (list of AST): matching nodes found in the AST
)";

static const char *nmodlprintvisitor_class = R"(

    NmodlPrintVisitor class
)";

static const char *sympysolvervisitor_class = R"(

    SympySolverVisitor class
)";

static const char *sympyconductancevisitor_class = R"(

    SympyConductanceVisitor class
)";
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
{% macro var(node) -%}
    {{ node.class_name | snake_case }}_
{%- endmacro -%}

{% macro args(children) %}
    {% for c in children %} {{ c.get_typename() }} {%- if not loop.last %}, {% endif %} {% endfor %}
{%- endmacro -%}

namespace py = pybind11;

{% for node in nodes %}
void PyVisitor::visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) {
    PYBIND11_OVERLOAD_PURE(void, Visitor, visit_{{ node.class_name|snake_case }}, node);
}
{% endfor %}

{% for node in nodes %}
void PyAstVisitor::visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) {
    PYBIND11_OVERLOAD(void, AstVisitor, visit_{{ node.class_name|snake_case }}, node);
}
{% endfor %}


class PyNmodlPrintVisitor : private VisitorOStreamResources, public NmodlPrintVisitor {
public:
    using NmodlPrintVisitor::NmodlPrintVisitor;
    using VisitorOStreamResources::flush;

    PyNmodlPrintVisitor() = default;
    PyNmodlPrintVisitor(std::string filename) : NmodlPrintVisitor(filename) {};
    PyNmodlPrintVisitor(py::object object) : VisitorOStreamResources(object),
                                             NmodlPrintVisitor(*ostream) { };

    {% for node in nodes %}
    void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) override {
        NmodlPrintVisitor::visit_{{ node.class_name|snake_case }}(node);
        flush();
    }
    {% endfor %}
};


void init_visitor_module(py::module& m) {
    py::module m_visitor = m.def_submodule("visitor");

    py::class_<Visitor, PyVisitor> visitor(m_visitor, "Visitor", docstring::visitor_class);
    visitor.def(py::init<>())
    {% for node in nodes %}
        .def("visit_{{ node.class_name | snake_case }}", &Visitor::visit_{{ node.class_name | snake_case }})
        {% if loop.last -%};{% endif %}
    {% endfor %}

    py::class_<AstVisitor, Visitor, PyAstVisitor> ast_visitor(m_visitor, "AstVisitor", docstring::astvisitor_class);
    ast_visitor.def(py::init<>())
    {% for node in nodes %}
        .def("visit_{{ node.class_name | snake_case }}", &AstVisitor::visit_{{ node.class_name | snake_case }})
        {% if loop.last -%};{% endif %}
    {% endfor %}

    py::class_<PyNmodlPrintVisitor, Visitor> nmodl_visitor(m_visitor, "NmodlPrintVisitor", docstring::nmodlprintvisitor_class);
    nmodl_visitor.def(py::init<std::string>());
    nmodl_visitor.def(py::init<py::object>());
    nmodl_visitor.def(py::init<>())
    {% for node in nodes %}
        .def("visit_{{ node.class_name | snake_case }}", &PyNmodlPrintVisitor::visit_{{ node.class_name | snake_case }})
        {% if loop.last -%};{% endif %}
    {% endfor %}

    py::class_<AstLookupVisitor, Visitor> lookup_visitor(m_visitor, "AstLookupVisitor", docstring::astlookupvisitor_class);
    lookup_visitor.def(py::init<>())
        .def(py::init<ast::AstNodeType>())
        .def("get_nodes", &AstLookupVisitor::get_nodes)
        .def("clear", &AstLookupVisitor::clear)
        .def("lookup", (std::vector<std::shared_ptr<ast::AST>> (AstLookupVisitor::*)(ast::AST*, ast::AstNodeType)) &AstLookupVisitor::lookup)
        .def("lookup", (std::vector<std::shared_ptr<ast::AST>> (AstLookupVisitor::*)(ast::AST*, std::vector<ast::AstNodeType>&)) &AstLookupVisitor::lookup)
    {% for node in nodes %}
        .def("visit_{{ node.class_name | snake_case }}", &AstLookupVisitor::visit_{{ node.class_name | snake_case }})
        {% if loop.last -%};{% endif %}
    {% endfor %}

    py::class_<SympySolverVisitor, AstVisitor> sympy_solver_visitor(m_visitor, "SympySolverVisitor", docstring::sympysolvervisitor_class);
    sympy_solver_visitor.def(py::init<bool>(), py::arg("use_pade_approx")=false)
        .def("visit_program", &SympySolverVisitor::visit_program);

    py::class_<SympyConductanceVisitor, AstVisitor> sympy_conductance_visitor(m_visitor, "SympyConductanceVisitor", docstring::sympyconductancevisitor_class);
    sympy_conductance_visitor.def(py::init<>())
        .def("visit_program", &SympyConductanceVisitor::visit_program);
}

#pragma clang diagnostic pop
