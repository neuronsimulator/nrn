/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/rename_visitor.hpp"

#include "ast/ast.hpp"
#include "parser/c11_driver.hpp"


namespace nmodl {
namespace visitor {

/// rename matching variable
void RenameVisitor::visit_name(ast::Name& node) {
    const auto& name = node.get_node_name();
    if (name == var_name) {
        auto& value = node.get_value();
        value->set(new_var_name);
    }
}

/** Prime name has member order which is an integer. In theory
 * integer could be "macro name" and hence could end-up renaming
 * macro. In practice this won't be an issue as we order is set
 * by parser. To be safe we are only renaming prime variable.
 */
void RenameVisitor::visit_prime_name(ast::PrimeName& node) {
    node.visit_children(*this);
}

/**
 * Parse verbatim blocks and rename variable if it is used.
 */
void RenameVisitor::visit_verbatim(ast::Verbatim& node) {
    if (!rename_verbatim) {
        return;
    }

    const auto& statement = node.get_statement();
    auto text = statement->eval();
    parser::CDriver driver;

    driver.scan_string(text);
    auto tokens = driver.all_tokens();

    std::string result;
    for (auto& token: tokens) {
        if (token == var_name) {
            result += new_var_name;
        } else {
            result += token;
        }
    }
    statement->set(result);
}

}  // namespace visitor
}  // namespace nmodl
