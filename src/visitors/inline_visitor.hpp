/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::InlineVisitor
 */

#include <map>
#include <stack>

#include "symtab/decl.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class InlineVisitor
 * \brief %Visitor to inline local procedure and function calls
 *
 * Motivation: Mod files often have function and procedure calls. Procedure
 * typically has use of range variables like:
 *
 * \code{.mod}
 *      NEURON {
 *          RANGE tau, alpha, beta
 *      }
 *
 *      DERIVATIVE states() {
 *          ...
 *          rates()
 *          alpha = tau + beta
 *      }
 *
 *      PROCEDURE rates() {
 *          tau = xx * 0.12 * some_var
 *          beta = yy * 0.11
 *      }
 * \endcode
 *
 * One can reduce the memory bandwidth pressure by inlining rates() and then
 * replacing tau and beta with local variables. Many mod files from BlueBrain
 * and other open source projects could be hugely benefited by inlining pass.
 * The goal of this pass is to implement procedure and function inlining in
 * the nmodl programs. After inlining we should be able to translate AST back
 * to "transformed" nmodl program which can be compiled and run by NEURON or
 * CoreNEURON simulator.
 *
 * Implementation Notes:
 *  - We start with iterating statements in the statement block
 *  - Function or procedure calls are treated in same way
 *  - We visit the childrens of a statement and "trap" function calls
 *  - New ast node WrappedExpression has been added to YAML specification
 *    to facilitate easily "capturing" function calls. For example, function
 *    call can appear in argument, condition, binary expression, unary expression
 *    etc. When inlining happens, we have to replace FunctionCall node with new
 *    variable name. As visitor receives the pointer to function call, we can't
 *    replace the node. Hence WrappedExpression node has added. With this we
 *    can just override visit function of a single node and then can easily
 *    replace the expression to whatever the result will be
 *  - In case of lag and table statement inlining is disabled (haven't carefully
 *    looked into side effects)
 *  - Inlining pass needs to be run after symbol table pass
 *  - Inlining is done in recursive way i.e. if A() calls B() then first B()
 *    is checked for function calls/inlining and so on. This is dony by looking
 *    ast node of B() in symbol table and visiting it recursively
 *  - Procedure calls are typically standalone statements i.e. expression statements.
 *    But note that the nmodl grammar allows procedures to appear as part of expression.
 *    Procedures always return 0
 *  - Standalone procedure or function calls are replaced with the procedure/function body
 *
 *  Examples:
 *
 * \code{.mod}
 *      PROCEDURE rates_1() {
 *          LOCAL x
 *          rates_2(23.1)
 *      }
 *
 *      PROCEDURE rates_2(y) {
 *          LOCAL x
 *          x = 21.1*v+y
 *      }
 * \endcode
 *
 * The result of inlining :
 *
 * \code{.mod}
 *      PROCEDURE rates_1() {
 *            LOCAL x, rates_2_in_0
 *            {
 *                LOCAL x, y_in_0
 *                y_in_0 = 23.1
 *                x = 21.1*v+y_in_0
 *                rates_2_in_0 = 0
 *            }
 *      }
 *
 *      PROCEDURE rates_2(y) {
 *            LOCAL x
 *            x = 21.1*v+y
 *      }
 * \endcode
 *
 *  - Arguments for function call are copied into local variables
 *  - Local statement gets added to callee block (if doesn't exist)
 *  - Procedure body gets appended with extra assignment statement with variable
 *    used for returning value.
 *
 * \todo
 *  - Recursive function calls are not supported and need to add checks to avoid stack explosion
 *  - Currently we rename variables more than necessary, this could be improved [low priority]
 *  - Function calls as part of an argument of function call itself are not completely inlined [low
 *    priority]
 *  - Symbol table pass needs to be re-run in order to update the definitions/usage
 *  - Location of symbol/nodes after inlining still points to old nodes
 */
class InlineVisitor: public AstVisitor {
  private:
    /// statement block containing current function call
    ast::StatementBlock* caller_block = nullptr;

    /// statement containing current function call
    std::shared_ptr<ast::Statement> caller_statement;

    /// symbol table for program node
    symtab::SymbolTable const* program_symtab = nullptr;

    /// statement blocks in call hierarchy
    std::stack<ast::StatementBlock*> statementblock_stack;

    /// statements being executed in call hierarchy
    std::stack<std::shared_ptr<ast::Statement>> statement_stack;

    /// map to track the statements being replaces (typically for procedure calls)
    std::map<std::shared_ptr<ast::Statement>, ast::ExpressionStatement*> replaced_statements;

    /// map to track statements being prepended before function call (typically for function calls)
    std::map<std::shared_ptr<ast::Statement>,
             std::vector<std::shared_ptr<ast::ExpressionStatement>>>
        inlined_statements;

    /// map to track replaced function calls (typically for function calls)
    std::map<ast::FunctionCall*, std::string> replaced_fun_calls;

    /// variables currently being renamed and their count (for renaming)
    std::map<std::string, int> inlined_variables;

    /// true if given statement block can be inlined
    bool can_inline_block(const ast::StatementBlock& block) const;

    /// true if statement can be replaced with inlined body
    /// this is possible for standalone function/procedure call as statement
    bool can_replace_statement(const std::shared_ptr<ast::Statement>& statement);

    /**
     * inline function/procedure into caller block
     * @param callee : ast node representing function/procedure definition being called
     * @param node : function/procedure call node
     * @param caller : statement block containing function call
     */
    bool inline_function_call(ast::Block& callee,
                              ast::FunctionCall& node,
                              ast::StatementBlock& caller);

    /// add assignment statements into given statement block to inline arguments
    void inline_arguments(ast::StatementBlock& inlined_block,
                          const ast::ArgumentVector& callee_parameters,
                          const ast::ExpressionVector& caller_expressions);

    /// add assignment statement at end of block (to use as a return statement
    /// in case of procedure blocks)
    void add_return_variable(ast::StatementBlock& block, std::string& varname);

  public:
    InlineVisitor() = default;

    void visit_function_call(ast::FunctionCall& node) override;

    void visit_statement_block(ast::StatementBlock& node) override;

    void visit_wrapped_expression(ast::WrappedExpression& node) override;

    void visit_program(ast::Program& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
