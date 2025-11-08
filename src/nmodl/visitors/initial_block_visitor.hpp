/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::MergeInitialBlocksVisitor
 */

#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class MergeInitialBlocksVisitor
 * \brief Visitor which merges all INITIAL blocks into one
 */
class MergeInitialBlocksVisitor: public AstVisitor {
  public:
    void visit_program(ast::Program& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
