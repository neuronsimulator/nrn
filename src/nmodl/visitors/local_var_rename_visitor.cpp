#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/visitor_utils.hpp"

using namespace ast;
using namespace symtab;

/// rename name conflicting variables in the statement block and it's all children
void LocalVarRenameVisitor::visit_statement_block(StatementBlock* node) {
    /// nothing to do
    if (node->statements.empty()) {
        return;
    }

    auto current_symtab = node->get_symbol_table();
    if (current_symtab) {
        symtab = current_symtab;
    }

    // Some statetements like forall, from, while are of type expression statement type.
    // These statements contain statement block but do not have symbol table. And hence
    // we push last non-null symbol table on the stack.
    symtab_stack.push(symtab);

    // first need to process all children : perform recursively from innermost block
    for (auto& item : node->statements) {
        item->visit_children(this);
    }

    auto variables = get_local_variables(node);

    SymbolTable* parent_symtab = nullptr;
    if (symtab) {
        parent_symtab = symtab->get_parent_table();
    }

    /// global blocks do not change (do no have parent symbol table)
    /// if no variables in the block then there is nothing to do
    if (parent_symtab == nullptr || variables == nullptr) {
        return;
    }

    RenameVisitor rename_visitor;

    for (auto& var : *variables) {
        std::string name = var->get_name();
        auto s = parent_symtab->lookup_in_scope(name);

        /// if symbol represents variable (to avoid renaming use of units like mV)
        if (s && s->is_variable()) {
            std::string new_name = get_new_name(name, "r", renamed_variables);
            rename_visitor.set(name, new_name);
            rename_visitor.visit_statement_block(node);
        }
    }

    /// go back to previous block in hierarchy
    symtab = symtab_stack.top();
    symtab_stack.pop();
}
