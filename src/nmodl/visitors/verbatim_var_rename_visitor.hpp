/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <stack>
#include <string>

#include "symtab/symbol_table.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {

/**
 * \class VerbatimVarRenameVisitor
 * \brief Rename variable in verbatim block
 *
 * Verbatim blocks in NMODL use different names for local
 * and range variables:
 *
 * - if local variable is xx then translated name of variable
 *    in C file is _lxx
 *  - if range (or any other global) variable is xx then translated
 *    name of the variable is _p_xx
 *
 * This naming convention is based on NEURON code generation convention.
 * As part of this pass, we revert such usages of the variable to original
 * names. We do this only if variable is present in symbol table.
 *
 * \todo : check if symbol table lookup is ok or there are cases where this
 * could be error prone.
 */

class VerbatimVarRenameVisitor: public AstVisitor {
  private:
    /// non-null symbol table in the scope hierarchy
    symtab::SymbolTable* symtab = nullptr;

    /// symbol tables in nested blocks
    std::stack<symtab::SymbolTable*> symtab_stack;

    /// prefix used for local variable
    const std::string local_prefix = "_l";

    /// prefix used for range variables
    const std::string range_prefix = "_p_";

    std::string rename_variable(std::string);

  public:
    VerbatimVarRenameVisitor() = default;

    virtual void visit_verbatim(ast::Verbatim* node) override;
    virtual void visit_statement_block(ast::StatementBlock* node) override;
};

}  // namespace nmodl