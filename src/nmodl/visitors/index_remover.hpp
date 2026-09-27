/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "ast/all.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \class IndexRemover
 * \brief Helper visitor to replace index of array variable with integer
 *
 * When loop is unrolled, the index variable like `i` :
 *
 *   ca[i] <-> ca[i+1]
 *
 * has type `Name` in the AST. This needs to be replaced with `Integer`
 * for optimizations like constant folding. This pass look at name and
 * binary expressions under index variables.
 */
class IndexRemover: public AstVisitor {
  private:
    /// index variable name
    std::string index;

    /// integer value of index variable
    int value;

    /// true if we are visiting index variable
    bool under_indexed_name = false;

  public:
    IndexRemover(std::string index, int value);

    /// if expression we are visiting is `Name` then return new `Integer` node
    std::shared_ptr<ast::Expression> replace_for_name(
        const std::shared_ptr<ast::Expression>& node) const;

    void visit_binary_expression(ast::BinaryExpression& node) override;
    void visit_indexed_name(ast::IndexedName& node) override;
};

}  // namespace visitor
}  // namespace nmodl
