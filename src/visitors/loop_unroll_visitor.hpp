/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::LoopUnrollVisitor
 */

#include <string>

#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class LoopUnrollVisitor
 * \brief Unroll for loop in the AST
 *
 * Derivative and kinetic blocks have for loop with coupled set of ODEs :
 *
 * \code{.mod}
 *     DEFINE NANN 4
 *     KINETIC state {
 *        FROM i=0 TO NANN-2 {
 *    		~ ca[i] <-> ca[i+1] (DFree*frat[i+1]*1(um), DFree*frat[i+1]*1(um))
 *        }
 *     }
 * \endcode
 *
 * To solve these ODEs with SymPy, we need to get all ODEs from such loops.
 * This visitor unroll such loops and insert new expression statement in
 * the AST :
 *
 * \code{.mod}
 *     KINETIC state {
 *       {
 *           ~ ca[0] <-> ca[0+1] (DFree*frat[0+1]*1(um), DFree*frat[0+1]*1(um))
 *           ~ ca[1] <-> ca[1+1] (DFree*frat[1+1]*1(um), DFree*frat[1+1]*1(um))
 *           ~ ca[2] <-> ca[2+1] (DFree*frat[2+1]*1(um), DFree*frat[2+1]*1(um))
 *       }
 *     }
 * \endcode
 *
 * Note that the index `0+1` is not expanded to `1` because we do not run
 * constant folder pass within this loop (but could be easily done).
 */
class LoopUnrollVisitor: public AstVisitor {
  public:
    LoopUnrollVisitor() = default;

    void visit_statement_block(ast::StatementBlock& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
