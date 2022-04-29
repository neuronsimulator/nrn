/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief Utility functions for visitors implementation
 */

#include <map>
#include <set>
#include <string>
#include <unordered_set>

#include <ast/ast_decl.hpp>
#include <utils/common_utils.hpp>

namespace nmodl {
namespace visitor {

using nmodl::utils::UseNumbersInString;

/**
 * \brief Return the "original_string" with a random suffix if "original_string" exists in "vars"
 *
 * Return a std::string in the form "original_string"_"random_string", where
 * random_string is a string defined in the nmodl::utils::SingletonRandomString
 * for the original_string. Vars is a const ref to std::set<std::string> which
 * holds the names that need to be checked for uniqueness. Choose if the
 * "random_string" will include numbers using "use_num"
 *
 * \param vars a const ref to std::set<std::string> which holds the names of the variables we should
 *             check for uniqueness. Normally this is a vector of all the variables defined in the
 *             mod file.
 * \param original_string the original string to be suffixed with a random string
 * \param use_num a UseNumbersInString enum value to choose if the random string will include
 *                numbers
 * \return std::string the new string with the proper suffix if needed
 */
std::string suffix_random_string(
    const std::set<std::string>& vars,
    const std::string& original_string,
    const UseNumbersInString use_num = nmodl::utils::UseNumbersInString::WithNumbers);

/// Return new name variable by appending `_suffix_COUNT` where `COUNT` is
/// number of times the given variable is already used.
std::string get_new_name(const std::string& name,
                         const std::string& suffix,
                         std::map<std::string, int>& variables);


/// Return pointer to local statement in the given block, otherwise nullptr
std::shared_ptr<ast::LocalListStatement> get_local_list_statement(const ast::StatementBlock& node);


/// Add empty local statement to given block if already doesn't exist
void add_local_statement(ast::StatementBlock& node);


/// Add new local variable to the block
ast::LocalVar* add_local_variable(ast::StatementBlock& node, const std::string& varname);
ast::LocalVar* add_local_variable(ast::StatementBlock& node, ast::Identifier* varname);
ast::LocalVar* add_local_variable(ast::StatementBlock& node, const std::string& varname, int dim);


/// Create ast statement node from given code in string format
std::shared_ptr<ast::Statement> create_statement(const std::string& code_statement);

/// Same as for create_statement but for vectors of strings
std::vector<std::shared_ptr<ast::Statement>> create_statements(
    const std::vector<std::string>::const_iterator& code_statements_beg,
    const std::vector<std::string>::const_iterator& code_statements_end);


/// Create ast statement block node from given code in string format
std::shared_ptr<ast::StatementBlock> create_statement_block(
    const std::vector<std::string>& code_statements);


///  Remove statements from given statement block if they exist
void remove_statements_from_block(ast::StatementBlock& block,
                                  const std::set<ast::Node*>& statements);


/// Return set of strings with the names of all global variables
std::set<std::string> get_global_vars(const ast::Program& node);


/// Checks whether block contains a call to a particular function
bool calls_function(const ast::Ast& node, const std::string& name);

}  // namespace visitor

/// traverse \a node recursively and collect nodes of given \a types
std::vector<std::shared_ptr<const ast::Ast>> collect_nodes(
    const ast::Ast& node,
    const std::vector<ast::AstNodeType>& types = {});

/// traverse \a node recursively and collect nodes of given \a types
std::vector<std::shared_ptr<ast::Ast>> collect_nodes(
    ast::Ast& node,
    const std::vector<ast::AstNodeType>& types = {});

bool sparse_solver_exists(const ast::Ast& node);

/// Given AST node, return the NMODL string representation
std::string to_nmodl(const ast::Ast& node, const std::set<ast::AstNodeType>& exclude_types = {});

/// Given a shared pointer to an AST node, return the NMODL string representation
template <typename T>
typename std::enable_if<std::is_base_of<ast::Ast, T>::value, std::string>::type to_nmodl(
    const std::shared_ptr<T>& node,
    const std::set<ast::AstNodeType>& exclude_types = {}) {
    return to_nmodl(*node, exclude_types);
}

/// Given AST node, return the JSON string representation
std::string to_json(const ast::Ast& node,
                    bool compact = false,
                    bool expand = false,
                    bool add_nmodl = false);

/// If \p lhs and \p rhs combined represent an assignment (we assume to have an "=" in between them)
/// we extract the variables on which the assigned variable depends on. We provide the input with
/// lhs and rhs because there are a few nodes that have this similar structure but slightly
/// different naming (binaryExpression, diff_eq_expression, linExpression)
std::pair<std::string, std::unordered_set<std::string>> statement_dependencies(
    const std::shared_ptr<ast::Expression>& lhs,
    const std::shared_ptr<ast::Expression>& rhs);

/// Given a Indexed node, return the name with index
std::string get_indexed_name(const ast::IndexedName& node);

/// Given a VarName node, return the full var name including index
std::string get_full_var_name(const ast::VarName& node);

}  // namespace nmodl
