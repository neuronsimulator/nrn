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
#include "ast/all.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class MergeTopLevelBlocksVisitor
 * \brief Visitor which merges given top-level blocks into one
 *
 * This template takes two arguments which both describe the type of top-level
 * block. The arguments must match and refer to the same type of block!
 *
 * The first argument is a subclass of nmodl::ast::Ast
 *
 * The second argument is an instance of nmodl::ast::AstNodeType
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
        // since ast::Program::erase_node can only delete top-level nodes, we
        // need to keep track of the found includes, as well as all of the
        // _other_ statements (since ast::Include does not provide an
        // erase_node function, but only set_blocks)
        std::unordered_map<std::shared_ptr<ast::Include>, ast::NodeVector> include_blocks_to_keep;

        const auto& blocks = node.get_blocks();
        collect_statements_from_vector(blocks,
                                       statements,
                                       blocks_to_delete,
                                       include_blocks_to_keep);

        // insert new top-level block which has all the collected statements
        auto statement_block = ast::StatementBlock(statements);
        auto toplevel_block = ast_class(statement_block.clone());
        node.emplace_back_node(toplevel_block.clone());

        // delete all of the previously-found top-level blocks
        node.erase_node(blocks_to_delete);

        // also delete all of the blocks from INCLUDE blocks
        for (const auto& [include_block, blocks_to_keep]: include_blocks_to_keep) {
            include_block->set_blocks(blocks_to_keep);
        }
    }

  private:
    // Helper function to collect all top-level blocks in an INCLUDE except `ast_class` ones
    ast::NodeVector collect_include_except(const ast::Include& node) const {
        ast::NodeVector result;
        for (const auto& block: node.get_blocks()) {
            // only insert if it's not an instance of `ast_class`
            if (std::dynamic_pointer_cast<ast_class>(block) == nullptr) {
                result.push_back(block);
            }
        }
        return result;
    }

    // Helper function to collect statements from a NodeVector (including nested includes)
    void collect_statements_from_vector(
        const ast::NodeVector& blocks,
        ast::StatementVector& statements,
        std::unordered_set<ast::Node*>& blocks_to_delete,
        std::unordered_map<std::shared_ptr<ast::Include>, ast::NodeVector>&
            include_blocks_to_keep) {
        for (auto& block: blocks) {
            auto include_block = std::dynamic_pointer_cast<ast::Include>(block);
            if (include_block) {
                // Recursively process nested includes
                const auto& included_blocks = include_block->get_blocks();
                include_blocks_to_keep[include_block] = collect_include_except(*include_block);
                collect_statements_from_vector(included_blocks,
                                               statements,
                                               blocks_to_delete,
                                               include_blocks_to_keep);
            } else {
                auto temp_block = std::dynamic_pointer_cast<ast_class>(block);
                // check if it's the correct type
                if (temp_block) {
                    auto statement_block = temp_block->get_statement_block();
                    // if block is not empty, copy statement-block into vector
                    if (statement_block && !statement_block->get_statements().empty()) {
                        statements.push_back(
                            std::make_shared<ast::ExpressionStatement>(statement_block));
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
