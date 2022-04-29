/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::NeuronSolveVisitor
 */

#include <map>
#include <string>

#include "symtab/decl.hpp"
#include "visitors/ast_visitor.hpp"


namespace nmodl {
namespace visitor {

/**
 * \addtogroup solver
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class NeuronSolveVisitor
 * \brief %Visitor that solves ODEs using old solvers of NEURON
 *
 * This pass solves ODEs in derivative block using `cnexp`, `euler` and
 * `derivimplicit`method. This solved mimics original implementation in
 * nocmodl/mod2c. The original ODEs get replaced with the solution and
 * transformations are performed at AST level.
 *
 * \sa nmodl::visitor::SympySolverVisitor
 */
class NeuronSolveVisitor: public AstVisitor {
  private:
    /// true while visiting differential equation
    bool differential_equation = false;

    /// global symbol table
    symtab::SymbolTable* program_symtab = nullptr;

    /// a map holding solve block names and methods
    std::map<std::string, std::string> solve_blocks;

    /// method specified in solve block
    std::string solve_method;

    /// visiting derivative block
    bool derivative_block = false;

    /// the derivative name currently being visited
    std::string derivative_block_name;

  public:
    NeuronSolveVisitor() = default;

    void visit_solve_block(ast::SolveBlock& node) override;
    void visit_diff_eq_expression(ast::DiffEqExpression& node) override;
    void visit_derivative_block(ast::DerivativeBlock& node) override;
    void visit_binary_expression(ast::BinaryExpression& node) override;
    void visit_program(ast::Program& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
