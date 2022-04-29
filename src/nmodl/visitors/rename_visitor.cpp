/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/rename_visitor.hpp"

#include "ast/all.hpp"
#include "parser/c11_driver.hpp"
#include "utils/logger.hpp"
#include "visitors/visitor_utils.hpp"


namespace nmodl {
namespace visitor {


std::string RenameVisitor::new_name_generator(const std::string& old_name) {
    std::string new_name;
    if (add_random_suffix) {
        if (renamed_variables.find(old_name) != renamed_variables.end()) {
            new_name = renamed_variables[old_name];
        } else {
            const auto& vars = get_global_vars(*ast);
            if (add_prefix) {
                new_name = suffix_random_string(vars,
                                                new_var_name_prefix + old_name,
                                                UseNumbersInString::WithoutNumbers);
            } else {
                new_name =
                    suffix_random_string(vars, new_var_name, UseNumbersInString::WithoutNumbers);
            }
            renamed_variables[old_name] = new_name;
        }
    } else {
        if (add_prefix) {
            new_name = new_var_name_prefix + old_name;
        } else {
            new_name = new_var_name;
        }
    }
    return new_name;
}

/// rename matching variable
void RenameVisitor::visit_name(const ast::Name& node) {
    const auto& name = node.get_node_name();
    if (std::regex_match(name, var_name_regex)) {
        std::string new_name = new_name_generator(name);
        node.get_value()->set(new_name);
        std::string token_string = node.get_token() != nullptr
                                       ? " at " + node.get_token()->position()
                                       : "";
        logger->warn("RenameVisitor :: Renaming variable {}{} to {}", name, token_string, new_name);
    }
}

/** Prime name has member order which is an integer. In theory
 * integer could be "macro name" and hence could end-up renaming
 * macro. In practice this won't be an issue as we order is set
 * by parser. To be safe we are only renaming prime variable.
 */
void RenameVisitor::visit_prime_name(const ast::PrimeName& node) {
    node.visit_children(*this);
}

/**
 * Parse verbatim blocks and rename variable if it is used.
 */
void RenameVisitor::visit_verbatim(const ast::Verbatim& node) {
    if (!rename_verbatim) {
        return;
    }

    const auto& statement = node.get_statement();
    const auto& text = statement->eval();
    parser::CDriver driver;

    driver.scan_string(text);
    auto tokens = driver.all_tokens();

    std::ostringstream result;
    for (auto& token: tokens) {
        if (std::regex_match(token, var_name_regex)) {
            /// Check if variable is already renamed and use the same naming otherwise add the
            /// new_name to the renamed_variables map
            const std::string& new_name = new_name_generator(token);
            result << new_name;
            logger->warn("RenameVisitor :: Renaming variable {} in VERBATIM block to {}",
                         token,
                         new_name);
        } else {
            result << token;
        }
    }
    statement->set(result.str());
}

}  // namespace visitor
}  // namespace nmodl
