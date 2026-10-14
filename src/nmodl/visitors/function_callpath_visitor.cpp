
/*
 * Copyright 2024 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visitors/function_callpath_visitor.hpp"

namespace nmodl {
namespace visitor {

using symtab::Symbol;
using symtab::syminfo::NmodlType;

void FunctionCallpathVisitor::visit_var_name(const ast::VarName& node) {
    if (visited_functions_or_procedures.empty()) {
        return;
    }
    /// If node is either a RANGE var, a POINTER or a BBCOREPOINTER then
    /// the FUNCTION or PROCEDURE it's used in should have the `use_range_ptr_var`
    /// property
    auto sym = psymtab->lookup(node.get_node_name());
    const auto properties = NmodlType::range_var | NmodlType::pointer_var |
                            NmodlType::bbcore_pointer_var;
    if (sym && sym->has_any_property(properties)) {
        const auto top = visited_functions_or_procedures.back();
        const auto caller_func_name =
            top->is_function_block()
                ? dynamic_cast<const ast::FunctionBlock*>(top)->get_node_name()
                : dynamic_cast<const ast::ProcedureBlock*>(top)->get_node_name();
        auto caller_func_proc_sym = psymtab->lookup(caller_func_name);
        caller_func_proc_sym->add_properties(NmodlType::use_range_ptr_var);
    }
}

void FunctionCallpathVisitor::visit_function_call(const ast::FunctionCall& node) {
    if (visited_functions_or_procedures.empty()) {
        return;
    }
    const auto name = node.get_node_name();
    const auto func_symbol = psymtab->lookup(name);
    if (!func_symbol ||
        !func_symbol->has_any_property(NmodlType::function_block | NmodlType::procedure_block) ||
        func_symbol->get_nodes().empty()) {
        return;
    }
    /// Visit the called FUNCTION/PROCEDURE AST node to check whether
    /// it has `use_range_ptr_var` property. If it does the currently called
    /// function needs to have it too.
    const auto func_block = func_symbol->get_nodes()[0];
    func_block->accept(*this);
    if (func_symbol->has_any_property(NmodlType::use_range_ptr_var)) {
        const auto top = visited_functions_or_procedures.back();
        auto caller_func_name =
            top->is_function_block()
                ? dynamic_cast<const ast::FunctionBlock*>(top)->get_node_name()
                : dynamic_cast<const ast::ProcedureBlock*>(top)->get_node_name();
        auto caller_func_proc_sym = psymtab->lookup(caller_func_name);
        caller_func_proc_sym->add_properties(NmodlType::use_range_ptr_var);
    }
}

void FunctionCallpathVisitor::visit_procedure_block(const ast::ProcedureBlock& node) {
    /// Avoid recursive calls
    if (std::find(visited_functions_or_procedures.begin(),
                  visited_functions_or_procedures.end(),
                  &node) != visited_functions_or_procedures.end()) {
        return;
    }
    visited_functions_or_procedures.push_back(&node);
    node.visit_children(*this);
    visited_functions_or_procedures.pop_back();
}

void FunctionCallpathVisitor::visit_function_block(const ast::FunctionBlock& node) {
    // Avoid recursive calls
    if (std::find(visited_functions_or_procedures.begin(),
                  visited_functions_or_procedures.end(),
                  &node) != visited_functions_or_procedures.end()) {
        return;
    }
    visited_functions_or_procedures.push_back(&node);
    node.visit_children(*this);
    visited_functions_or_procedures.pop_back();
}

void FunctionCallpathVisitor::visit_program(const ast::Program& node) {
    psymtab = node.get_symbol_table();
    node.visit_children(*this);
}

}  // namespace visitor
}  // namespace nmodl
