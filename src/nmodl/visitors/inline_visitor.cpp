#include "visitors/inline_visitor.hpp"

using namespace ast;

bool InlineVisitor::can_inline_block(StatementBlock* block) {
    bool to_inline = true;
    for (const auto& statement : block->statements) {
        /// inlining is disabled if function/procedure contains table or lag statement
        if (statement->is_table_statement() || statement->is_lag_statement()) {
            to_inline = false;
            break;
        }
    }
    return to_inline;
}

void InlineVisitor::add_local_statement(StatementBlock* node) {
    auto variables = get_local_variables(node);
    if (variables == nullptr) {
        auto statement = std::make_shared<LocalListStatement>(LocalVarVector());
        node->statements.insert(node->statements.begin(), statement);
    }
}

void InlineVisitor::add_return_variable(StatementBlock* block, std::string& varname) {
    auto lhs = new Name(new String(varname));
    auto rhs = new Integer(0, nullptr);
    auto expression = new BinaryExpression(lhs, BinaryOperator(BOP_ASSIGN), rhs);
    auto statement = std::make_shared<ExpressionStatement>(expression);
    block->statements.push_back(statement);
}

/** We can replace statement if the entire statement itself is a function call.
 * In this case we check if:
 *  - given statement is expression statement
 *  - if expression is wrapped expression
 *  - if wrapped expression is a function call
 *
 * \todo: add method to ast itself to simplify this implementation
 */
bool InlineVisitor::can_replace_statement(const std::shared_ptr<Statement>& statement) {
    if (!statement->is_expression_statement()) {
        return false;
    }

    bool to_replace = false;
    auto es = static_cast<ExpressionStatement*>(statement.get());
    auto e = es->expression;
    if (e->is_wrapped_expression()) {
        auto wrapped_expression = static_cast<WrappedExpression*>(e.get());
        if (wrapped_expression->expression->is_function_call()) {
            to_replace = true;
        }
    }
    return to_replace;
}

void InlineVisitor::inline_arguments(StatementBlock* inlined_block,
                                     const ArgumentVector& callee_arguments,
                                     const ExpressionVector& caller_expressions) {
    /// nothing to inline if no arguments for function call
    if (caller_expressions.empty()) {
        return;
    }

    /// add local statement in the block if missing
    add_local_statement(inlined_block);

    auto local_variables = get_local_variables(inlined_block);
    auto& statements = inlined_block->statements;

    size_t counter = 0;

    for (const auto& argument : callee_arguments) {
        auto name = argument->name->clone();
        auto old_name = name->get_name();
        auto new_name = get_new_name(old_name, "in", inlined_variables);
        name->set_name(new_name);

        /// for argument add new variable to local statement
        local_variables->push_back(std::make_shared<LocalVar>(name));

        /// variables in cloned block needs to be renamed
        RenameVisitor visitor(old_name, new_name);
        inlined_block->visit_children(&visitor);

        auto lhs = new VarName(name->clone(), nullptr, nullptr);
        auto rhs = caller_expressions.at(counter)->clone();

        /// create assignment statement and insert after the local variables
        auto expression = new BinaryExpression(lhs, BinaryOperator(ast::BOP_ASSIGN), rhs);
        auto statement = std::make_shared<ExpressionStatement>(expression);
        statements.insert(statements.begin() + counter + 1, statement);
        counter++;
    }
}

void InlineVisitor::visit_function_call(FunctionCall* node) {
    /// argument can be function call itself
    node->visit_children(this);

    std::string function_name = node->name->get_name();
    auto symbol = program_symtab->lookup_in_scope(function_name);

    /// nothing to do if called function is not defined or it's external
    if (symbol == nullptr || symbol->is_external_symbol_only()) {
        return;
    }

    auto function_definition = symbol->get_node();
    if (function_definition == nullptr) {
        throw std::runtime_error("symbol table doesn't have ast node for " + function_name);
    }

    /// first inline called function
    function_definition->visit_children(this);

    if (function_definition->is_procedure_block()) {
        auto proc = (ProcedureBlock*)function_definition;
        inline_function_call(proc, node, caller_block);
    } else if (function_definition->is_function_block()) {
        auto func = (FunctionBlock*)function_definition;
        inline_function_call(func, node, caller_block);
    }
}

void InlineVisitor::visit_statement_block(StatementBlock* node) {
    /** While inlining we have to add new statements before call site.
     *  In order to return result we also have to add new local variable
     *  to the caller block. Hence we have to keep track of caller block,
     *  caller block's symbol table and caller statement.
     */
    caller_block = node;
    statementblock_stack.push(node);

    /** Add empty local statement at the begining of block if already doesn't exist.
     * Why? When we iterate over statements and inline function call, we have to add
     * local variable to return the result. As we can't modify vector while iterating,
     * we pre-add local statement without any local variables. If inlining pass doesn't
     * add any variable then this statement will be removed.
     */
    add_local_statement(node);

    auto& statements = node->statements;

    for (auto& statement : statements) {
        caller_statement = statement;
        statement_stack.push(statement);
        caller_statement->visit_children(this);
        statement_stack.pop();
    }

    /// if nothing was added into local statement, remove it
    LocalVarVector* local_variables = get_local_variables(node);
    if (local_variables->empty()) {
        statements.erase(statements.begin());
    }

    /// check if any statement is candidate for replacement due to inlining
    /// this is typicall case of procedure calls
    for (auto& statement : statements) {
        if (replaced_statements.find(statement) != replaced_statements.end()) {
            statement.reset(replaced_statements[statement]);
        }
    }

    /// all statements from inlining needs to be added before the caller statement
    for (auto& element : inlined_statements) {
        auto it = std::find(statements.begin(), statements.end(), element.first);
        if (it != statements.end()) {
            statements.insert(it, element.second.begin(), element.second.end());
            element.second.clear();
        }
    }

    /** Restore the caller context : consider call chain A() -> B() -> none.
     * When we finishes processing B's statements, even we pop the stack,
     * caller_* variables still point to B's context. Hence first we have
     * to pop elements and then use top() to get context of A(). We have to
     * check for non-empty() stack because if there is only A() -> none then
     * stack is already empty.
     */
    statementblock_stack.pop();

    if (!statement_stack.empty()) {
        caller_statement = statement_stack.top();
    }
    if (!statementblock_stack.empty()) {
        caller_block = statementblock_stack.top();
    }
}

/** Visit all wrapped expressions which can contain function calls.
 *  If a function call is replaced then the wrapped expression is
 *  also replaced with new variable node from the inlining result.
 */
void InlineVisitor::visit_wrapped_expression(WrappedExpression* node) {
    node->visit_children(this);
    if (node->expression->is_function_call()) {
        auto expression = static_cast<FunctionCall*>(node->expression.get());
        if (replaced_fun_calls.find(expression) != replaced_fun_calls.end()) {
            auto var = replaced_fun_calls[expression];
            node->expression = std::make_shared<Name>(new String(var));
        }
    }
}

void InlineVisitor::visit_program(Program* node) {
   program_symtab = node->get_symbol_table();
   if(program_symtab == nullptr) {
       throw std::runtime_error("Program node doesn't have symbol table");
   }
    node->visit_children(this);
}