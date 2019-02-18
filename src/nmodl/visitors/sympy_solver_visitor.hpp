#pragma once

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <set>
#include <vector>

#include "ast/ast.hpp"
#include "symtab/symbol.hpp"
#include "visitors/ast_visitor.hpp"

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

  public:
    SympySolverVisitor() = default;

    void visit_solve_block(ast::SolveBlock* node) override;
    void visit_diff_eq_expression(ast::DiffEqExpression* node) override;
    void visit_derivative_block(ast::DerivativeBlock* node) override;
    void visit_program(ast::Program* node) override;
};