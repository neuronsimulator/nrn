#ifndef NMODL_INLINE_VISITOR_HPP
#define NMODL_INLINE_VISITOR_HPP

#include <map>
#include <stack>

#include "ast/ast.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/visitor_utils.hpp"
#include "symtab/symbol_table.hpp"

/**
 * \class InlineVisitor
 * \brief Visitor to inline local procedure and function calls
 *
 * Motivation: Mod files often have function and procedure calls. Procedure
 * typically has use of range variables like:
 *
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
 *
 * One can reduce the memory bandwidth pressure by inlining rates() and then
 * replacing tau and beta with local variables. Many mod files from BlueBrain
 * and other open source projects could be hugely benefited by inlining pass.
 * The goal of this pass is to implement procedure and function inlining in
 * the nmodl programs. After inlining we should be asble to translate AST back
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
             PROCEDURE rates_1() {
                LOCAL x
                rates_2(23.1)
            }

            PROCEDURE rates_2(y) {
                LOCAL x
                x = 21.1*v+y
            }

 * The result of inlining :

           PROCEDURE rates_1() {
                LOCAL x, rates_2_in_0
                {
                    LOCAL x, y_in_0
                    y_in_0 = 23.1
                    x = 21.1*v+y_in_0
                    rates_2_in_0 = 0
                }
          }

          PROCEDURE rates_2(y) {
                LOCAL x
                x = 21.1*v+y
          }

 *  - Arguments for function call are copied into local variables
 *  - Local statement gets added to callee block (if doesn't exist)
 *  - Procedure body gets appended with extra assignment statement with variable
 *    used for returning value.
 *
 * \todo:
 *  - Recursive function calls are not supported and need to add checks to avoid stack explosion
 *  - Currently we rename variables more than necessary, this could be improved [low priority]
 *  - Function calls as part of an argument of function call itself are not completely inlined [low
 priority]
 *  - Symbol table pass needs to be re-run in order to update the definitions/usage
 *  - Location of symbol/nodes after inlining still points to old nodes
 */

class InlineVisitor : public AstVisitor {
  private:
    /// statement block containing current function call
    ast::StatementBlock* caller_block = nullptr;

    /// statement containing current function call
    std::shared_ptr<ast::Statement> caller_statement;

    /// symbol table for current statement block (or of parent block if doesn't have)
    std::shared_ptr<symtab::SymbolTable> caller_symtab;

    /// symbol tables in call hierarchy
    std::stack<std::shared_ptr<symtab::SymbolTable>> symtab_stack;

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
    bool can_inline_block(ast::StatementBlock* block);

    /// true if statement can be replaced with inlined body
    /// this is possible for standalone function/procedure call as statement
    bool can_replace_statement(const std::shared_ptr<ast::Statement>& statement);

    /// inline function/procedure into caller block
    template <typename T>
    void inline_function_call(T* callee, ast::FunctionCall* node, ast::StatementBlock* caller);

    /// add assignement statements into given statement block to inline arguments
    void inline_arguments(ast::StatementBlock* inlined_block,
                          const ast::ArgumentVector& callee_arguments,
                          const ast::ExpressionVector& caller_expressions);

    /// add assignment statement at end of block (to use as a return statement
    /// in case of procedure blocks)
    void add_return_variable(ast::StatementBlock* block, std::string& varname);

    /// add local statement to the block if doesn't exist already
    void add_local_statement(ast::StatementBlock* node);

  public:
    InlineVisitor() = default;

    virtual void visit_function_call(ast::FunctionCall* node) override;

    virtual void visit_statement_block(ast::StatementBlock* node) override;

    virtual void visit_wrapped_expression(ast::WrappedExpression* node) override;
};

/**
 * Inline function/procedure call
 * @tparam T
 * @param callee : ast node representing definition of function/procedure being called
 * @param node : function/procedure call node
 * @param caller : statement block containing function call
 */
template <typename T>
void InlineVisitor::inline_function_call(T* callee,
                                         ast::FunctionCall* node,
                                         ast::StatementBlock* caller) {
    std::string function_name = callee->name->get_name();

    /// do nothing if we can't inline given procedure/function
    if (!can_inline_block(callee->statementblock.get())) {
        std::cerr << "Can not inline function call to " + function_name;
        return;
    }

    auto local_variables = get_local_variables(caller);
    auto& caller_arguments = node->arguments;

    /// each block should already have local statement (added in statement block's visit function)
    if (local_variables == nullptr) {
        throw std::logic_error("got local statement as nullptr");
    }

    std::string new_varname = get_new_name(function_name, "in", inlined_variables);

    /// create new variable which will be used for returning value from inlined block
    auto name = new ast::Name(new ast::String(new_varname));
    ModToken tok;
    name->set_token(tok);

    local_variables->push_back(std::make_shared<ast::LocalVar>(name));

    /// get a copy of function/procedure body
    auto inlined_block = callee->statementblock->clone();

    /// function definition has function name as return value. we have to rename
    /// it with new variable name
    RenameVisitor visitor(function_name, new_varname);
    inlined_block->visit_children(&visitor);

    /// \todo: have to re-run symtab visitor pass to update symbol table
    inlined_block->set_symbol_table(nullptr);

    /// each argument is added as new assignment statement
    inline_arguments(inlined_block, callee->arguments, caller_arguments);

    /// to return value from procedure we have to add new variable
    if (callee->is_procedure_block()) {
        add_return_variable(inlined_block, new_varname);
    }

    /// check if caller statement could be replaced and add new statement to appropriate map
    bool to_replace = can_replace_statement(caller_statement);
    auto statement = new ast::ExpressionStatement(inlined_block);

    if (to_replace) {
        replaced_statements[caller_statement] = statement;
    } else {
        inlined_statements[caller_statement].push_back(
            std::shared_ptr<ast::ExpressionStatement>(statement));
    }

    /// variable name which will replace the function call that we just inlined
    replaced_fun_calls[node] = new_varname;
}

#endif  //
