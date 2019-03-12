/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <set>
#include <vector>

#include "ast/ast.hpp"
#include "symtab/symbol.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/lookup_visitor.hpp"

namespace nmodl {

/**
 * \class SympySolverVisitor
 * \brief Visitor for differential equations in derivative block
 *
 * For solver method "cnexp":
 * replace each the differential equation with its analytic
 * solution using SymPy, optionally using the (1,1) order in dt
 * Pade approximant to the solution.
 *
 * For solver method "euler":
 * replace each the differential equation with a
 * forwards Euler timestep
 *
 * For solver method "sparse":
 * constructs backwards Euler timestep for coupled set of *linear*
 * ODEs, solves resulting linear algebraic equation by
 * Gaussian substitution, replaces differential equations
 * with explicit solution of backwards Euler equations
 */

class SympySolverVisitor: public AstVisitor {
  private:
    /** Replace binary expression with new expression provided as string */
    static void replace_diffeq_expression(ast::DiffEqExpression* expr, const std::string& new_expr);

    /** Remove statements from given statement block if they exist */
    static void remove_statements_from_block(ast::StatementBlock* block,
                                             const std::set<ast::Node*> statements);

    std::shared_ptr<ast::EigenNewtonSolverBlock> construct_eigen_newton_solver_block(
        const std::vector<std::string>& setup_x,
        const std::vector<std::string>& functor,
        const std::vector<std::string>& update_state);
    /// global variables
    std::set<std::string> global_vars;

    /// local variables in current block + globals
    std::set<std::string> vars;

    /// map between derivative block names and associated solver method
    std::map<std::string, std::string> derivative_block_solve_method{};

    /// prime variables from LHS of ODEs
    std::vector<std::string> prime_variables;

    /// ODE statements appeared in the derivative block
    std::set<ast::Node*> diffeq_statements;

    /// current expression statement being visited (to track ODEs)
    ast::ExpressionStatement* current_diffeq_statement;

    /// current statement block being visited
    ast::StatementBlock* current_statement_block;

    /// method specified in solve block
    std::string solve_method;

    /// optionally replace cnexp solution with (1,1) pade approx
    bool use_pade_approx;

    // optionally do CSE (common subexpression elimination) for sparse solver
    bool elimination;

    /// vector of coupled ODE equations to solve
    std::vector<std::string> ode_system;

    static std::string to_nmodl_for_sympy(ast::AST* node) {
        return nmodl::to_nmodl(node, {ast::AstNodeType::UNIT});
    }

  public:
    SympySolverVisitor(bool use_pade_approx = false, bool elimination = true)
        : use_pade_approx(use_pade_approx)
        , elimination(elimination){};

    void visit_diff_eq_expression(ast::DiffEqExpression* node) override;
    void visit_expression_statement(ast::ExpressionStatement* node) override;
    void visit_derivative_block(ast::DerivativeBlock* node) override;
    void visit_statement_block(ast::StatementBlock* node) override;
    void visit_program(ast::Program* node) override;
};

}  // namespace nmodl