/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <algorithm>

#include "visitors/defuse_analyze_visitor.hpp"
#include "visitors/localize_visitor.hpp"

namespace nmodl {

using symtab::Symbol;
using symtab::syminfo::NmodlType;

bool LocalizeVisitor::node_for_def_use_analysis(ast::Node* node) {
    auto type = node->get_node_type();

    /**
     * Blocks where we should compute def-use chains. We are excluding
     * procedures and functions because we expect those to be "inlined".
     * If procedure/function is not inlined then DefUse pass returns
     * result as "Use". So it's safe.
     */
    // clang-format off
    const std::vector<ast::AstNodeType> blocks_to_analyze = {
            ast::AstNodeType::INITIAL_BLOCK,
            ast::AstNodeType::BREAKPOINT_BLOCK,
            ast::AstNodeType::CONSTRUCTOR_BLOCK,
            ast::AstNodeType::DESTRUCTOR_BLOCK,
            ast::AstNodeType::DERIVATIVE_BLOCK,
            ast::AstNodeType::LINEAR_BLOCK,
            ast::AstNodeType::NON_LINEAR_BLOCK,
            ast::AstNodeType::DISCRETE_BLOCK,
            ast::AstNodeType::PARTIAL_BLOCK,
            ast::AstNodeType::NET_RECEIVE_BLOCK,
            ast::AstNodeType::TERMINAL_BLOCK,
            ast::AstNodeType::BA_BLOCK,
            ast::AstNodeType::FOR_NETCON,
            ast::AstNodeType::BEFORE_BLOCK,
            ast::AstNodeType::AFTER_BLOCK,
    };
    // clang-format on
    auto it = std::find(blocks_to_analyze.begin(), blocks_to_analyze.end(), type);
    auto node_to_analyze = !(it == blocks_to_analyze.end());
    auto solve_procedure = is_solve_procedure(node);
    return (node_to_analyze || solve_procedure);
}

/*
 * Check if given node is a procedure block and if it is used
 * in the solve statement.
 */
bool LocalizeVisitor::is_solve_procedure(ast::Node* node) {
    if (node->is_procedure_block()) {
        auto symbol = program_symtab->lookup(node->get_node_name());
        if (symbol && symbol->has_properties(NmodlType::to_solve)) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> LocalizeVisitor::variables_to_optimize() {
    // clang-format off
    const NmodlType excluded_var_properties = NmodlType::extern_var
                                     | NmodlType::extern_neuron_variable
                                     | NmodlType::read_ion_var
                                     | NmodlType::write_ion_var
                                     | NmodlType::prime_name
                                     | NmodlType::nonspecific_cur_var
                                     | NmodlType::pointer_var
                                     | NmodlType::bbcore_pointer_var
                                     | NmodlType::electrode_cur_var
                                     | NmodlType::section_var;

    NmodlType global_var_properties = NmodlType::range_var
                                       | NmodlType::dependent_def
                                       | NmodlType::param_assign;
    // clang-format on


    /**
     *  \todo Voltage v can be global variable as well as external. In order
     *        to avoid optimizations, we need to handle this case properly
     *  \todo Instead of ast node, use symbol properties to check variable type
     */
    auto variables = program_symtab->get_variables_with_properties(global_var_properties);

    std::vector<std::string> result;
    for (auto& variable: variables) {
        if (!variable->has_properties(excluded_var_properties)) {
            result.push_back(variable->get_name());
        }
    }
    return result;
}

void LocalizeVisitor::visit_program(ast::Program* node) {
    /// symtab visitor pass need to be run before
    program_symtab = node->get_symbol_table();
    if (program_symtab == nullptr) {
        logger->warn("LocalizeVisitor :: symbol table is not setup, returning");
        return;
    }

    auto variables = variables_to_optimize();
    for (const auto& varname: variables) {
        const auto& blocks = node->get_blocks();
        std::map<DUState, std::vector<std::shared_ptr<ast::Node>>> block_usage;

        /// compute def use chains
        for (auto block: blocks) {
            if (node_for_def_use_analysis(block.get())) {
                DefUseAnalyzeVisitor v(program_symtab, ignore_verbatim);
                auto usages = v.analyze(block.get(), varname);
                auto result = usages.eval();
                block_usage[result].push_back(block);
            }
        }

        /// as we are doing global analysis, if any global block is "using"
        /// variable then we can't localize the variable
        auto it = block_usage.find(DUState::U);
        if (it == block_usage.end()) {
            logger->debug("LocalizeVisitor : localized variable {}", varname);

            /// all blocks that are have either definition or conditional definition
            /// need local variable
            for (auto state: {DUState::D, DUState::CD}) {
                for (auto& block: block_usage[state]) {
                    auto block_ptr = dynamic_cast<ast::Block*>(block.get());
                    auto statement_block = block_ptr->get_statement_block();
                    ast::LocalVar* variable;
                    auto symbol = program_symtab->lookup(varname);

                    if (symbol->is_array()) {
                        variable = add_local_variable(statement_block.get(), varname,
                                                      symbol->get_length());
                    } else {
                        variable = add_local_variable(statement_block.get(), varname);
                    }

                    /// mark variable as localized in global symbol table
                    symbol->mark_localized();

                    /// insert new symbol in the symbol table of current block
                    auto symtab = statement_block->get_symbol_table();
                    auto new_symbol = std::make_shared<Symbol>(varname, variable);
                    new_symbol->add_property(NmodlType::local_var);
                    new_symbol->mark_created();
                    symtab->insert(new_symbol);
                }
            }
        }
    }
}

}  // namespace nmodl
