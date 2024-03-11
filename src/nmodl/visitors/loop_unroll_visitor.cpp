/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visitors/loop_unroll_visitor.hpp"


#include "ast/all.hpp"
#include "parser/c11_driver.hpp"
#include "utils/logger.hpp"
#include "visitors/index_remover.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {


/// return underlying expression wrapped by WrappedExpression
static std::shared_ptr<ast::Expression> unwrap(const std::shared_ptr<ast::Expression>& expr) {
    if (expr && expr->is_wrapped_expression()) {
        const auto& e = std::dynamic_pointer_cast<ast::WrappedExpression>(expr);
        return e->get_expression();
    }
    return expr;
}


/**
 * Unroll given for loop
 *
 * \param node From loop node in the AST
 * \return expression statement representing unrolled loop if successful otherwise \a nullptr
 */
static std::shared_ptr<ast::ExpressionStatement> unroll_for_loop(
    const std::shared_ptr<ast::FromStatement>& node) {
    /// loop can be in the form of `FROM i=(0) TO (10)`
    /// so first unwrap all elements of the loop
    const auto& from = unwrap(node->get_from());
    const auto& to = unwrap(node->get_to());
    const auto& increment = unwrap(node->get_increment());

    /// we can unroll if iteration space of the loop is known
    /// after constant folding start, end and increment must be known
    if (!from->is_integer() || !to->is_integer() ||
        (increment != nullptr && !increment->is_integer())) {
        return nullptr;
    }

    int start = std::dynamic_pointer_cast<ast::Integer>(from)->eval();
    int end = std::dynamic_pointer_cast<ast::Integer>(to)->eval();
    int step = 1;
    if (increment != nullptr) {
        step = std::dynamic_pointer_cast<ast::Integer>(increment)->eval();
    }

    ast::StatementVector statements;
    std::string index_var = node->get_node_name();
    for (int i = start; i <= end; i += step) {
        /// duplicate loop body and copy all statements to new vector
        const auto new_block = std::unique_ptr<ast::StatementBlock>(
            node->get_statement_block()->clone());
        IndexRemover(index_var, i).visit_statement_block(*new_block);
        statements.insert(statements.end(),
                          new_block->get_statements().begin(),
                          new_block->get_statements().end());
    }

    /// create new statement representing unrolled loop
    auto block = new ast::StatementBlock(std::move(statements));
    return std::make_shared<ast::ExpressionStatement>(block);
}


/**
 * Parse verbatim blocks and rename variable if it is used.
 */
void LoopUnrollVisitor::visit_statement_block(ast::StatementBlock& node) {
    node.visit_children(*this);

    const auto& statements = node.get_statements();

    for (auto iter = statements.begin(); iter != statements.end(); ++iter) {
        if ((*iter)->is_from_statement()) {
            const auto& statement = std::dynamic_pointer_cast<ast::FromStatement>((*iter));

            /// check if any verbatim block exists
            const auto& verbatim_blocks = collect_nodes(*statement, {ast::AstNodeType::VERBATIM});
            if (!verbatim_blocks.empty()) {
                logger->debug("LoopUnrollVisitor : can not unroll because of verbatim block");
                continue;
            }

            /// unroll loop, replace current statement on successfull unroll
            const auto& new_statement = unroll_for_loop(statement);
            if (new_statement != nullptr) {
                node.reset_statement(iter, new_statement);

                const auto& before = to_nmodl(statement);
                const auto& after = to_nmodl(new_statement);
                logger->debug("LoopUnrollVisitor : \n {} \n unrolled to \n {}", before, after);
            }
        }
    }
}

}  // namespace visitor
}  // namespace nmodl
