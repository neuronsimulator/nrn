/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::SemanticAnalysisVisitor
 */

#include "ast/ast.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class SemanticAnalysisVisitor
 * \brief %Visitor to check some semantic rules on the AST
 *
 * Current checks:
 *
 * 1. Check that a function or a procedure containing a TABLE statement contains only one argument
 * (mandatory in mod2c).
 * 2. Check that destructor blocks are only inside mod file that are point_process.
 * 3. A TABLE statement in functions cannot have name list, and should have one in procedures.
 * 4. Check if ion variables from a `USEION` statement are not declared in `CONSTANT` block.
 * 5. Check if an independent variable is not 't'.
 * 6. Check that mutex are not badly use
 * 7. Check than function table got at least one argument.
 * 8. Check that at most one derivative block is present.
 * 9. Check that RANDOM variable is mentioned only as first arg in random function.
 */

class SemanticAnalysisVisitor: public ConstAstVisitor {
  private:
    // if semantic analysis check has failed
    bool check_fail = false;
    // symbol table for the program
    symtab::SymbolTable* program_symtab = nullptr;
    /// true if accelerator backend is used for code generation
    bool accel_backend = false;
    /// true if the procedure or the function contains only one argument
    bool one_arg_in_procedure_function = false;
    /// true if we are in a procedure block
    bool in_procedure = false;
    /// true if we are in a function block
    bool in_function = false;
    /// true if the mod file is of type point process
    bool is_point_process = false;
    /// true if we are inside a mutex locked part
    bool in_mutex = false;

    /// Check number of DERIVATIVE blocks
    void visit_program(const ast::Program& node) override;

    /// Store if we are in a procedure and if the arity of this is 1
    void visit_procedure_block(const ast::ProcedureBlock& node) override;

    /// Store if we are in a function and if the arity of this is 1
    void visit_function_block(const ast::FunctionBlock& node) override;

    /// Visit a table statement and check that the arity of the block were 1
    void visit_table_statement(const ast::TableStatement& node) override;

    /// Visit destructor and check that the file is of type POINT_PROCESS or ARTIFICIAL_CELL
    void visit_destructor_block(const ast::DestructorBlock& node) override;

    /// Visit independent block and check if one of the variable is not t
    void visit_independent_block(const ast::IndependentBlock& node) override;

    /// Visit function table to check that number of args > 0
    void visit_function_table_block(const ast::FunctionTableBlock& node) override;

    /// Look if protect is inside a locked block
    void visit_protect_statement(const ast::ProtectStatement& node) override;

    /// Look if MUTEXLOCK is inside a locked block
    void visit_mutex_lock(const ast::MutexLock& node) override;

    /// Look if MUTEXUNLOCK is outside a locked block
    void visit_mutex_unlock(const ast::MutexUnlock& node) override;

    /// Only use of random_var is as first arg in random function.
    void visit_name(const ast::Name& node) override;

    /// random function first arg must be random_var
    void visit_function_call(const ast::FunctionCall& node) override;

    bool check_name_conflict(const ast::Program& node);

    bool check_table_vars(const ast::Program& node);
  public:
    SemanticAnalysisVisitor(bool accel_backend = false)
        : accel_backend(accel_backend) {}
    bool check(const ast::Program& node);
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
