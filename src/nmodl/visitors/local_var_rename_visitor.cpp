#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/rename_visitor.hpp"

using namespace ast;
using namespace symtab;

/** Return new name variable by appending "_r_COUNT" where COUNT is number
 * of times the given variable is already used.
 */
std::string LocalVarRenameVisitor::get_new_name(const std::string& name) {
    int suffix = 0;
    if (renamed_variables.find(name) != renamed_variables.end()) {
        suffix = renamed_variables[name];
    }

    /// increase counter for next usage
    renamed_variables[name] = suffix + 1;
    return (name + "_r_" + std::to_string(suffix));
}

LocalVarVector* LocalVarRenameVisitor::get_local_variables(StatementBlock* node) {
    LocalVarVector* variables = nullptr;
    for (const auto& statement : node->statements) {
        /// we are only interested in local variable definition statement
        if (statement->get_type() == Type::LOCAL_LIST_STATEMENT) {
            auto local_statement = std::static_pointer_cast<LocalListStatement>(statement);
            variables = &(local_statement->variables);
            break;
        }
    }
    return variables;
}

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

    std::shared_ptr<SymbolTable> parent_symtab;
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
            std::string new_name = get_new_name(name);
            rename_visitor.set(name, new_name);
            rename_visitor.visit_statement_block(node);
        }
    }

    /// go back to previous block in hierarchy
    symtab = symtab_stack.top();
    symtab_stack.pop();
}
