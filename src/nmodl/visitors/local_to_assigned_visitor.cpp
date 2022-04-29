/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <iostream>
#include <memory>
#include <unordered_set>

#include "ast/assigned_block.hpp"
#include "ast/local_list_statement.hpp"
#include "ast/program.hpp"
#include "visitors/local_to_assigned_visitor.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {

void LocalToAssignedVisitor::visit_program(ast::Program& node) {
    ast::AssignedDefinitionVector assigned_variables;
    std::unordered_set<ast::LocalVar*> local_variables_to_remove;
    std::unordered_set<ast::Node*> local_nodes_to_remove;
    std::shared_ptr<ast::AssignedBlock> assigned_block;

    const auto& top_level_nodes = node.get_blocks();
    const auto& symbol_table = node.get_symbol_table();

    for (auto& top_level_node: top_level_nodes) {
        /// save pointer to assigned block to add new variables (only one block)
        if (top_level_node->is_assigned_block()) {
            assigned_block = std::static_pointer_cast<ast::AssignedBlock>(top_level_node);
        }

        /// only process local_list statements otherwise continue
        if (!top_level_node->is_local_list_statement()) {
            continue;
        }

        const auto& local_list_statement = std::static_pointer_cast<ast::LocalListStatement>(
            top_level_node);

        for (const auto& local_variable: local_list_statement->get_variables()) {
            const auto& variable_name = local_variable->get_node_name();
            /// check if local variable is being updated in the mod file
            if (symbol_table->lookup(variable_name)->get_write_count() > 0) {
                assigned_variables.emplace_back(
                    std::make_shared<ast::AssignedDefinition>(local_variable->get_name(),
                                                              nullptr,
                                                              nullptr,
                                                              nullptr,
                                                              nullptr,
                                                              nullptr,
                                                              nullptr));
                local_variables_to_remove.insert(local_variable.get());
            }
        }

        /// remove local variables being converted to assigned
        local_list_statement->erase_local_var(local_variables_to_remove);

        /// if all local variables are converted to assigned, keep track
        /// for removal
        if (local_list_statement->get_variables().empty()) {
            local_nodes_to_remove.insert(top_level_node.get());
        }
    }

    /// remove empty local statements if empty
    node.erase_node(local_nodes_to_remove);

    /// if no assigned block found add one to the node otherwise emplace back new assigned variables
    if (assigned_block == nullptr) {
        assigned_block = std::make_shared<ast::AssignedBlock>(assigned_variables);
        node.emplace_back_node(std::static_pointer_cast<ast::Node>(assigned_block));
    } else {
        for (const auto& assigned_variable: assigned_variables) {
            assigned_block->emplace_back_assigned_definition(assigned_variable);
        }
    }
}

}  // namespace visitor
}  // namespace nmodl
