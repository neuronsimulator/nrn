/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <map>
#include <stack>

#include "ast/ast.hpp"
#include "symtab/symbol_table.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {

/**
 * \class LocalVarRenameVisitor
 * \brief Visitor to rename local variables conflicting with global scope
 *
 * Motivation: During inlining we have to do data-flow-analysis. Consider
 * below example:
 *
 *      NEURON {
 *          RANGE tau, beta
 *      }
 *
 *      DERIVATIVE states() {
 *          LOCAL tau
 *          ...
 *          rates()
 *      }
 *
 *      PROCEDURE rates() {
 *          tau = beta * 0.12 * some_var
 *      }
 *
 * When rates() will be inlined into states(), local definition of tau will
 * conflict with range variable tau. Hence we can't just copy the statements.
 * Dataflow analysis could be done at the time of inlining. Other way is to run
 * this pass before inlining and pre-rename any local-global variable conflicts.
 * As we are renaming local variables only, it's safe and there are no side effects.
 *
 * \todo: currently we are renaming variables even if there is no inlining candidates.
 * In this case ideally we should not rename.
 */

class LocalVarRenameVisitor: public AstVisitor {
  private:
    /// non-null symbol table in the scope hierarchy
    symtab::SymbolTable* symtab = nullptr;

    /// symbol tables in case of nested blocks
    std::stack<symtab::SymbolTable*> symtab_stack;

    /// variables currently being renamed and their count
    std::map<std::string, int> renamed_variables;

  public:
    LocalVarRenameVisitor() = default;
    virtual void visit_statement_block(ast::StatementBlock* node) override;
};

}  // namespace nmodl