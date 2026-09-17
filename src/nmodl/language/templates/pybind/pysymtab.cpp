/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#include "visitors/symtab_visitor.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "ast/all.hpp"
#include "pybind/pybind_utils.hpp"
#include "symtab/symbol.hpp"
#include "symtab/symbol_properties.hpp"
#include "symtab/symbol_table.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

/**
 * \file
 * \brief Symbol Table related Python bindings
 */


namespace nmodl {
namespace docstring {

static const char* sym_decl_type_enum = R"(
    Variable DeclarationType enum
)";

static const char* symbol_class = R"(
    Symbol class
)";

static const char* symbol_table_class = R"(
    SymbolTable class

    Attributes:
        name (str): name of the block
        table (Table): table holding all symbols in the current block
        node (AST): pointer to ast node for which current block symbol created
        global (bool): true if current symbol table is global. blocks like neuron,
                    parameter defines global variables and hence they go into single global symbol table
        parent (SymbolTable): pointer to the symbol table of parent block in the mod file
        children (dict of (str, SymbolTable)): symbol table for each enclosing block in the current nmodl block construct.
)";

static const char* symtabvisitor_class = R"(
    SymtabVisitor class
)";

}  // namespace docstring
}  // namespace nmodl


namespace nb = nanobind;
using namespace nanobind::literals;

using namespace nmodl;
using namespace symtab;
using namespace visitor;


/**
 * \brief Class mirroring nmodl::visitor::SymtabVisitor for Python bindings
 */
class PySymtabVisitor: private VisitorOStreamResources, public SymtabVisitor {
  public:
    explicit PySymtabVisitor(bool update = false)
        : SymtabVisitor(update){};
    PySymtabVisitor(std::string filename, bool update = false)
        : SymtabVisitor(filename, update){};
    PySymtabVisitor(nb::object object, bool update = false)
        : VisitorOStreamResources(object)
        , SymtabVisitor(*ostream, update){};
};


