/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::LocalToAssignedVisitor
 */

#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * @addtogroup visitor_classes
 * @{
 */

/**
 * \class LocalToAssignedVisitor
 * \brief Visitor to convert top level LOCAL variables to ASSIGNED variables
 *
 * Some of the existing mod file include declaration of LOCAL variables in
 * the top level of the mod file. Those variables are normally written in
 * the INITIAL block which is executed potentially by multiple threads.
 * This results into race condition in case of CoreNEURON. To avoid this,
 * such variables are converted to ASSIGNED variables which by default are
 * handled as RANGE.
 *
 * For example:
 *
 * \code{.mod}
 *      NEURON {
 *          SUFFIX test
 *  	    RANGE x
 *      }
 *
 *      LOCAL qt
 *
 *      INITIAL {
 *          qt = 10.0
 *      }
 *
 *      PROCEDURE rates(v (mV)) {
 *          x = qt + 12.2
 *      }
 * \endcode
 *
 * In the above example, `qt` is used as temporary variable to pass value from
 * INITIAL block to PROCEDURE. This works fine in case of serial execution but
 * in parallel execution we end up in race condition. To avoid this, we convert
 * qt to ASSIGNED variable.
 *
 * \todo
 *   - Variables like qt are often constant. As long as INITIAL block is executed
 *     serially or qt is updated in atomic way then we don't have a problem.
 */

class LocalToAssignedVisitor: public AstVisitor {
  public:
    /// \name Ctor & dtor
    /// \{

    /// Default constructor
    LocalToAssignedVisitor() = default;

    /// \}

    /// Visit ast::Program node to transform top level LOCAL variables
    /// to ASSIGNED if they are written in the mod file
    void visit_program(ast::Program& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
