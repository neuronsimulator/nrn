/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <iostream>

#include "codegen/codegen_naming.hpp"
#include "symtab/symbol.hpp"
#include "utils/logger.hpp"
#include "visitor_utils.hpp"
#include "visitors/sympy_solver_visitor.hpp"


namespace py = pybind11;
using namespace py::literals;

namespace nmodl {

using symtab::syminfo::NmodlType;

void SympySolverVisitor::replace_diffeq_expression(ast::DiffEqExpression* expr,
                                                   const std::string& new_expr) {
    auto new_statement = create_statement(new_expr);
    auto new_expr_statement = std::dynamic_pointer_cast<ast::ExpressionStatement>(new_statement);
    auto new_bin_expr = std::dynamic_pointer_cast<ast::BinaryExpression>(
        new_expr_statement->get_expression());
    expr->set_expression(std::move(new_bin_expr));
}

std::shared_ptr<ast::EigenNewtonSolverBlock>
SympySolverVisitor::construct_eigen_newton_solver_block(
    const std::vector<std::string>& setup_x,
    const std::vector<std::string>& functor,
    const std::vector<std::string>& update_state) {
    auto setup_x_block = create_statement_block(setup_x);
    auto functor_block = create_statement_block(functor);
    auto update_state_block = create_statement_block(update_state);
    return std::make_shared<ast::EigenNewtonSolverBlock>(setup_x_block, functor_block,
                                                         update_state_block);
}

void SympySolverVisitor::remove_statements_from_block(ast::StatementBlock* block,
                                                      const std::set<ast::Node*> statements) {
    auto& statement_vec = block->statements;
    statement_vec.erase(std::remove_if(statement_vec.begin(), statement_vec.end(),
                                       [&statements](std::shared_ptr<ast::Statement>& s) {
                                           return statements.find(s.get()) != statements.end();
                                       }),
                        statement_vec.end());
}

void SympySolverVisitor::visit_statement_block(ast::StatementBlock* node) {
    current_statement_block = node;
    node->visit_children(this);
}

void SympySolverVisitor::visit_expression_statement(ast::ExpressionStatement* node) {
    current_diffeq_statement = node;
    node->visit_children(this);
}

void SympySolverVisitor::visit_diff_eq_expression(ast::DiffEqExpression* node) {
    auto& lhs = node->get_expression()->lhs;
    auto& rhs = node->get_expression()->rhs;

    if (!lhs->is_var_name()) {
        logger->warn("SympySolverVisitor :: LHS of differential equation is not a VariableName");
        return;
    }
    auto lhs_name = std::dynamic_pointer_cast<ast::VarName>(lhs)->get_name();
    if (!lhs_name->is_prime_name()) {
        logger->warn("SympySolverVisitor :: LHS of differential equation is not a PrimeName");
        return;
    }

    prime_variables.push_back(lhs->get_node_name());
    diffeq_statements.insert(current_diffeq_statement);

    const auto node_as_nmodl = to_nmodl_for_sympy(node);
    const auto locals = py::dict("equation_string"_a = node_as_nmodl,
                                 "t_var"_a = codegen::naming::NTHREAD_T_VARIABLE,
                                 "dt_var"_a = codegen::naming::NTHREAD_DT_VARIABLE, "vars"_a = vars,
                                 "use_pade_approx"_a = use_pade_approx);

    if (solve_method == codegen::naming::EULER_METHOD) {
        logger->debug("SympySolverVisitor :: EULER - solving: {}", node_as_nmodl);
        // replace x' = f(x) differential equation
        // with forwards Euler timestep:
        // x = x + f(x) * dt
        py::exec(R"(
                from nmodl.ode import forwards_euler2c
                exception_message = ""
                try:
                    solution = forwards_euler2c(equation_string, dt_var, vars)
                except Exception as e:
                    # if we fail, fail silently and return empty string
                    solution = ""
                    exception_message = str(e)
            )",
                 py::globals(), locals);
    } else if (solve_method == codegen::naming::CNEXP_METHOD) {
        // replace x' = f(x) differential equation
        // with analytic solution for x(t+dt) in terms of x(t)
        // x = ...
        logger->debug("SympySolverVisitor :: CNEXP - solving: {}", node_as_nmodl);
        py::exec(R"(
                from nmodl.ode import integrate2c
                exception_message = ""
                try:
                    solution = integrate2c(equation_string, t_var, dt_var, vars, use_pade_approx)
                except Exception as e:
                    # if we fail, fail silently and return empty string
                    solution = ""
                    exception_message = str(e)
            )",
                 py::globals(), locals);
    } else {
        // for other solver methods: just collect the ODEs & return
        logger->debug("SympySolverVisitor :: adding ODE system: {}", to_nmodl_for_sympy(node));
        ode_system.push_back(to_nmodl_for_sympy(node));
        return;
    }

    // replace ODE with solution in AST
    auto solution = locals["solution"].cast<std::string>();
    logger->debug("SympySolverVisitor :: -> solution: {}", solution);

    auto exception_message = locals["exception_message"].cast<std::string>();
    if (!exception_message.empty()) {
        logger->warn("SympySolverVisitor :: python exception: " + exception_message);
        return;
    }

    if (!solution.empty()) {
        replace_diffeq_expression(node, solution);
    } else {
        logger->warn("SympySolverVisitor :: solution to differential equation not possible");
    }
}

