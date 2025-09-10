/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::ImplicitMethodVisitor
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
 * \class ImplicitMethodVisitor
 * \brief %Visitor for adding implicit method to SOLVE blocks
 */
class ImplicitMethodVisitor: public AstVisitor {
  private:
    std::unordered_set<std::string> derivative_block_names;

  public:
    void visit_program(ast::Program& node) override;
    void visit_solve_block(ast::SolveBlock& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
