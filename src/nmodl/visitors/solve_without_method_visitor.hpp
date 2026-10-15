/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::SolveWithoutMethodVisitor
 */

#include "visitors/ast_visitor.hpp"

#include <string>
#include <unordered_set>


namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class SolveWithoutMethodVisitor
 * \brief %Visitor for adding an explicit method to a SOLVE block which has an implicit one
 */
class SolveWithoutMethodVisitor: public AstVisitor {
  private:
    std::unordered_set<std::string> derivative_block_names;

  public:
    void visit_program(ast::Program& node) override;
    void visit_solve_block(ast::SolveBlock& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
