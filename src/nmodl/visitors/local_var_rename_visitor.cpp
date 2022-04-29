/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/local_var_rename_visitor.hpp"

#include "ast/expression_statement.hpp"
#include "ast/local_list_statement.hpp"
#include "ast/statement_block.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/visitor_utils.hpp"


namespace nmodl {
namespace visitor {

using symtab::SymbolTable;

/// rename name conflicting variables in the statement block and it's all children
void LocalVarRenameVisitor::visit_statement_block(ast::StatementBlock& node) {
    /// nothing to do
    if (node.get_statements().empty()) {
        return;
    }

    auto current_symtab = node.get_symbol_table();
    if (current_symtab != nullptr) {
        symtab = current_symtab;
    }

    // Some statements like forall, from, while are of type expression statement type.
    // These statements contain statement block but do not have symbol table. And hence
    // we push last non-null symbol table on the stack.
    symtab_stack.push(symtab);

    // first need to process all children : perform recursively from innermost block
    for (const auto& item: node.get_statements()) {
        item->visit_children(*this);
    }

    /// go back to previous block in hierarchy
    symtab = symtab_stack.top();
    symtab_stack.pop();

    SymbolTable* parent_symtab = nullptr;
    if (symtab != nullptr) {
        parent_symtab = symtab->get_parent_table();
    }

    const auto& variables = get_local_list_statement(node);

    /// global blocks do not change (do no have parent symbol table)
    /// if no variables in the block then there is nothing to do
    if (parent_symtab == nullptr || variables == nullptr) {
        return;
    }

    RenameVisitor rename_visitor;

    for (const auto& var: variables->get_variables()) {
        std::string name = var->get_node_name();
        auto s = parent_symtab->lookup_in_scope(name);
        /// if symbol is a variable name (avoid renaming use of units like mV)
        if (s && s->is_variable()) {
            std::string new_name = get_new_name(name, "r", renamed_variables);
            rename_visitor.set(name, new_name);
            rename_visitor.visit_statement_block(node);
            auto symbol = symtab->lookup_in_scope(name);
            symbol->set_name(new_name);
            symbol->mark_renamed();
        }
    }
}

}  // namespace visitor
}  // namespace nmodl
