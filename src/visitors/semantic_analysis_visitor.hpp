/*************************************************************************
 * Copyright (C) 2021-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::SemanticAnalysisVisitor
 */

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class SemanticAnalysisVisitor
 * \brief %Visitor to check some semantic rules on the ast
 *
 * Current checks:
 *
 * 1. Check that a function or a procedure containing a TABLE statement contains only one argument
 * (mandatory in mod2c).
 * 2. Check that destructor blocks are only inside mod file that are point_process
 */
#include "ast/ast.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

class SemanticAnalysisVisitor: public ConstAstVisitor {
  private:
    bool check_fail = false;

    /// true if the procedure or the function contains only one argument
    bool one_arg_in_procedure_function = false;
    /// true if we are in a procedure or a function block
    bool in_procedure_function = false;
    /// true if the mod file is of type point process
    bool is_point_process = false;

    /// Store if we are in a procedure and if the arity of this is 1
    void visit_procedure_block(const ast::ProcedureBlock& node) override;

    /// Store if we are in a function and if the arity of this is 1
    void visit_function_block(const ast::FunctionBlock& node) override;

    /// Visit a table statement and check that the arity of the block were 1
    void visit_table_statement(const ast::TableStatement& node) override;

    /// Visit destructor and check that the file is of type POINT_PROCESS or ARTIFICIAL_CELL
    void visit_destructor_block(const ast::DestructorBlock& node) override;

  public:
    SemanticAnalysisVisitor() = default;

    bool check(const ast::Program& node);
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
