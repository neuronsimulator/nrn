/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <map>
#include <set>
#include <string>

#include "ast/ast.hpp"

namespace nmodl {

/** Return new name variable by appending "_suffix_COUNT" where COUNT is number
 * of times the given variable is already used.
 */
std::string get_new_name(const std::string& name,
                         const std::string& suffix,
                         std::map<std::string, int>& variables);

/** Return pointer to local statement in the given block, otherwise nullptr */
ast::LocalVarVector* get_local_variables(const ast::StatementBlock* node);

/** Add empty local statement to given block if already doesn't exist */
void add_local_statement(ast::StatementBlock* node);

/** Add new local variable to the block */
ast::LocalVar* add_local_variable(ast::StatementBlock* node, const std::string& varname);
ast::LocalVar* add_local_variable(ast::StatementBlock* node, ast::Identifier* varname);
ast::LocalVar* add_local_variable(ast::StatementBlock* node, const std::string& varname, int dim);

/** Create ast statement node from given code in string format */
std::shared_ptr<ast::Statement> create_statement(const std::string& code_statement);

/** Create ast statement block node from given code in string format */
std::shared_ptr<ast::StatementBlock> create_statement_block(
    const std::vector<std::string>& code_statements);

/**  Remove statements from given statement block if they exist */
void remove_statements_from_block(ast::StatementBlock* block,
                                  const std::set<ast::Node*> statements);

/** Return set of strings with the names of all global variables */
std::set<std::string> get_global_vars(ast::Program* node);

/** Given AST node, return the NMODL string representation */
std::string to_nmodl(ast::AST* node, const std::set<ast::AstNodeType>& exclude_types = {});

/** Given AST node, return the JSON string representation */
std::string to_json(ast::AST* node, bool compact = false, bool expand = false);

}  // namespace nmodl
