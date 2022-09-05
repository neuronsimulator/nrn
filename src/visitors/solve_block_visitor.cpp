/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/solve_block_visitor.hpp"

#include <cassert>
#include <fmt/format.h>

#include "ast/all.hpp"
#include "codegen/codegen_naming.hpp"
#include "visitor_utils.hpp"

namespace nmodl {
namespace visitor {

void SolveBlockVisitor::visit_breakpoint_block(ast::BreakpointBlock& node) {
    in_breakpoint_block = true;
    node.visit_children(*this);
    in_breakpoint_block = false;
}

/// check if given node contains sympy solution
static bool has_sympy_solution(const ast::Ast& node) {
    return !collect_nodes(node, {ast::AstNodeType::EIGEN_NEWTON_SOLVER_BLOCK}).empty();
}

/**
 * Create solution expression node that will be used for solve block
 * \param solve_block solve block used to describe node to solve and method
 * \return solution expression that will be used to replace the solve block
 *
 * Depending on the solver used, solve block is converted to solve expression statement
 * that will be used to replace solve block. Note that the blocks are clones instead of
 * shared_ptr because DerivimplicitCallback is currently contain whole node
 * instead of just pointer.
 */
ast::SolutionExpression* SolveBlockVisitor::create_solution_expression(
    ast::SolveBlock& solve_block) {
    /// find out the block that is going to solved
    const auto& block_name = solve_block.get_block_name()->get_node_name();
    const auto& solve_node_symbol = symtab->lookup(block_name);
    if (solve_node_symbol == nullptr) {
        throw std::runtime_error(
            fmt::format("SolveBlockVisitor :: cannot find the block '{}' to solve it", block_name));
    }
    auto node_to_solve = solve_node_symbol->get_node();

    /// in case of derivimplicit method if neuron solver is used (i.e. not sympy) then
    /// the solution is not in place but we have to create a callback to newton solver
    const auto& method = solve_block.get_method();
    std::string solve_method = method ? method->get_node_name() : "";
    if (solve_method == codegen::naming::DERIVIMPLICIT_METHOD &&
        !has_sympy_solution(*node_to_solve)) {
        /// typically derivimplicit is used for derivative block only
        assert(node_to_solve->get_node_type() == ast::AstNodeType::DERIVATIVE_BLOCK);
        auto derivative_block = dynamic_cast<ast::DerivativeBlock*>(node_to_solve);
        auto callback_expr = new ast::DerivimplicitCallback(derivative_block->clone());
        return new ast::SolutionExpression(solve_block.clone(), callback_expr);
    }

    auto block_to_solve = node_to_solve->get_statement_block();
    return new ast::SolutionExpression(solve_block.clone(), block_to_solve->clone());
}

/**
 * Replace solve blocks with solution expression
 * @param node Ast node for SOLVE statement in the mod file
 */
void SolveBlockVisitor::visit_expression_statement(ast::ExpressionStatement& node) {
    node.visit_children(*this);
    if (node.get_expression()->is_solve_block()) {
        auto solve_block = dynamic_cast<ast::SolveBlock*>(node.get_expression().get());
        auto sol_expr = create_solution_expression(*solve_block);
        if (in_breakpoint_block) {
            nrn_state_solve_statements.emplace_back(new ast::ExpressionStatement(sol_expr));
        } else {
            node.set_expression(std::shared_ptr<ast::SolutionExpression>(sol_expr));
        }
    }
}

void SolveBlockVisitor::visit_program(ast::Program& node) {
    symtab = node.get_symbol_table();
    node.visit_children(*this);
    /// add new node NrnState with solve blocks from breakpoint block
    if (!nrn_state_solve_statements.empty()) {
        auto nrn_state = new ast::NrnStateBlock(nrn_state_solve_statements);
        node.emplace_back_node(nrn_state);
    }
}

}  // namespace visitor
}  // namespace nmodl
