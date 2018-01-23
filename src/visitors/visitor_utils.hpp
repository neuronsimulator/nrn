#ifndef NMODL_VISITOR_UTILS
#define NMODL_VISITOR_UTILS

#include <string>
#include <map>

#include "ast/ast.hpp"

/** Return new name variable by appending "_suffix_COUNT" where COUNT is number
 * of times the given variable is already used.
 */
std::string get_new_name(const std::string& name,
                         const std::string& suffix,
                         std::map<std::string, int>& variables);
ast::LocalVarVector* get_local_variables(const ast::StatementBlock* node);

#endif