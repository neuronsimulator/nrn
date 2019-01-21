#include <memory>
#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "pybind/pyvisitor.hpp"
#include "visitors/nmodl_visitor.hpp"

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


class PyNModlResources {
protected:
    std::unique_ptr<py::detail::pythonbuf> buf;
    std::unique_ptr<std::ostream> ostream;
public:
    PyNModlResources() = default;
    PyNModlResources(py::object object) : buf(new py::detail::pythonbuf(object)),
                                          ostream(new std::ostream(buf.get())) {}
};


class PyNmodlPrintVisitor : private PyNModlResources, public NmodlPrintVisitor {
public:
    using NmodlPrintVisitor::NmodlPrintVisitor;

    PyNmodlPrintVisitor() = default;
    PyNmodlPrintVisitor(std::string filename) : NmodlPrintVisitor(filename) {};
    PyNmodlPrintVisitor(py::object object) : PyNModlResources(object),
                                             NmodlPrintVisitor(*ostream) { };
};


void init_visitor_module(py::module& m) {
    py::module m_visitor = m.def_submodule("visitor");

    py::class_<Visitor, PyVisitor> visitor(m_visitor, "Visitor");
    visitor.def(py::init<>())
    {% for node in nodes %}
        .def("visit_{{ node.class_name | snake_case }}", &Visitor::visit_{{ node.class_name | snake_case }})
        {% if loop.last -%};{% endif %}
    {% endfor %}

    py::class_<AstVisitor, PyAstVisitor> ast_visitor(m_visitor, "AstVisitor");
    ast_visitor.def(py::init<>())
    {% for node in nodes %}
        .def("visit_{{ node.class_name | snake_case }}", &AstVisitor::visit_{{ node.class_name | snake_case }})
        {% if loop.last -%};{% endif %}
    {% endfor %}

    py::class_<PyNmodlPrintVisitor> nmodl_visitor(m_visitor, "NmodlPrintVisitor");
    nmodl_visitor.def(py::init<std::string>());
    nmodl_visitor.def(py::init<py::object>());
    nmodl_visitor.def(py::init<>())
    {% for node in nodes %}
        .def("visit_{{ node.class_name | snake_case }}", &PyNmodlPrintVisitor::visit_{{ node.class_name | snake_case }})
        {% if loop.last -%};{% endif %}
    {% endfor %}


}
#pragma clang diagnostic pop