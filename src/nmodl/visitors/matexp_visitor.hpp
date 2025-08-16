/*
 * Copyright 2025 David McDougall
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::MatexpVisitor
 */

#include "ast/ast.hpp"
#include "symtab/symbol.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class MatexpVisitor
 * \brief Visitor used for generating the necessary AST nodes for matexp solver
 */
class MatexpVisitor: public AstVisitor {
  private:
    /// ordered list of state variables
    std::vector<std::shared_ptr<symtab::Symbol>> states;

    /// blocks to be solved
    std::vector<std::string> steadystate_blocks;

    /// blocks to be solved
    std::vector<std::string> solve_blocks;

    /// blocks to be kept unchanged and solved by a different method
    std::vector<std::string> keep_blocks;

    /// visiting kinetic block that is being solved
    bool in_kinetic_block = false;

    /// switch for solve/steadystate, either "dt" or "1000000000"
    std::string dt;

    /// blocks having been solved, waiting to be appended to the program
    std::vector<std::shared_ptr<ast::ProcedureBlock>> solved_blocks;

    void solve_kinetic_block(const ast::KineticBlock& node, bool steadystate);

    int get_state_index(const std::string& state_name);

  public:
    MatexpVisitor() = default;

    void visit_program(ast::Program& node) override;
    void visit_solve_block(ast::SolveBlock& node) override;
    void visit_kinetic_block(ast::KineticBlock& node) override;
    void visit_statement_block(ast::StatementBlock& node) override;
    void visit_reaction_statement(ast::ReactionStatement& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
