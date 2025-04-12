/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::CvodeVisitor
 */

#include "symtab/decl.hpp"
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
 * \class CvodeVisitor
 * \brief Visitor used for generating the necessary AST nodes for CVODE
 */
class CvodeVisitor: public AstVisitor {
  public:
    void visit_program(ast::Program& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
