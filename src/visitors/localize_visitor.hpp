/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::LocalizeVisitor
 */

#include <map>

#include "symtab/decl.hpp"
#include "visitors/ast_visitor.hpp"


namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class LocalizeVisitor
 * \brief %Visitor to transform global variable usage to local
 *
 * Motivation: As NMODL doesn't support returning multiple values,
 * procedures are often written with use of range variables that
 * can be made local. For example:
 *
 * \code{.mod}
 *      NEURON {
 *          RANGE tau, alpha, beta
 *      }
 *
 *      DERIVATIVE states() {
 *          ...
 *          rates()
 *          alpha = tau + beta
 *      }
 *
 *      PROCEDURE rates() {
 *          tau = xx * 0.12 * some_var
 *          beta = yy * 0.11
 *      }
 * \endcode
 *
 * In above example we are only interested in variable alpha computed in
 * DERIVATIVE block. If rates() is inlined into DERIVATIVE block then we
 * get:
 *
 * \code{.mod}
 *       DERIVATIVE states() {
 *          ...
 *          {
 *              tau = xx * 0.12 * some_var
 *              beta = yy * 0.11
 *          }
 *          alpha = tau + beta
 *      }
 * \endcode
 *
 * Now tau and beta could become local variables provided that their values
 * are not used in any other global blocks.
 *
 * Implementation Notes:
 *   - For every global variable in the mod file we have to compute
 *     def-use chains in global blocks (except procedure and functions, which should be
 *     already inlined).
 *   - If every block has "definition" first then that variable is safe to "localize"
 *
 * \todo
 *   - We are excluding procedures/functions because they will be still using global
 *     variables. We need to have dead-code removal pass to eliminate unused procedures/
 *     functions before localizer pass.
 */
class LocalizeVisitor: public ConstAstVisitor {
  private:
    /// ignore verbatim blocks while localizing
    bool ignore_verbatim = false;

    symtab::SymbolTable* program_symtab = nullptr;

    std::vector<std::string> variables_to_optimize() const;

    bool node_for_def_use_analysis(const ast::Node& node) const;

    bool is_solve_procedure(const ast::Node& node) const;

  public:
    LocalizeVisitor() = default;

    explicit LocalizeVisitor(bool ignore_verbatim)
        : ignore_verbatim(ignore_verbatim) {}

    void visit_program(const ast::Program& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
