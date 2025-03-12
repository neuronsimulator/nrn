/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::CodegenTransformVisitor
 */

#include "visitors/ast_visitor.hpp"

namespace nmodl {

/**
 * @addtogroup codegen_details
 * @{
 */

/**
 * \class CodegenTransformVisitor
 * \brief Visitor to make last transformation to AST before codegen
 *
 * Modifications made:
 * - add an argument to the table if it is inside a function. In this case
 *   the argument is the name of the function.
 */

class CodegenTransformVisitor: public visitor::AstVisitor {
  public:
    /// \name Ctor & dtor
    /// \{

    /// Default constructor
    CodegenTransformVisitor() = default;

    /// \}

    void visit_function_block(ast::FunctionBlock& node) override;
};

/** \} */  // end of codegen_details

}  // namespace nmodl
