/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::IspcRenameVisitor
 */

#include <string>

#include "visitors/ast_visitor.hpp"
#include "visitors/rename_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * @addtogroup visitor_classes
 * @{
 */

/**
 * \class IspcRenameVisitor
 * \brief Rename variable names to match with ISPC backend requirement
 *
 * Wrapper class of RenameVisitor to rename the
 * variables of the mod file that match the representation
 * of double constants by the ISPC compiler. If those
 * variables are not renamed to avoid matching the
 * representation of the double constants of the ISPC
 * compiler, then there are compilation issues.
 * Here are some examples of double constants that
 * ISPC compiler parses: 3.14d, 31.4d-1, 1.d, 1.0d,
 * 1d-2. A regex which matches these variables is:
 *     ([0-9\.]*d[\-0-9]+)|([0-9\.]+d[\-0-9]*)
 *
 * Example mod file:
 * \code{.mod}
 *          NEURON {
 *              SUFFIX test_ispc_rename
 *              RANGE d1, d2, d3, var_d3, d4
 *          }
 *          ASSIGNED {
 *              d1
 *              d2
 *              d3
 *              var_d3
 *              d4
 *          }
 *          INITIAL {
 *              d1 = 1
 *              d2 = 2
 *              d3 = 3
 *              var_d3 = 3
 *          }
 *          PROCEDURE func () {
 *          VERBATIM
 *              d4 = 4;
 *          ENDVERBATIM
 *          }
 * \endcode
 * Variables d1, d2 and d4 match the double constant
 * presentation of variables by ISPC and should be
 * renamed to var_d1, var_d2 and var_d4 to avoid
 * compilation errors.
 * var_d3 can stay the same because it doesn't match
 * the regex.
 * In case var_d1 already exists as variable then d1
 * is renamed to var_d1_<random_sequence>.
 */
class IspcRenameVisitor: public RenameVisitor {
  public:
    /// Default constructor
    IspcRenameVisitor() = delete;

    /// Constructor that takes as parameter the AST and calls the RenameVisitor
    explicit IspcRenameVisitor(std::shared_ptr<ast::Program> ast)
        : RenameVisitor(ast, "([0-9\\.]*d[\\-0-9]+)|([0-9\\.]+d[\\-0-9]*)", "var_", true, true) {}
};

/** @} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
