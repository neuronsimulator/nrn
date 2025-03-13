/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include "ast/ast_decl.hpp"
#include "ast/function_block.hpp"
#include "ast/string.hpp"
#include "ast/table_statement.hpp"
#include "codegen/codegen_transform_visitor.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
using namespace ast;

void CodegenTransformVisitor::visit_function_block(FunctionBlock& node) {
    auto table_statements = collect_nodes(node, {AstNodeType::TABLE_STATEMENT});
    for (auto t: table_statements) {
        auto t_ = std::dynamic_pointer_cast<TableStatement>(t);
        t_->set_table_vars(
            {std::make_shared<Name>(std::make_shared<String>(node.get_node_name()))});
    }
}
}  // namespace nmodl
