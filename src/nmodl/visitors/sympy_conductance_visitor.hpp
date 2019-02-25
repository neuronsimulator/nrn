/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <map>
#include <set>
#include <vector>

#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include "ast/ast.hpp"
#include "symtab/symbol.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/lookup_visitor.hpp"

/**
 * \class SympyConductanceVisitor
 * \brief Visitor for generating CONDUCTANCE statements for ions
 *
 * This class visits each ion expresion I = ... in the breakpoint
 * and symbolically differentiates it to get dI/dV,
 * i.e. the conductance.
 *
 * If this coincides with an existing global variable g
 * (e.g. when I is linear in V) then it will add
 * CONDUCTANCE g USEION I
 *
 * If dI/dV is a more complicated expression, it generates
 * a new unique variable name g_unique, and adds two lines
 * CONDUCTANCE g_unique USEION I
 * g_unique = [dI/dV expression]
 *
 * If an ion channel already has a CONDUCTANCE statement
 * then it does not modify it.
 *
 * TODO: take into account any functions called in breakpoint
 */

class SympyConductanceVisitor: public AstVisitor {
  private:
    /// true while visiting breakpoint block
    bool breakpoint_block = false;
    typedef std::map<std::string, std::string> string_map;
    typedef std::set<std::string> string_set;
    // set of all variables for SymPy
    string_set vars;
    // set of currents to ignore
    string_set i_ignore;
    // map between current write names and ion names
    string_map i_name;
    bool NONSPECIFIC_CONDUCTANCE_ALREADY_EXISTS = false;
    // list in order of binary expressions in breakpoint
    std::vector<std::string> ordered_binary_exprs;
    // ditto but for LHS of expression only
    std::vector<std::string> ordered_binary_exprs_lhs;
    // map from lhs of binary expression to index of expression in above vector
    std::map<std::string, std::size_t> binary_expr_index;
    std::vector<std::shared_ptr<ast::AST>> use_ion_nodes;
    std::vector<std::shared_ptr<ast::AST>> nonspecific_nodes;

    std::vector<std::string> generate_statement_strings(ast::BreakpointBlock* node);
    void lookup_useion_statements();
    void lookup_nonspecific_statements();

  public:
    SympyConductanceVisitor() = default;
    void visit_binary_expression(ast::BinaryExpression* node) override;
    void visit_breakpoint_block(ast::BreakpointBlock* node) override;
    void visit_conductance_hint(ast::ConductanceHint* node) override;
    void visit_program(ast::Program* node) override;
};