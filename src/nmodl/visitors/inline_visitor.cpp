/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/inline_visitor.hpp"

#include "ast/all.hpp"
#include "parser/c11_driver.hpp"
#include "utils/logger.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/visitor_utils.hpp"


namespace nmodl {
namespace visitor {

using namespace ast;

bool InlineVisitor::can_inline_block(const StatementBlock& block) const {
    bool to_inline = true;
    const auto& statements = block.get_statements();
    for (const auto& statement: statements) {
        /// inlining is disabled if function/procedure contains table or lag statement
        if (statement->is_table_statement() || statement->is_lag_statement()) {
            to_inline = false;
            break;
        }
        // verbatim blocks with return statement are not safe to inline
        // especially for net_receive block
        if (statement->is_verbatim()) {
            const auto node = static_cast<const Verbatim*>(statement.get());
            auto text = node->get_statement()->eval();
            parser::CDriver driver;
            driver.scan_string(text);
            if (driver.has_token("return")) {
                to_inline = false;
                break;
            }
        }
    }
    return to_inline;
}

void InlineVisitor::add_return_variable(StatementBlock& block, std::string& varname) {
    auto lhs = new Name(new String(varname));
    auto rhs = new Integer(0, nullptr);
    auto expression = new BinaryExpression(lhs, BinaryOperator(BOP_ASSIGN), rhs);
    auto statement = std::make_shared<ExpressionStatement>(expression);
    block.emplace_back_statement(statement);
}

/** We can replace statement if the entire statement itself is a function call.
 * In this case we check if:
 *  - given statement is expression statement
 *  - if expression is wrapped expression
 *  - if wrapped expression is a function call
 *
 * \todo Add method to ast itself to simplify this implementation
 */
bool InlineVisitor::can_replace_statement(const std::shared_ptr<Statement>& statement) {
    if (!statement->is_expression_statement()) {
        return false;
    }

    bool to_replace = false;
    auto es = static_cast<ExpressionStatement*>(statement.get());
    auto e = es->get_expression();
    if (e->is_wrapped_expression()) {
        auto wrapped_expression = static_cast<WrappedExpression*>(e.get());
        if (wrapped_expression->get_expression()->is_function_call()) {
            // if caller is external function (i.e. neuron function) don't replace it
            const auto& function_call = std::static_pointer_cast<FunctionCall>(
                wrapped_expression->get_expression());
            const auto& function_name = function_call->get_node_name();
            const auto& symbol = program_symtab->lookup_in_scope(function_name);
            to_replace = !symbol->is_external_variable();
        }
    }
    return to_replace;
}

void InlineVisitor::inline_arguments(StatementBlock& inlined_block,
                                     const ArgumentVector& callee_parameters,
                                     const ExpressionVector& caller_expressions) {
    /// nothing to inline if no arguments for function call
    if (caller_expressions.empty()) {
        return;
    }

    size_t counter = 0;
    const auto& statements = inlined_block.get_statements();

    for (const auto& argument: callee_parameters) {
        auto name = argument->get_name()->clone();
        auto old_name = name->get_node_name();
        auto new_name = get_new_name(old_name, "in", inlined_variables);
        name->set_name(new_name);

        /// for argument add new variable to local statement
        add_local_variable(inlined_block, name);

        /// variables in cloned block needs to be renamed
        RenameVisitor visitor(old_name, new_name);
        inlined_block.visit_children(visitor);

        auto lhs = new VarName(name->clone(), nullptr, nullptr);
        auto rhs = caller_expressions.at(counter)->clone();

        /// create assignment statement and insert after the local variables
        auto expression = new BinaryExpression(lhs, BinaryOperator(ast::BOP_ASSIGN), rhs);
        auto statement = std::make_shared<ExpressionStatement>(expression);
        inlined_block.insert_statement(statements.begin() + counter + 1, statement);
        counter++;
    }
}


bool InlineVisitor::inline_function_call(ast::Block& callee,
                                         ast::FunctionCall& node,
                                         ast::StatementBlock& caller) {
    const auto& function_name = callee.get_node_name();

    /// do nothing if we can't inline given procedure/function
    if (!can_inline_block(*callee.get_statement_block())) {
        logger->warn("Can not inline function call to {}", function_name);
        return false;
    }

    /// make sure to rename conflicting local variable in caller block
    /// because in case of procedure inlining they can conflict with
    /// global variables used in procedure being inlined
    LocalVarRenameVisitor v;
    v.visit_statement_block(caller);

    const auto& caller_arguments = node.get_arguments();
    std::string new_varname = get_new_name(function_name, "in", inlined_variables);

    /// check if caller statement could be replaced
    bool to_replace = can_replace_statement(caller_statement);

    /// need to add local variable for function calls or for procedure call if it is part of
    /// expression (standalone procedure calls don't need return statement)
    if (callee.is_function_block() || to_replace == false) {
        /// create new variable which will be used for returning value from inlined block
        auto name = new ast::Name(new ast::String(new_varname));
        ModToken tok;
        name->set_token(tok);

        auto local_list_statement = get_local_list_statement(caller);
        /// each block should already have local statement
        if (local_list_statement == nullptr) {
            throw std::logic_error("got local statement as nullptr");
        }
        local_list_statement->emplace_back_local_var(std::make_shared<ast::LocalVar>(name));
    }

    /// get a copy of function/procedure body
    auto inlined_block = callee.get_statement_block()->clone();

    /// function definition has function name as return value. we have to rename
    /// it with new variable name
    RenameVisitor visitor(function_name, new_varname);
    inlined_block->visit_children(visitor);

    /// \todo Have to re-run symtab visitor pass to update symbol table
    inlined_block->set_symbol_table(nullptr);

    /// each argument is added as new assignment statement
    inline_arguments(*inlined_block, callee.get_parameters(), caller_arguments);

    /// to return value from procedure we have to add new variable
    if (callee.is_procedure_block() && to_replace == false) {
        add_return_variable(*inlined_block, new_varname);
    }

    auto statement = new ast::ExpressionStatement(inlined_block);

    if (to_replace) {
        replaced_statements[caller_statement] = statement;
    } else {
        inlined_statements[caller_statement].push_back(
            std::shared_ptr<ast::ExpressionStatement>(statement));
    }

    /// variable name which will replace the function call that we just inlined
    replaced_fun_calls[&node] = new_varname;
    return true;
}


void InlineVisitor::visit_function_call(FunctionCall& node) {
    /// argument can be function call itself
    node.visit_children(*this);

    const auto& function_name = node.get_name()->get_node_name();
    auto symbol = program_symtab->lookup_in_scope(function_name);

    /// nothing to do if called function is not defined or it's external
    if (symbol == nullptr || symbol->is_external_variable()) {
        return;
    }

    auto function_definition = symbol->get_node();
    if (function_definition == nullptr) {
        throw std::runtime_error("symbol table doesn't have ast node for " + function_name);
    }

    /// first inline called function
    function_definition->visit_children(*this);

    bool inlined = false;

    if (function_definition->is_procedure_block()) {
        auto proc = (ProcedureBlock*) function_definition;
        inlined = inline_function_call(*proc, node, *caller_block);
    } else if (function_definition->is_function_block()) {
        auto func = (FunctionBlock*) function_definition;
        inlined = inline_function_call(*func, node, *caller_block);
    }

    if (inlined) {
        symbol->mark_inlined();
    }
}

void InlineVisitor::visit_statement_block(StatementBlock& node) {
    /** While inlining we have to add new statements before call site.
     *  In order to return result we also have to add new local variable
     *  to the caller block. Hence we have to keep track of caller block,
     *  caller block's symbol table and caller statement.
     */
    caller_block = &node;
    statementblock_stack.push(&node);

    /** Add empty local statement at the begining of block if already doesn't exist.
     * Why? When we iterate over statements and inline function call, we have to add
     * local variable to return the result. As we can't modify vector while iterating,
     * we pre-add local statement without any local variables. If inlining pass doesn't
     * add any variable then this statement will be removed.
     */
    add_local_statement(node);

    const auto& statements = node.get_statements();

    for (const auto& statement: statements) {
        caller_statement = statement;
        statement_stack.push(statement);
        caller_statement->visit_children(*this);
        statement_stack.pop();
    }

    /// each block should already have local statement
    auto local_list_statement = get_local_list_statement(node);
    if (local_list_statement->get_variables().empty()) {
        node.erase_statement(statements.begin());
    }

    /// check if any statement is candidate for replacement due to inlining
    /// this is typicall case of procedure calls
    for (auto it = statements.begin(); it < statements.end(); ++it) {
        const auto& statement = *it;
        if (replaced_statements.find(statement) != replaced_statements.end()) {
            node.reset_statement(it, replaced_statements[statement]);
        }
    }

    /// all statements from inlining needs to be added before the caller statement
    for (auto& element: inlined_statements) {
        auto it = std::find(statements.begin(), statements.end(), element.first);
        if (it != statements.end()) {
            node.insert_statement(it, element.second, element.second.begin(), element.second.end());
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
void InlineVisitor::visit_wrapped_expression(WrappedExpression& node) {
    node.visit_children(*this);
    const auto& e = node.get_expression();
    if (e->is_function_call()) {
        auto expression = dynamic_cast<FunctionCall*>(e.get());
        if (replaced_fun_calls.find(expression) != replaced_fun_calls.end()) {
            auto var = replaced_fun_calls[expression];
            node.set_expression(std::make_shared<Name>(new String(var)));
        }
    }
}

void InlineVisitor::visit_program(Program& node) {
    program_symtab = node.get_symbol_table();
    if (program_symtab == nullptr) {
        throw std::runtime_error("Program node doesn't have symbol table");
    }
    node.visit_children(*this);
}

}  // namespace visitor
}  // namespace nmodl