void SympySolverVisitor::visit_derivative_block(ast::DerivativeBlock* node) {
    // get all vars for this block, i.e. global vars + local vars
    vars = global_vars;
    if (auto symtab = node->get_statement_block()->get_symbol_table()) {
        auto localvars = symtab->get_variables_with_properties(NmodlType::local_var);
        for (const auto& v: localvars) {
            vars.insert(v->get_name());
        }
    }

    // get user specified solve method for this block
    solve_method = derivative_block_solve_method[node->get_node_name()];

    /// clear information from previous derivative block if any
    diffeq_statements.clear();
    prime_variables.clear();
    ode_system.clear();

    // visit each differential equation:
    //  - for CNEXP or EULER, each equation is independent & is replaced with its solution
    //  - otherwise, each equation is added to ode_system (and to binary_expressions_to_replace)
    node->visit_children(this);

    /// if there are no odes collected then there is nothing to do
    if (!ode_system.empty()) {
        // solve system of ODEs in ode_system
        logger->debug("SympySolverVisitor :: Solving {} system of ODEs", solve_method);
        auto locals = py::dict("equation_strings"_a = ode_system,
                               "t_var"_a = codegen::naming::NTHREAD_T_VARIABLE,
                               "dt_var"_a = codegen::naming::NTHREAD_DT_VARIABLE, "vars"_a = vars,
                               "do_cse"_a = elimination);
        py::exec(R"(
                from nmodl.ode import solve_ode_system
                exception_message = ""
                try:
                    solutions, new_local_vars = solve_ode_system(equation_strings, t_var, dt_var, vars, do_cse)
                except Exception as e:
                    # if we fail, fail silently and return empty string
                    solutions = [""]
                    new_local_vars = [""]
                    exception_message = str(e)
            )",
                 py::globals(), locals);

        // returns a vector of solutions, i.e. new statements to add to block:
        auto solutions = locals["solutions"].cast<std::vector<std::string>>();
        // and a vector of new local variables that need to be declared in the block:
        auto new_local_vars = locals["new_local_vars"].cast<std::vector<std::string>>();
        auto exception_message = locals["exception_message"].cast<std::string>();

        if (!exception_message.empty()) {
            logger->warn("SympySolverVisitor :: solve_ode_system python exception: " +
                         exception_message);
            return;
        }

        // sanity check: must have at least as many solutions as ODE's to replace:
        if (solutions.size() < ode_system.size()) {
            logger->warn("SympySolverVisitor :: Solve failed: fewer solutions than ODE's");
            return;
        }

        // declare new local vars
        if (!new_local_vars.empty()) {
            for (const auto& new_local_var: new_local_vars) {
                logger->debug("SympySolverVisitor :: -> declaring new local variable: {}",
                              new_local_var);
                add_local_variable(current_statement_block, new_local_var);
            }
        }

        if (solve_method == codegen::naming::SPARSE_METHOD) {
            remove_statements_from_block(current_statement_block, diffeq_statements);
            // get a copy of existing statements in block
            auto statements = current_statement_block->get_statements();
            // add new statements
            for (const auto& sol: solutions) {
                logger->debug("SympySolverVisitor :: -> adding statement: {}", sol);
                statements.push_back(create_statement(sol));
            }
            // replace old set of statements in AST with new one
            current_statement_block->set_statements(std::move(statements));
        } else if (solve_method == codegen::naming::DERIVIMPLICIT_METHOD) {
            /// Construct X from the state variables by using the original
            /// ODE statements in the block. Also create statements to update
            /// state variables from X
            std::vector<std::string> setup_x_eqs;
            std::vector<std::string> update_state_eqs;
            for (int i = 0; i < prime_variables.size(); i++) {
                auto statement = prime_variables[i] + " = " + "X[ " + std::to_string(i) + "]";
                auto rev_statement = "X[ " + std::to_string(i) + "]" + " = " + prime_variables[i];
                update_state_eqs.push_back(statement);
                setup_x_eqs.push_back(rev_statement);
            }

            /// remove original ODE statements from the block where they initially appear
            remove_statements_from_block(current_statement_block, diffeq_statements);

            /// create newton solution block and add that as statement back in the block
            /// statements in solutions : put F, J into new functor to be created for eigen
            auto solver_block = construct_eigen_newton_solver_block(setup_x_eqs, solutions,
                                                                    update_state_eqs);

            if (vars.find("X") != vars.end()) {
                logger->error("SympySolverVisitor :: -> X conflicts with NMODL variable");
            }

            current_statement_block->addStatement(
                std::make_shared<ast::ExpressionStatement>(solver_block));
        }
    }
}

void SympySolverVisitor::visit_program(ast::Program* node) {
    global_vars = get_global_vars(node);

    // get list of solve statements with names & methods
    AstLookupVisitor ast_lookup_visitor;
    auto solve_block_nodes = ast_lookup_visitor.lookup(node, ast::AstNodeType::SOLVE_BLOCK);
    for (const auto& block: solve_block_nodes) {
        if (auto block_ptr = std::dynamic_pointer_cast<ast::SolveBlock>(block)) {
            std::string solve_method = block_ptr->get_method()->get_value()->eval();
            std::string block_name = block_ptr->get_block_name()->get_value()->eval();
            logger->debug("SympySolverVisitor :: Found SOLVE statement: using {} for {}",
                          solve_method, block_name);
            derivative_block_solve_method[block_name] = solve_method;
        }
    }

    node->visit_children(this);
}

}  // namespace nmodl
