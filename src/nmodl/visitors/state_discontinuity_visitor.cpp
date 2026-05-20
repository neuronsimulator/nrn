/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <utility>

#include "ast/all.hpp"
#include "codegen/codegen_naming.hpp"
#include "utils/logger.hpp"
#include "visitors/state_discontinuity_visitor.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {


// convert `state_discontinuity(A, B)` call to `A = B` statement
static auto convert_state_discontinuity(const ast::FunctionCall& node) {
    const auto& args = node.get_arguments();
    const auto& lhs = args[0];
    const auto& rhs = args[1];
    return create_statement(fmt::format("{} = {}", to_nmodl(lhs), to_nmodl(rhs)));
}


// we only change the call to `state_discontinuity` if it is in the NET_RECEIVE block
void StateDiscontinuityVisitor::visit_net_receive_block(ast::NetReceiveBlock& node) {
    in_net_receive_block = true;
    node.visit_children(*this);
    in_net_receive_block = false;
}

void StateDiscontinuityVisitor::visit_statement_block(ast::StatementBlock& node) {
    if (in_net_receive_block) {
        // where we will store all of the new statements to insert into the block
        ast::StatementVector new_statements;

        // collect all statements; if it's a call to `state_discontinuity`, replace it with `A = B`
        const auto& statements = node.get_statements();
        for (const auto& statement: statements) {
            auto current_statement = statement;
            const auto& hits = collect_nodes(*current_statement, {ast::AstNodeType::FUNCTION_CALL});
            for (const auto& hit: hits) {
                const auto& fn_call = std::dynamic_pointer_cast<ast::FunctionCall>(hit);
                if (fn_call->get_name()->get_node_name() ==
                    codegen::naming::NRN_STATE_DISC_METHOD) {
                    current_statement = convert_state_discontinuity(*fn_call);
                    logger->info("Converting {} to {}",
                                 to_nmodl(statement),
                                 to_nmodl(current_statement));
                }
            }
            new_statements.push_back(current_statement);
        }
        node.set_statements(new_statements);
    }
    node.visit_children(*this);
}


}  // namespace visitor
}  // namespace nmodl
