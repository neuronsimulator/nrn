/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <iostream>
#include <memory>
#include <unordered_set>

#include "ast/global.hpp"
#include "ast/neuron_block.hpp"
#include "ast/program.hpp"
#include "ast/range.hpp"
#include "ast/statement_block.hpp"
#include "visitors/global_var_visitor.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {

void GlobalToRangeVisitor::visit_neuron_block(ast::NeuronBlock& node) {
    ast::RangeVarVector range_variables;
    std::unordered_set<ast::GlobalVar*> global_variables_to_remove;
    std::unordered_set<ast::Statement*> global_statements_to_remove;

    auto& statement_block = node.get_statement_block();
    auto& statements = (*statement_block).get_statements();
    const auto& symbol_table = ast.get_symbol_table();

    for (auto& statement: statements) {
        /// only process global statements
        if (statement->is_global()) {
            const auto& global_variables =
                std::static_pointer_cast<ast::Global>(statement)->get_variables();
            global_variables_to_remove.clear();
            for (auto& global_variable: global_variables) {
                auto variable_name = global_variable->get_node_name();
                /// check if global variable is being updated in the mod file
                if (symbol_table->lookup(variable_name)->get_write_count() > 0) {
                    range_variables.emplace_back(new ast::RangeVar(global_variable->get_name()));
                    global_variables_to_remove.emplace(global_variable.get());
                }
            }

            /// remove offending global variables
            std::static_pointer_cast<ast::Global>(statement)->erase_global_var(
                global_variables_to_remove);

            /// add empty global statements to global_statements_to_remove
            if (global_variables.empty()) {
                global_statements_to_remove.emplace(statement.get());
            }
        }
    }

    /// remove offending global statements if empty
    statement_block->erase_statement(global_statements_to_remove);

    /// insert new range variables replacing global ones
    if (!range_variables.empty()) {
        auto range_statement = new ast::Range(range_variables);
        statement_block->emplace_back_statement(range_statement);
    }
}

}  // namespace visitor
}  // namespace nmodl
