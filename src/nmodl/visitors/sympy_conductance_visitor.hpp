/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::SympyConductanceVisitor
 */

#include <map>
#include <set>
#include <vector>

#include "visitors/ast_visitor.hpp"

namespace nmodl {

namespace visitor {

/**
 * @addtogroup visitor_classes
 * @{
 */

/**
 * \class SympyConductanceVisitor
 * \brief %Visitor for generating CONDUCTANCE statements for ions
 *
 * This class visits each ion expression `I = ...` in the `BREAKPOINT`
 * and symbolically differentiates it to get `dI/dV`, i.e. the conductance.
 * If this coincides with an existing global variable `g` (e.g. when
 * `I` is linear in `V`) then it will add
 *
 * \code{.mod}
 *      CONDUCTANCE g USEION I
 * \endcode
 *
 * If `dI/dV` is a more complicated expression, it generates a new unique
 * variable name `g_unique`, and adds two lines
 *
 * \code{.mod}
 *      CONDUCTANCE g_unique USEION I
 *      g_unique = [dI/dV expression]
 * \endcode
 *
 * If an ion channel already has a `CONDUCTANCE` statement then it does
 * not modify it.
 */
class SympyConductanceVisitor: public AstVisitor {
    typedef std::map<std::string, std::string> string_map;
    typedef std::set<std::string> string_set;

  private:
    /// true while visiting breakpoint block
    bool under_breakpoint_block = false;

    /// set of all variables for SymPy
    string_set all_vars;

    /// set of currents to ignore
    string_set i_ignore;

    /// map between current write names and ion names
    string_map i_name;

    bool NONSPECIFIC_CONDUCTANCE_ALREADY_EXISTS = false;

    /// list in order of binary expressions in breakpoint
    std::vector<std::string> ordered_binary_exprs;

    /// ditto but for LHS of expression only
    std::vector<std::string> ordered_binary_exprs_lhs;

    /// map from lhs of binary expression to index of expression in above vector
    std::map<std::string, std::size_t> binary_expr_index;

    /// use ion ast nodes
    std::vector<std::shared_ptr<const ast::Ast>> use_ion_nodes;

    /// non specific currents
    std::vector<std::shared_ptr<const ast::Ast>> nonspecific_nodes;

    std::vector<std::string> generate_statement_strings(ast::BreakpointBlock& node);
    void lookup_useion_statements();
    void lookup_nonspecific_statements();

    static std::string to_nmodl_for_sympy(const ast::Ast& node);

  public:
    SympyConductanceVisitor() = default;
    void visit_binary_expression(ast::BinaryExpression& node) override;
    void visit_breakpoint_block(ast::BreakpointBlock& node) override;
    void visit_conductance_hint(ast::ConductanceHint& node) override;
    void visit_program(ast::Program& node) override;
};

/** @} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
