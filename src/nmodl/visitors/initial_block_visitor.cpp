/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unordered_set>

#include "visitors/initial_block_visitor.hpp"
#include "visitors/visitor_utils.hpp"

#include "ast/all.hpp"

namespace nmodl {
namespace visitor {

void MergeInitialBlocksVisitor::visit_program(ast::Program& node) {
    // check if there is > 1 INITIAL block
    if (collect_nodes(node, {ast::AstNodeType::INITIAL_BLOCK}).size() <= 1) {
        return;
    }

    // collect all blocks in the program
    const auto& blocks = node.get_blocks();

    // collect all statements from INITIAL blocks, and INITIAL blocks themselves
    ast::StatementVector statements;
    std::unordered_set<ast::Node*> blocks_to_delete;
    for (auto& block: blocks) {
        if (block->is_initial_block()) {
            auto statement_block =
                std::static_pointer_cast<ast::StatementBlock>(block)->get_statement_block();
            // if block is not empty, copy statements into vector
            if (statement_block) {
                for (const auto& statement: statement_block->get_statements()) {
                    statements.push_back(statement);
                }
            }
            blocks_to_delete.insert(block.get());
        }
    }

    // insert new INITIAL block which has the above statements
    auto new_block = ast::StatementBlock(statements);
    auto new_initial_block = ast::InitialBlock(new_block.clone());
    node.emplace_back_node(new_initial_block.clone());

    // delete all of the previously-found INITIAL blocks
    node.erase_node(blocks_to_delete);
}
}  // namespace visitor
}  // namespace nmodl
