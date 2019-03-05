/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <set>
#include <vector>

#include "ast/ast.hpp"
#include "symtab/symbol.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {

/**
 * \class SympySolverVisitor
 * \brief Visitor for differential equations in derivative block
 *
 * When solver method is "cnexp", this class replaces the
 * differential equations with their analytic solution using SymPy,
 * i.e. it duplicates the functionality of CnexpSolveVisitor
 *
 * It will soon also deal with other solver methods.
 */

class SympySolverVisitor: public AstVisitor {
  private:
    std::set<std::string> vars;
    /// method specified in solve block
    std::string solve_method;

    /// solver method names
    const std::string cnexp_method = "cnexp";
    /// optionally replace solution with (1,1) pade approx
    bool use_pade_approx;

  public:
    SympySolverVisitor(bool use_pade_approx = false)
        : use_pade_approx(use_pade_approx){};

    void visit_solve_block(ast::SolveBlock* node) override;
    void visit_diff_eq_expression(ast::DiffEqExpression* node) override;
    void visit_derivative_block(ast::DerivativeBlock* node) override;
    void visit_program(ast::Program* node) override;
};

}  // namespace nmodl