/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 *
 * \dir
 * \brief Visitors implementation
 *
 * \file
 * \brief \copybrief nmodl::visitor::ConstantFolderVisitor
 */

#include <stack>
#include <string>

#include "visitors/ast_visitor.hpp"


namespace nmodl {
namespace visitor {

/**
 * @addtogroup visitor_classes
 * @{
 */

/**
 * \class ConstantFolderVisitor
 * \brief Perform constant folding of integer/float/double expressions
 *
 * MOD file from user could have binary expressions that could be
 * expanded at compile time. For example, KINETIC blocks could have
 *
 * \code{.mod}
 *      DEFINE NANN 10
 *
 *      KINETIC states {
 *          FROM i=0 TO NANN-2 {
 *              ....
 *          }
 *      }
 * \endcode
 *
 * For passes like loop unroll, we need to evaluate `NANN-2` at
 * compile time and this pass perform such operations.
 */
class ConstantFolderVisitor: public AstVisitor {
  public:
    ConstantFolderVisitor() = default;
    void visit_wrapped_expression(ast::WrappedExpression& node) override;
    void visit_paren_expression(ast::ParenExpression& node) override;
};

/** @} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
