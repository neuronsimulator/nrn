/*
 * Copyright 2025 David McDougall
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::MatexpVisitor
 */

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
    /// blocks to be solved
    std::vector<ast::SolveBlock*> steadystate_blocks;

    /// blocks to be solved
    std::vector<ast::SolveBlock*> solve_blocks;

    /// blocks to be solved by a different solver method
    std::vector<std::string> keep_blocks;

    /// all kinetic blocks in the program
    std::vector<ast::KineticBlock*> kinetic_blocks;

    ast::KineticBlock* find_kinetic_block(const std::string& block_name);

    std::shared_ptr<ast::MatexpBlock> solve_kinetic_block(const ast::KineticBlock& node,
                                                          bool steadystate);

    void replace_solve_block(const ast::SolveBlock& node, bool steadystate);

    std::shared_ptr<ast::MatexpBlock> remove_solve_block(const ast::SolveBlock& node, bool steadystate);

    /// ordered list of state variables
    std::vector<std::shared_ptr<symtab::Symbol>> states;

    int get_state_index(const std::string& state_name);

    /// currently visiting kinetic block that is being solved
    bool in_jacobian_block = false;

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
