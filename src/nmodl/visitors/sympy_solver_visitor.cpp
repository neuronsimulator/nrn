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

void SympySolverVisitor::visit_solve_block(ast::SolveBlock* node) {
    auto method = node->get_method();
    if (method) {
        solve_method = method->get_value()->eval();
    }
}

void SympySolverVisitor::visit_diff_eq_expression(ast::DiffEqExpression* node) {
    if ((solve_method != cnexp_method) && (solve_method != euler_method)) {
        logger->debug(
            "SympySolverVisitor :: solve method \"{}\" not {} or {}, so not integrating "
            "expression analytically",
            solve_method, cnexp_method, euler_method);
        return;
    }

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

    const auto node_as_nmodl = to_nmodl_for_sympy(node);
    const auto locals = py::dict("equation_string"_a = node_as_nmodl,
                                 "t_var"_a = codegen::naming::NTHREAD_T_VARIABLE,
                                 "dt_var"_a = codegen::naming::NTHREAD_DT_VARIABLE, "vars"_a = vars,
                                 "use_pade_approx"_a = use_pade_approx);

    if (solve_method == euler_method) {
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
    } else if (solve_method == cnexp_method) {
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
    }

    auto solution = locals["solution"].cast<std::string>();
    logger->debug("SympySolverVisitor :: -> solution: {}", solution);

    auto exception_message = locals["exception_message"].cast<std::string>();
    if (!exception_message.empty()) {
        logger->warn("SympySolverVisitor :: python exception: " + exception_message);
    }

    if (!solution.empty()) {
        auto statement = create_statement(solution);
        auto expr_statement = std::dynamic_pointer_cast<ast::ExpressionStatement>(statement);
        auto bin_expr = std::dynamic_pointer_cast<ast::BinaryExpression>(
            expr_statement->get_expression());
        lhs.reset(bin_expr->lhs->clone());
        rhs.reset(bin_expr->rhs->clone());
    } else {
        logger->warn("SympySolverVisitor :: solution to differential equation not possible");
    }
}

void SympySolverVisitor::visit_derivative_block(ast::DerivativeBlock* node) {
    // get any local vars
    if (auto symtab = node->get_statement_block()->get_symbol_table()) {
        auto localvars = symtab->get_variables_with_properties(NmodlType::local_var);
        for (const auto& v: localvars) {
            vars.insert(v->get_name());
        }
    }
    node->visit_children(this);
}

void SympySolverVisitor::visit_program(ast::Program* node) {
    vars = get_global_vars(node);
    node->visit_children(this);
}

}  // namespace nmodl
