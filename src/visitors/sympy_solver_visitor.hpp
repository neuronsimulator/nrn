/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::SympySolverVisitor
 */

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ast/ast.hpp"
#include "symtab/symbol.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {

/**
 * @addtogroup visitor_classes
 * @{
 */

/**
 * \class SympySolverVisitor
 * \brief %Visitor for systems of algebraic and differential equations
 *
 * For DERIVATIVE block, solver method `cnexp`:
 *  - replace each ODE with its analytic solution
 *  - optionally using the `(1,1)` order Pade approximant in dt
 *
 * For `DERIVATIVE` block, solver method `euler`:
 *  - replace each ODE with forwards Euler timestep
 *
 * For `DERIVATIVE` block, solver method `sparse` and `derivimplicit`:
 *  - construct backwards Euler timestep non-linear system
 *  - return function F and its Jacobian J to be solved by newton solver
 *
 * For `LINEAR` blocks:
 *  - for small systems: solve linear system of algebraic equations by
 *    Gaussian elimination, replace equations with solutions
 *  - for large systems: return matrix and vector of linear system
 *    to be solved by e.g. LU factorization
 *
 * For `NON_LINEAR` blocks:
 *  - return function F and its Jacobian J to be solved by newton solver
 */
class SympySolverVisitor: public AstVisitor {
  private:
    /// clear any data from previous block & get set of block local vars + global vars
    void init_block_data(ast::Node* node);

    /// construct vector from set of state vars in correct order
    void init_state_vars_vector();

    /// replace binary expression with new expression provided as string
    static void replace_diffeq_expression(ast::DiffEqExpression& expr, const std::string& new_expr);

    /// raise error if kinetic/ode/(non)linear statements are spread over multiple blocks
    void check_expr_statements_in_same_block();

    /// return iterator pointing to where solution should be inserted in statement block
    ast::StatementVector::const_iterator get_solution_location_iterator(
        const ast::StatementVector& statements);

    /// construct solver block
    void construct_eigen_solver_block(const std::vector<std::string>& pre_solve_statements,
                                      const std::vector<std::string>& solutions,
                                      bool linear);

    /// solve linear system (for "LINEAR")
    void solve_linear_system(const std::vector<std::string>& pre_solve_statements = {});

    /// solve non-linear system (for "derivimplicit", "sparse" and "NONLINEAR")
    void solve_non_linear_system(const std::vector<std::string>& pre_solve_statements = {});

    /// return NMODL string version of node, excluding any units
    static std::string to_nmodl_for_sympy(ast::Ast& node) {
        return nmodl::to_nmodl(node, {ast::AstNodeType::UNIT, ast::AstNodeType::UNIT_DEF});
    }

    /// Function used by SympySolverVisitor::filter_X to replace the name X in a std::string
    /// to X_operator
    static std::string& replaceAll(std::string& context,
                                   const std::string& from,
                                   const std::string& to);

    /// Check original_vector for elements that contain a variable named original_string and
    /// rename it to substitution_string
    static std::vector<std::string> filter_string_vector(
        const std::vector<std::string>& original_vector,
        const std::string& original_string,
        const std::string& substitution_string);

    /// global variables
    std::set<std::string> global_vars;

    /// local variables in current block + globals
    std::set<std::string> vars;

    /// custom function calls used in ODE block
    std::set<std::string> function_calls;

    /// map between derivative block names and associated solver method
    std::unordered_map<std::string, std::string> derivative_block_solve_method{};

    /// expression statements appearing in the block
    /// (these can be of type DiffEqExpression, LinEquation or NonLinEquation)
    std::unordered_set<ast::Statement*> expression_statements;

    /// current expression statement being visited (to track ODEs / (non)lineqs)
    ast::ExpressionStatement* current_expression_statement;

    /// last expression statement visited (to know where to insert solutions in statement block)
    ast::ExpressionStatement* last_expression_statement = nullptr;

    /// current statement block being visited
    ast::StatementBlock* current_statement_block = nullptr;

    /// block where expression statements appear (to check there is only one)
    ast::StatementBlock* block_with_expression_statements = nullptr;

    /// method specified in solve block
    std::string solve_method;

    /// vector of {ODE, linear eq, non-linear eq} system to solve
    std::vector<std::string> eq_system;

    /// only solve eq_system system of equations if this is true:
    bool eq_system_is_valid = true;

    /// true for (non)linear eqs, to identify all state vars used in equations
    bool collect_state_vars = false;

    /// vector of all state variables (in order specified in STATE block in mod file)
    std::vector<std::string> all_state_vars;

    /// set of state variables used in block
    std::set<std::string> state_vars_in_block;

    /// vector of state vars used *in block* (in same order as all_state_vars)
    std::vector<std::string> state_vars;

    /// map from state vars to the algebraic equation from CONSERVE statement that should replace
    /// their ODE, if any
    std::unordered_map<std::string, std::string> conserve_equation;

    /// optionally replace cnexp solution with (1,1) pade approx
    bool use_pade_approx;

    /// optionally do CSE (common subexpression elimination) for sparse solver
    bool elimination;

    /// max number of state vars allowed for small system linear solver
    int SMALL_LINEAR_SYSTEM_MAX_STATES;

  public:
    explicit SympySolverVisitor(bool use_pade_approx = false,
                                bool elimination = true,
                                int SMALL_LINEAR_SYSTEM_MAX_STATES = 3)
        : use_pade_approx(use_pade_approx)
        , elimination(elimination)
        , SMALL_LINEAR_SYSTEM_MAX_STATES(SMALL_LINEAR_SYSTEM_MAX_STATES){};

    void visit_var_name(ast::VarName& node) override;
    void visit_diff_eq_expression(ast::DiffEqExpression& node) override;
    void visit_conserve(ast::Conserve& node) override;
    void visit_derivative_block(ast::DerivativeBlock& node) override;
    void visit_lin_equation(ast::LinEquation& node) override;
    void visit_linear_block(ast::LinearBlock& node) override;
    void visit_non_lin_equation(ast::NonLinEquation& node) override;
    void visit_non_linear_block(ast::NonLinearBlock& node) override;
    void visit_expression_statement(ast::ExpressionStatement& node) override;
    void visit_statement_block(ast::StatementBlock& node) override;
    void visit_program(ast::Program& node) override;
};

/** @} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
