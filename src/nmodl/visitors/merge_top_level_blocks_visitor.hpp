/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::MergeTopLevelBlocksVisitor
 */

#include "visitors/ast_visitor.hpp"

#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class MergeTopLevelBlocksVisitor
 * \brief Visitor which merges given top-level blocks into one
 */
template <typename ast_class, ast::AstNodeType ast_type>
class MergeTopLevelBlocksVisitor: public AstVisitor {
  public:
    MergeTopLevelBlocksVisitor() = default;
    void visit_program(ast::Program& node) override {
        // check if there is > 1 blocks in total
        if (collect_nodes(node, {ast_type}).size() <= 1) {
            return;
        }

        // collect all top-level blocks in the program
        const auto& blocks = node.get_blocks();

        // collect all statements from top-level blocks, and the blocks themselves
        ast::StatementVector statements;
        std::unordered_set<ast::Node*> blocks_to_delete;
        for (auto& block: blocks) {
            auto temp_block = std::dynamic_pointer_cast<ast_class>(block);
            // check if it's the correct type
            if (temp_block) {
                auto statement_block = temp_block->get_statement_block();
                // if block is not empty, copy statements into vector
                if (statement_block) {
                    for (const auto& statement: statement_block->get_statements()) {
                        statements.push_back(statement);
                    }
                }
                blocks_to_delete.insert(block.get());
            }
        }

        // insert new top-level block which has the above statements
        auto new_block = ast::StatementBlock(statements);
        auto new_initial_block = ast_class(new_block.clone());
        node.emplace_back_node(new_initial_block.clone());

        // delete all of the previously-found top-level blocks
        node.erase_node(blocks_to_delete);
    }
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
