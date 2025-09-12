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
template <typename ast_class,
          ast::AstNodeType ast_type,
          // only enable it for descendents of `ast::Block`, and only if it's not an abstract class
          typename = std::enable_if_t<std::is_base_of_v<ast::Block, ast_class> &&
                                      !std::is_abstract_v<ast_class>>>
class MergeTopLevelBlocksVisitor: public AstVisitor {
  public:
    MergeTopLevelBlocksVisitor() = default;

    void visit_program(ast::Program& node) override {
        // check if there is > 1 block in total (including nested includes)
        if (collect_nodes(node, {ast_type}).size() <= 1) {
            return;
        }

        // collect all statements from all blocks (including nested includes)
        ast::StatementVector statements;
        std::unordered_set<ast::Node*> blocks_to_delete;

        const auto& blocks = node.get_blocks();
        collect_statements_from_vector(blocks, statements, blocks_to_delete);

        // insert new top-level block which has all the collected statements
        auto new_block = ast::StatementBlock(statements);
        auto new_initial_block = ast_class(new_block.clone());
        node.emplace_back_node(new_initial_block.clone());

        // delete all of the previously-found top-level blocks
        node.erase_node(blocks_to_delete);
    }

  private:
    // Helper function to collect statements from a NodeVector (including nested includes)
    void collect_statements_from_vector(const ast::NodeVector& blocks,
                                        ast::StatementVector& statements,
                                        std::unordered_set<ast::Node*>& blocks_to_delete) {
        for (auto& block: blocks) {
            auto include_block = std::dynamic_pointer_cast<ast::Include>(block);
            if (include_block) {
                // Recursively process nested includes
                const auto& included_blocks = include_block->get_blocks();
                collect_statements_from_vector(included_blocks, statements, blocks_to_delete);
            } else {
                auto temp_block = std::dynamic_pointer_cast<ast_class>(block);
                // check if it's the correct type
                if (temp_block) {
                    logger->info("Found block {}", typeid(ast_class).name());
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
        }
    }
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
