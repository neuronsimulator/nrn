/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::SolveBlockVisitor
 */

#include "symtab/decl.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class SolveBlockVisitor
 * \brief Replace solve block statements with actual solution node in the AST
 *
 * Once SympySolverVisitor or NeuronSolveVisitor is ran, solve blocks can be replaced
 * with solution expression node that represent solution that is going to be solved.
 * All solve statements appearing in breakpoint block get added to NrnState block as
 * solution expression.
 */
class SolveBlockVisitor: public AstVisitor {
  private:
    symtab::SymbolTable* symtab = nullptr;

    bool in_breakpoint_block = false;

    /// solve expression statements for NrnState block
    ast::StatementVector nrn_state_solve_statements;

    ast::SolutionExpression* create_solution_expression(ast::SolveBlock& solve_block);

  public:
    SolveBlockVisitor() = default;
    void visit_breakpoint_block(ast::BreakpointBlock& node) override;
    void visit_expression_statement(ast::ExpressionStatement& node) override;
    void visit_program(ast::Program& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