void init_symtab_module(nb::module_& m) {
    nb::module_ m_symtab = m.def_submodule("symtab");

    nb::enum_<syminfo::DeclarationType>(m_symtab,
                                        "DeclarationType",
                                        docstring::sym_decl_type_enum)
        .value("function", syminfo::DeclarationType::function)
        .value("variable", syminfo::DeclarationType::variable)
        .export_values();

    nb::enum_<syminfo::Scope>(m_symtab, "Scope")
        .value("external", syminfo::Scope::external)
        .value("global", syminfo::Scope::global)
        .value("local", syminfo::Scope::local)
        .value("neuron", syminfo::Scope::neuron)
        .export_values();

    nb::enum_<syminfo::Status> e_status(m_symtab, "Status", nb::is_arithmetic(), nb::is_flag());
    e_status.value("empty", syminfo::Status::empty)
        .value("created", syminfo::Status::created)
        .value("from_state", syminfo::Status::from_state)
        .value("globalized", syminfo::Status::globalized)
        .value("inlined", syminfo::Status::inlined)
        .value("localized", syminfo::Status::localized)
        .value("renamed", syminfo::Status::renamed)
        .value("thread_safe", syminfo::Status::thread_safe)
        .export_values();
    e_status.def("__or__", [](const syminfo::Status& x, syminfo::Status y) { return x | y; })
        .def("__and__", [](const syminfo::Status& x, syminfo::Status y) { return x & y; })
        .def("__str__", &syminfo::to_string<syminfo::Status>);

    nb::enum_<syminfo::VariableType>(m_symtab, "VariableType")
        .value("array", syminfo::VariableType::array)
        .value("scalar", syminfo::VariableType::scalar)
        .export_values();

    nb::enum_<syminfo::Access>(m_symtab, "Access")
        .value("read", syminfo::Access::read)
        .value("write", syminfo::Access::write)
        .export_values();

    nb::enum_<syminfo::NmodlType> e_nmodltype(m_symtab, "NmodlType", nb::is_arithmetic(),
                                             nb::is_flag());
    e_nmodltype.value("empty", syminfo::NmodlType::empty)
        .value("argument", syminfo::NmodlType::argument)
        .value("bbcore_pointer_var", syminfo::NmodlType::bbcore_pointer_var)
        .value("constant_var", syminfo::NmodlType::constant_var)
        .value("assigned_definition", syminfo::NmodlType::assigned_definition)
        .value("derivative_block", syminfo::NmodlType::derivative_block)
        .value("discrete_block", syminfo::NmodlType::discrete_block)
        .value("electrode_cur_var", syminfo::NmodlType::electrode_cur_var)
        .value("extern_method", syminfo::NmodlType::extern_method)
        .value("extern_neuron_variable", syminfo::NmodlType::extern_neuron_variable)
        .value("extern_var", syminfo::NmodlType::extern_var)
        .value("vector_def", syminfo::NmodlType::factor_def)
        .value("function_block", syminfo::NmodlType::function_block)
        .value("function_table_block", syminfo::NmodlType::function_table_block)
        .value("global_var", syminfo::NmodlType::global_var)
        .value("kinetic_block", syminfo::NmodlType::kinetic_block)
        .value("linear_block", syminfo::NmodlType::linear_block)
        .value("local_var", syminfo::NmodlType::local_var)
        .value("man_linear_block", syminfo::NmodlType::non_linear_block)
        .value("nonspecific_cur_var", syminfo::NmodlType::nonspecific_cur_var)
        .value("param_assign", syminfo::NmodlType::param_assign)
        .value("pointer_var", syminfo::NmodlType::pointer_var)
        .value("prime_name", syminfo::NmodlType::prime_name)
        .value("procedure_block", syminfo::NmodlType::procedure_block)
        .value("range_var", syminfo::NmodlType::range_var)
        .value("read_ion_var", syminfo::NmodlType::read_ion_var)
        .value("state_var", syminfo::NmodlType::state_var)
        .value("table_assigned_var", syminfo::NmodlType::table_assigned_var)
        .value("table_statement_var", syminfo::NmodlType::table_statement_var)
        .value("to_solve", syminfo::NmodlType::to_solve)
        .value("unit_def", syminfo::NmodlType::unit_def)
        .value("useion", syminfo::NmodlType::useion)
        .value("write_ion_var", syminfo::NmodlType::write_ion_var)
        .export_values();
    e_nmodltype
        .def("__or__", [](const syminfo::NmodlType& x, syminfo::NmodlType y) { return x | y; })
        .def("__and__", [](const syminfo::NmodlType& x, syminfo::NmodlType y) { return x & y; })
        .def("__str__", &syminfo::to_string<syminfo::NmodlType>);


    nb::class_<Symbol> symbol(m_symtab, "Symbol", docstring::symbol_class);
    symbol.def(nb::init<std::string, ast::Ast*>(), "name"_a, "node"_a);
    symbol.def("get_token", &Symbol::get_token)
        .def("is_variable", &Symbol::is_variable)
        .def("is_external_variable", &Symbol::is_external_variable)
        .def("get_id", &Symbol::get_id)
        .def("get_status", &Symbol::get_status)
        .def("get_properties", &Symbol::get_properties)
        .def("get_node", [](const std::shared_ptr<Symbol>& s){ auto n = s->get_nodes(); return n.empty() ? nullptr : n.front(); })
        .def("get_nodes", &Symbol::get_nodes)
        .def("get_original_name", &Symbol::get_original_name)
        .def("get_name", &Symbol::get_name)
        .def("has_any_property", &Symbol::has_any_property)
        .def("has_all_properties", &Symbol::has_all_properties)
        .def("has_any_status", &Symbol::has_any_status)
        .def("has_all_status", &Symbol::has_all_status)
        .def("__str__", &Symbol::to_string);

    nb::class_<SymbolTable> symbol_table(m_symtab, "SymbolTable", docstring::symbol_table_class);
    symbol_table.def(nb::init<std::string, ast::Ast*, bool>(), "name"_a, "node"_a, "global"_a);
    symbol_table.def("name", &SymbolTable::name)
        .def("title", &SymbolTable::title)
        .def("is_method_defined", &SymbolTable::is_method_defined)
        .def("get_variables",
             &SymbolTable::get_variables,
             nb::arg("with") = syminfo::NmodlType::empty,
             nb::arg("without") = syminfo::NmodlType::empty)
        .def("get_variables_with_properties",
             &SymbolTable::get_variables_with_properties,
             nb::arg("properties"),
             nb::arg("all") = false)
        .def("get_variables_with_status",
             &SymbolTable::get_variables_with_status,
             nb::arg("status"),
             nb::arg("all") = false)
        .def("get_parent_table", &SymbolTable::get_parent_table)
        .def("get_parent_table_name", &SymbolTable::get_parent_table_name)
        .def("lookup", &SymbolTable::lookup)
        .def("lookup_in_scope", &SymbolTable::lookup_in_scope)
        .def("insert", &SymbolTable::insert)
        .def("insert_table", &SymbolTable::insert_table)
        .def("__str__", &SymbolTable::to_string);

    nb::class_<SymtabVisitor, AstVisitor, PySymtabVisitor> symtab_visitor(
        m_symtab, "SymtabVisitor", docstring::symtabvisitor_class);
    symtab_visitor.def(nb::init<std::string, bool>(),
                       nb::arg("filename"),
                       nb::arg("update") = false);
    symtab_visitor.def(nb::init<nb::object, bool>(), nb::arg("ostream"), nb::arg("update") = false);
    symtab_visitor.def(nb::init<bool>(), nb::arg("update") = false)
        .def("add_model_symbol_with_property",
             &PySymtabVisitor::add_model_symbol_with_property,
             "node"_a,
             "property"_a)
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
