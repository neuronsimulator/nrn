/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ast/ast_decl.hpp"
#include "ast/derivative_block.hpp"
#include "ast/name.hpp"
#include "ast/program.hpp"
#include "ast/solve_block.hpp"
#include "ast/string.hpp"
#include "codegen/codegen_naming.hpp"
#include "visitors/solve_without_method_visitor.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {

void SolveWithoutMethodVisitor::visit_program(ast::Program& node) {
    const auto& derivative_blocks = collect_nodes(node, {ast::AstNodeType::DERIVATIVE_BLOCK});
    for (const auto& derivative_block: derivative_blocks) {
        const auto& block = std::dynamic_pointer_cast<ast::DerivativeBlock>(derivative_block);
        derivative_block_names.insert(block->get_node_name());
    }
    node.visit_children(*this);
}

void SolveWithoutMethodVisitor::visit_solve_block(ast::SolveBlock& node) {
    const auto& name = node.get_block_name()->get_node_name();
    const auto& method = node.get_method();
    if (derivative_block_names.count(name) && !method) {
        node.set_method(std::make_shared<ast::Name>(
            std::make_shared<ast::String>(codegen::naming::DERIVIMPLICIT_METHOD)));
    }
    node.visit_children(*this);
}

}  // namespace visitor
}  // namespace nmodl
