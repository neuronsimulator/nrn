/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/neuron_solve_visitor.hpp"

#include "ast/all.hpp"
#include "codegen/codegen_naming.hpp"
#include "parser/diffeq_driver.hpp"
#include "symtab/symbol.hpp"
#include "utils/logger.hpp"
#include "visitors/visitor_utils.hpp"


namespace nmodl {
namespace visitor {

void NeuronSolveVisitor::visit_solve_block(ast::SolveBlock& node) {
    auto name = node.get_block_name()->get_node_name();
    const auto& method = node.get_method();
    solve_method = method ? method->get_value()->eval() : "";
    solve_blocks[name] = solve_method;
}


void NeuronSolveVisitor::visit_derivative_block(ast::DerivativeBlock& node) {
    derivative_block_name = node.get_name()->get_node_name();
    derivative_block = true;
    node.visit_children(*this);
    derivative_block = false;
}


void NeuronSolveVisitor::visit_diff_eq_expression(ast::DiffEqExpression& node) {
    differential_equation = true;
    node.visit_children(*this);
    differential_equation = false;
}


void NeuronSolveVisitor::visit_binary_expression(ast::BinaryExpression& node) {
    const auto& lhs = node.get_lhs();

    /// we have to only solve odes under derivative block where lhs is variable
    if (!derivative_block || !differential_equation || !lhs->is_var_name()) {
        return;
    }

    const auto& solve_method = solve_blocks[derivative_block_name];
    auto name = std::dynamic_pointer_cast<ast::VarName>(lhs)->get_name();

    if (name->is_prime_name()) {
        auto equation = to_nmodl(node);
        parser::DiffeqDriver diffeq_driver;

        if (solve_method == codegen::naming::CNEXP_METHOD) {
            std::string solution;
            /// check if ode can be solved with cnexp method
            if (diffeq_driver.cnexp_possible(equation, solution)) {
                auto statement = create_statement(solution);
                auto expr_statement = std::dynamic_pointer_cast<ast::ExpressionStatement>(
                    statement);
                const auto bin_expr = std::dynamic_pointer_cast<const ast::BinaryExpression>(
                    expr_statement->get_expression());
                node.set_lhs(std::shared_ptr<ast::Expression>(bin_expr->get_lhs()->clone()));
                node.set_rhs(std::shared_ptr<ast::Expression>(bin_expr->get_rhs()->clone()));
            } else {
                logger->warn("NeuronSolveVisitor :: cnexp solver not possible for {}",
                             to_nmodl(node));
            }
        } else if (solve_method == codegen::naming::EULER_METHOD) {
            std::string solution = diffeq_driver.solve(equation, solve_method);
            auto statement = create_statement(solution);
            auto expr_statement = std::dynamic_pointer_cast<ast::ExpressionStatement>(statement);
            const auto bin_expr = std::dynamic_pointer_cast<const ast::BinaryExpression>(
                expr_statement->get_expression());
            node.set_lhs(std::shared_ptr<ast::Expression>(bin_expr->get_lhs()->clone()));
            node.set_rhs(std::shared_ptr<ast::Expression>(bin_expr->get_rhs()->clone()));
        } else if (solve_method == codegen::naming::DERIVIMPLICIT_METHOD) {
            auto varname = "D" + name->get_node_name();
            node.set_lhs(std::make_shared<ast::Name>(new ast::String(varname)));
            if (program_symtab->lookup(varname) == nullptr) {
                auto symbol = std::make_shared<symtab::Symbol>(varname, ModToken());
                symbol->set_original_name(name->get_node_name());
                symbol->created_from_state();
                program_symtab->insert(symbol);
            }
        } else {
            logger->error("NeuronSolveVisitor :: solver method '{}' not supported", solve_method);
        }
    }
}

void NeuronSolveVisitor::visit_program(ast::Program& node) {
    program_symtab = node.get_symbol_table();
    node.visit_children(*this);
}

}  // namespace visitor
}  // namespace nmodl
