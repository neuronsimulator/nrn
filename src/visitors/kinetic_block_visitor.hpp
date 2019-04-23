/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::KineticBlockVisitor
 */

#include "ast/ast.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/visitor_utils.hpp"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace nmodl {
namespace visitor {

/**
 * @addtogroup visitor_classes
 * @{
 */

/**
 * \class KineticBlockVisitor
 * \brief %Visitor for kinetic block statements
 *
 * Replaces each KINETIC block with a DERIVATIVE block
 * containing a system of ODEs that is equivalent to
 * the original set of reaction statements
 *
 * Currently is only valid assuming the order
 * of statements within the KINETIC block does not
 * matter. Also does not yet support array variables.
 *
 */
class KineticBlockVisitor: public AstVisitor {
  private:
    /// update stoichiometric matrices with reaction var term
    void process_reac_var(const std::string& varname, int count = 1);

    /// stoichiometric matrices nu_L, nu_R
    /// forwards/backwards fluxes k_f, k_b
    /// (see kinetic_schemes.ipynb notebook for details)
    struct RateEqs {
        std::vector<std::vector<int>> nu_L;
        std::vector<std::vector<int>> nu_R;
        std::vector<std::string> k_f;
        std::vector<std::string> k_b;
    } rate_eqs;

    /// multiplicative factors for ODEs from COMPARTMENT statements
    std::vector<std::string> compartment_factors;

    /// additive constant terms for ODEs from reaction statements like ~ x << (a)
    std::vector<std::string> additive_terms;

    /// multiplicate constant terms for fluxes from non-state vars as reactants
    /// e.g. reaction statements like ~ x <-> c (a,a)
    /// where c is not a state var, which is equivalent to the ODE
    /// x' = a * (c - x)
    std::vector<std::string> non_state_var_fflux;
    std::vector<std::string> non_state_var_bflux;

    /// generated set of fluxes and ODEs
    std::vector<std::string> fflux;
    std::vector<std::string> bflux;
    std::vector<std::string> odes;

    /// current expressions for the `fflux`, `bflux` variables that can be used in the mod file
    /// and that are determined by the preceeding kinetic reaction statement, i.e. their
    /// value changes depending on their location inside the kinetic block
    std::string modfile_fflux;
    std::string modfile_bflux;

    /// number of state variables
    int state_var_count = 0;

    /// state variables vector
    std::vector<std::string> state_var;

    /// map from state variable to corresponding index
    std::map<std::string, int> state_var_index;

    /// true if we are visiting a reaction statement
    bool in_reaction_statement = false;

    /// true if we are visiting the left hand side of reaction statement
    bool in_reaction_statement_lhs = false;

    /// current statement index
    int i_statement = 0;

    /// vector of kinetic block nodes
    std::vector<ast::KineticBlock*> kinetic_blocks;

    /// statements to remove from block
    std::set<ast::Node*> statements_to_remove;

    /// current statement block being visited
    ast::StatementBlock* current_statement_block = nullptr;

  public:
    KineticBlockVisitor() = default;

    void visit_wrapped_expression(ast::WrappedExpression* node) override;
    void visit_reaction_operator(ast::ReactionOperator* node) override;
    void visit_react_var_name(ast::ReactVarName* node) override;
    void visit_reaction_statement(ast::ReactionStatement* node) override;
    void visit_conserve(ast::Conserve* node) override;
    void visit_compartment(ast::Compartment* node) override;
    void visit_statement_block(ast::StatementBlock* node) override;
    void visit_kinetic_block(ast::KineticBlock* node) override;
    void visit_program(ast::Program* node) override;
};

/** @} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl