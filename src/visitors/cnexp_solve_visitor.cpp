/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <sstream>

#include "codegen/codegen_naming.hpp"
#include "parser/diffeq_driver.hpp"
#include "symtab/symbol.hpp"
#include "utils/logger.hpp"
#include "visitors/cnexp_solve_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/visitor_utils.hpp"


namespace nmodl {

void CnexpSolveVisitor::visit_solve_block(ast::SolveBlock* node) {
    auto name = node->get_block_name()->get_node_name();
    auto method = node->get_method();
    solve_method = method ? method->get_value()->eval() : "";
    solve_blocks[name] = solve_method;
}


void CnexpSolveVisitor::visit_derivative_block(ast::DerivativeBlock* node) {
    derivative_block_name = node->get_name()->get_node_name();
    derivative_block = true;
    node->visit_children(this);
    derivative_block = false;
}


void CnexpSolveVisitor::visit_diff_eq_expression(ast::DiffEqExpression* node) {
    differential_equation = true;
    node->visit_children(this);
    differential_equation = false;
}


void CnexpSolveVisitor::visit_binary_expression(ast::BinaryExpression* node) {
    auto& lhs = node->lhs;
    auto& rhs = node->rhs;
    auto& op = node->op;

    /// we have to only solve odes under derivative block where lhs is variable
    if (!derivative_block || !differential_equation || !lhs->is_var_name()) {
        return;
    }

    auto solve_method = solve_blocks[derivative_block_name];
    auto name = std::dynamic_pointer_cast<ast::VarName>(lhs)->get_name();

    if (name->is_prime_name()) {
        auto equation = nmodl::to_nmodl(node);
        parser::DiffeqDriver diffeq_driver;

        if (solve_method == codegen::naming::CNEXP_METHOD) {
            std::string solution;
            /// check if ode can be solved with cnexp method
            if (diffeq_driver.cnexp_possible(equation, solution)) {
                auto statement = create_statement(solution);
                auto expr_statement = std::dynamic_pointer_cast<ast::ExpressionStatement>(
                    statement);
                auto bin_expr = std::dynamic_pointer_cast<ast::BinaryExpression>(
                    expr_statement->get_expression());
                lhs.reset(bin_expr->lhs->clone());
                rhs.reset(bin_expr->rhs->clone());
            } else {
                logger->warn("cnexp solver not possible for {}", to_nmodl(node));
            }
        } else if (solve_method == codegen::naming::EULER_METHOD) {
            std::string solution = diffeq_driver.solve(equation, solve_method);
            auto statement = create_statement(solution);
            auto expr_statement = std::dynamic_pointer_cast<ast::ExpressionStatement>(statement);
            auto bin_expr = std::dynamic_pointer_cast<ast::BinaryExpression>(
                expr_statement->get_expression());
            lhs.reset(bin_expr->lhs->clone());
            rhs.reset(bin_expr->rhs->clone());
        } else if (solve_method == codegen::naming::DERIVIMPLICIT_METHOD) {
            auto varname = "D" + name->get_node_name();
            auto variable = new ast::Name(new ast::String(varname));
            lhs.reset(variable);
            if (program_symtab->lookup(varname) == nullptr) {
                auto symbol = std::make_shared<symtab::Symbol>(varname, ModToken());
                symbol->set_original_name(name->get_node_name());
                symbol->created_from_state();
                program_symtab->insert(symbol);
            }
        } else {
            logger->error("solver method '{}' not supported", solve_method);
        }
    }
}

void CnexpSolveVisitor::visit_program(ast::Program* node) {
    program_symtab = node->get_symbol_table();
    node->visit_children(this);
}

}  // namespace nmodl
