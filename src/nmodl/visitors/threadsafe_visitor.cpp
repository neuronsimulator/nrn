/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visitors/threadsafe_visitor.hpp"

#include "ast/all.hpp"
#include "utils/logger.hpp"
#include "visitors/visitor_utils.hpp"
#include "codegen/codegen_naming.hpp"


namespace nmodl {
namespace visitor {
void ThreadsafeVisitor::visit_function_call(ast::FunctionCall& node) {
    const auto& name = node.get_node_name();
    if (not in_net_receive_block and name == codegen::naming::NRN_STATE_DISC_METHOD) {
        can_be_made_threadsafe = false;
    }
    node.visit_children(*this);
}

void ThreadsafeVisitor::visit_net_receive_block(ast::NetReceiveBlock& node) {
    in_net_receive_block = true;
    node.visit_children(*this);
    in_net_receive_block = false;
}
void ThreadsafeVisitor::visit_initial_block(ast::InitialBlock& node) {
    in_initial_block = true;
    node.visit_children(*this);
    in_initial_block = false;
}

void ThreadsafeVisitor::visit_param_block(ast::ParamBlock& node) {
    in_parameter_block = true;
    node.visit_children(*this);
    in_parameter_block = false;
}

void ThreadsafeVisitor::visit_assigned_block(ast::AssignedBlock& node) {
    in_assigned_block = true;
    node.visit_children(*this);
    in_assigned_block = false;
}


void ThreadsafeVisitor::visit_binary_expression(ast::BinaryExpression& node) {
    const auto& lhs = node.get_lhs();
    const auto& converted_lhs = std::dynamic_pointer_cast<ast::VarName>(lhs);
    const bool condition = not in_initial_block and not in_parameter_block and
                           not in_assigned_block;
    if (condition and converted_lhs and global_vars.count(converted_lhs->get_node_name()) > 0) {
        can_be_made_threadsafe = false;
    }
    node.visit_children(*this);
}

void ThreadsafeVisitor::visit_program(ast::Program& node) {
    // short circuit
    const auto& threadsafe_vars = collect_nodes(node, {ast::AstNodeType::THREAD_SAFE});
    if (threadsafe_vars.size() > 0) {
        logger->info("Mechanism already marked as thread-safe, nothing to do");
        return;
    }

    const auto& unsafe_constructs = collect_nodes(node,
                                                  {ast::AstNodeType::EXTERNAL,
                                                   ast::AstNodeType::LINEAR_BLOCK,
                                                   ast::AstNodeType::DISCRETE_BLOCK});
    if (unsafe_constructs.size() > 0) {
        can_be_made_threadsafe = false;
    }

    // I don't get this one for now. Can it be made threadsafe?
    const auto& pointer_vars = collect_nodes(node, {ast::AstNodeType::POINTER});
    if (pointer_vars.size() > 0) {
        can_be_made_threadsafe = false;
    }

    const auto& verbatim_blocks = collect_nodes(node, {ast::AstNodeType::VERBATIM});

    if (verbatim_blocks.size() > 0 and not convert_verbatim) {
        can_be_made_threadsafe = false;
    }

    const auto& global_vars_local = collect_nodes(node, {ast::AstNodeType::GLOBAL_VAR});
    for (const auto& var: global_vars_local) {
        const auto& conv = std::dynamic_pointer_cast<ast::GlobalVar>(var);
        global_vars.insert(conv->get_node_name());
    }

    node.visit_children(*this);

    if (not can_be_made_threadsafe) {
        logger->warn("Mechanism cannot be made thread safe");
        return;
    }

    const auto& neuron_block = collect_nodes(node, {ast::AstNodeType::NEURON_BLOCK});
    if (not neuron_block.size()) {
        // TODO insert the block
        return;
    }
    logger->info("Will insert THREADSAFE block");
    const auto& converted = std::dynamic_pointer_cast<ast::NeuronBlock>(neuron_block[0]);
    const auto& statement_block = converted->get_statement_block();
    auto expr_statement = std::make_shared<ast::ExpressionStatement>(
        std::make_shared<ast::Name>(std::make_shared<ast::String>("THREADSAFE")));
    statement_block->emplace_back_statement(expr_statement);
}

}  // namespace visitor
}  // namespace nmodl
