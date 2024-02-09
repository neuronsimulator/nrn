/*
 * Copyright 2024 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::FunctionCallpathVisitor
 */

#include "ast/all.hpp"
#include "symtab/decl.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class FunctionCallpathVisitor
 * \brief %Visitor for traversing \c FunctionBlock s and \c ProcedureBlocks through
 *        their \c FunctionCall s
 *
 * This visitor is used to traverse the \c FUNCTION s and \c PROCEDURE s in the NMODL files.
 * It visits the \c FunctionBlock s and \c ProcedureBlock s and if there is a \c FunctionCall
 * in those, it visits the \c FunctionBlock or \c ProcedureBlock of the \c FunctionCall.
 * Currently it only checks whether in this path of function calls there is any use of \c RANGE ,
 * \c POINTER or \c BBCOREPOINTER variable. In case there is it adds the \c use_range_ptr_var
 * property in the \c Symbol of the function or procedure in the program \c SymbolTable and does the
 * same recursively for all the caller functions. The \c use_range_ptr_var property is used later in
 * the \c CodegenNeuronCppVisitor .
 *
 */
class FunctionCallpathVisitor: public ConstAstVisitor {
  private:
    /// Vector of currently visited functions or procedures (used as a searchable stack)
    std::vector<const ast::Block*> visited_functions_or_procedures;

    /// symbol table for the program
    symtab::SymbolTable* psymtab = nullptr;

  public:
    void visit_var_name(const ast::VarName& node) override;

    void visit_function_call(const ast::FunctionCall& node) override;

    void visit_function_block(const ast::FunctionBlock& node) override;

    void visit_procedure_block(const ast::ProcedureBlock& node) override;

    void visit_program(const ast::Program& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
