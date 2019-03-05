/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/stl.h>

#include "ast/ast.hpp"
#include "pybind/pybind_utils.hpp"
#include "symtab/symbol_properties.hpp"
#include "symtab/symbol.hpp"
#include "symtab/symbol_table.hpp"
#include "visitors/symtab_visitor.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
{% macro var(node) -%}
    {{ node.class_name | snake_case }}_
{%- endmacro -%}

{% macro args(children) %}
    {% for c in children %} {{ c.get_typename() }} {%- if not loop.last %}, {% endif %} {% endfor %}
{%- endmacro -%}

namespace py = pybind11;
using namespace nmodl;
using namespace symtab;


class PySymtabVisitor : private VisitorOStreamResources, public SymtabVisitor {
public:
    using SymtabVisitor::SymtabVisitor;

    explicit PySymtabVisitor(bool update = false) : SymtabVisitor(update) { };
    PySymtabVisitor(std::string filename, bool update = false) : SymtabVisitor(filename, update) {};
    PySymtabVisitor(py::object object, bool update = false) : VisitorOStreamResources(object),
                                             SymtabVisitor(*ostream, update) { };
};


void init_symtab_module(py::module& m) {
    py::module m_symtab = m.def_submodule("symtab");

    py::enum_<syminfo::DeclarationType>(m_symtab, "DeclarationType")
            .value("function", syminfo::DeclarationType::function)
            .value("variable", syminfo::DeclarationType::variable)
            .export_values();

    py::enum_<syminfo::Scope>(m_symtab, "Scope")
            .value("external", syminfo::Scope::external)
            .value("global", syminfo::Scope::global)
            .value("local", syminfo::Scope::local)
            .value("neuron", syminfo::Scope::neuron)
            .export_values();

    py::enum_<syminfo::Status> e_status(m_symtab, "Status", py::arithmetic());
    e_status.value("created", syminfo::Status::created)
            .value("from_state", syminfo::Status::from_state)
            .value("globalized", syminfo::Status::globalized)
            .value("inlined", syminfo::Status::inlined)
            .value("localized", syminfo::Status::localized)
            .value("renamed", syminfo::Status::renamed)
            .value("thread_safe", syminfo::Status::thread_safe)
            .export_values();
    e_status.def("__or__",[](const syminfo::Status& x, syminfo::Status y) {
                return x | y;
            })
            .def("__and__",[](const syminfo::Status& x, syminfo::Status y) {
                return x & y;
            })
            .def("__str__", &syminfo::to_string<syminfo::Status>);

    py::enum_<syminfo::VariableType>(m_symtab, "VariableType")
            .value("array", syminfo::VariableType::array)
            .value("scalar", syminfo::VariableType::scalar)
            .export_values();

    py::enum_<syminfo::Access>(m_symtab, "Access")
            .value("read", syminfo::Access::read)
            .value("write", syminfo::Access::write)
            .export_values();

    py::enum_<syminfo::NmodlType> e_nmodltype(m_symtab, "NmodlType", py::arithmetic());
    e_nmodltype.value("argument", syminfo::NmodlType::argument)
            .value("bbcore_pointer_var", syminfo::NmodlType::bbcore_pointer_var)
            .value("constant_var", syminfo::NmodlType::constant_var)
            .value("dependent_def", syminfo::NmodlType::dependent_def)
            .value("derivative_block", syminfo::NmodlType::derivative_block)
            .value("discrete_block", syminfo::NmodlType::discrete_block)
            .value("electrode_cur_var", syminfo::NmodlType::electrode_cur_var)
            .value("extern_method", syminfo::NmodlType::extern_method)
            .value("extern_neuron_variable", syminfo::NmodlType::extern_neuron_variable)
            .value("extern_var", syminfo::NmodlType::extern_var)
            .value("vactor_def", syminfo::NmodlType::factor_def)
            .value("function_block", syminfo::NmodlType::function_block)
            .value("function_table_block", syminfo::NmodlType::function_table_block)
            .value("global_var", syminfo::NmodlType::global_var)
            .value("kinetic_block", syminfo::NmodlType::kinetic_block)
            .value("linear_block", syminfo::NmodlType::linear_block)
            .value("local_var", syminfo::NmodlType::local_var)
            .value("man_linear_block", syminfo::NmodlType::non_linear_block)
            .value("nonspecifig_cur_var", syminfo::NmodlType::nonspecific_cur_var)
            .value("param_assign", syminfo::NmodlType::param_assign)
            .value("partial_block", syminfo::NmodlType::partial_block)
            .value("pointer_var", syminfo::NmodlType::pointer_var)
            .value("prime_name", syminfo::NmodlType::prime_name)
            .value("procedure_block", syminfo::NmodlType::procedure_block)
            .value("range_var", syminfo::NmodlType::range_var)
            .value("read_ion_var", syminfo::NmodlType::read_ion_var)
            .value("section_var", syminfo::NmodlType::section_var)
            .value("state_var", syminfo::NmodlType::state_var)
            .value("table_dependent_var", syminfo::NmodlType::table_dependent_var)
            .value("table_statement_var", syminfo::NmodlType::table_statement_var)
            .value("to_solve", syminfo::NmodlType::to_solve)
            .value("unit_def", syminfo::NmodlType::unit_def)
            .value("useion", syminfo::NmodlType::useion)
            .value("write_ion_var", syminfo::NmodlType::write_ion_var)
            .export_values();
    e_nmodltype.def("__or__",[](const syminfo::NmodlType& x, syminfo::NmodlType y) {
                return x | y;
            })
            .def("__and__",[](const syminfo::NmodlType& x, syminfo::NmodlType y) {
                return x & y;
            })
            .def("__str__", &syminfo::to_string<syminfo::NmodlType>);


    py::class_<Symbol, std::shared_ptr<Symbol>> symbol(m_symtab, "Symbol");
    symbol.def(py::init<std::string, ast::AST*>());
    symbol.def("get_token", &Symbol::get_token)
            .def("is_variable", &Symbol::is_variable)
            .def("is_external_symbol_only", &Symbol::is_external_symbol_only)
            .def("get_id", &Symbol::get_id)
            .def("get_status", &Symbol::get_status)
            .def("get_properties", &Symbol::get_properties)
            .def("get_node", &Symbol::get_node)
            .def("get_original_name", &Symbol::get_original_name)
            .def("get_name", &Symbol::get_name)
            .def("has_properties", &Symbol::has_properties)
            .def("has_all_properties", &Symbol::has_all_properties)
            .def("has_any_status", &Symbol::has_any_status)
            .def("has_all_status", &Symbol::has_all_status)
            .def("__str__", &Symbol::to_string);

    py::class_<SymbolTable> symbol_table(m_symtab, "SymbolTable");
    symbol_table.def(py::init<std::string, ast::AST*, bool>());
    symbol_table.def("name", &SymbolTable::name)
            .def("type", &SymbolTable::type)
            .def("title", &SymbolTable::title)
            .def("is_method_defined", &SymbolTable::is_method_defined)
            .def("get_variables", &SymbolTable::get_variables)
            .def("get_variables_with_properties", &SymbolTable::get_variables_with_properties,
                    py::arg("properties"), py::arg("all") = false)
            .def("get_variables_with_status", &SymbolTable::get_variables_with_status,
                    py::arg("status"), py::arg("all") = false)
            .def("get_parent_table", &SymbolTable::get_parent_table)
            .def("get_parent_table_name", &SymbolTable::get_parent_table_name)
            .def("lookup", &SymbolTable::lookup)
            .def("lookup_in_scope", &SymbolTable::lookup_in_scope)
            .def("insert", &SymbolTable::insert)
            .def("insert_table", &SymbolTable::insert_table)
            .def("__str__", &SymbolTable::to_string);

    py::class_<SymtabVisitor, AstVisitor, PySymtabVisitor> symtab_visitor(m_symtab,
                                                                          "SymtabVisitor");
    symtab_visitor.def(py::init<std::string, bool>(), py::arg("filename"),
                       py::arg("update") = false);
    symtab_visitor.def(py::init<py::object, bool>(), py::arg("ostream"), py::arg("update") = false);
    symtab_visitor.def(py::init<bool>(), py::arg("update") = false)
            .def("add_model_symbol_with_property", &PySymtabVisitor::add_model_symbol_with_property)
            .def("setup_symbol", &PySymtabVisitor::setup_symbol)
            .def("setup_symbol_table", &PySymtabVisitor::setup_symbol_table)
            .def("setup_symbol_table_for_program_block",
                 &PySymtabVisitor::setup_symbol_table_for_program_block)
            .def("setup_symbol_table_for_global_block",
                 &PySymtabVisitor::setup_symbol_table_for_global_block)
            .def("setup_symbol_table_for_scoped_block",
                 &PySymtabVisitor::setup_symbol_table_for_scoped_block)
    {% for node in nodes %}
        .def("visit_{{ node.class_name | snake_case }}", &PySymtabVisitor::visit_{{ node.class_name | snake_case }})
        {% if loop.last -%};{% endif %}
    {% endfor %}

}

#pragma clang diagnostic pop
