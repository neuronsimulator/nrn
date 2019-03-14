/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <stack>
#include <string>

#include "ast/ast.hpp"
#include "utils/logger.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {

/**
 * \class ConstantFolderVisitor
 * \brief Perform constant folding of integer/float/double expressions
 *
 * MOD file from user could have binary expressions that could be
 * expanded at compile time. For example, KINETIC blocks could have
 *
 * DEFINE NANN 10
 *
 * KINETIC states {
 *      FROM i=0 TO NANN-2 {
 *          ....
 *      }
 * }
 *
 * For passes like loop unroll, we need to evaluate NANN-2 at
 * compile time and this pass perform such operations.
 */

class ConstantFolderVisitor: public AstVisitor {
  public:
    ConstantFolderVisitor() = default;
    void visit_wrapped_expression(ast::WrappedExpression* node) override;
    void visit_paren_expression(ast::ParenExpression* node) override;
};

}  // namespace nmodl