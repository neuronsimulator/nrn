#include <algorithm>

#include "visitors/localize_visitor.hpp"
#include "visitors/defuse_analyze_visitor.hpp"

using namespace ast;

bool LocalizeVisitor::node_to_localize(ast::Node* node) {
    auto type = node->get_type();
    // clang-format off
    const std::vector<ast::Type> blocks_to_localize = {
            ast::Type::INITIAL_BLOCK,
            ast::Type::BREAKPOINT_BLOCK,
            ast::Type::CONSTRUCTOR_BLOCK,
            ast::Type::DESTRUCTOR_BLOCK,
            ast::Type::DERIVATIVE_BLOCK,
            ast::Type::LINEAR_BLOCK,
            ast::Type::NON_LINEAR_BLOCK,
            ast::Type::DISCRETE_BLOCK,
            ast::Type::PARTIAL_BLOCK,
            ast::Type::NET_RECEIVE_BLOCK,
            ast::Type::TERMINAL_BLOCK,
            ast::Type::BA_BLOCK,
            ast::Type::FOR_NETCON,
            ast::Type::BEFORE_BLOCK,
            ast::Type::AFTER_BLOCK,
    };
    // clang-format on
    auto it = std::find(blocks_to_localize.begin(), blocks_to_localize.end(), type);
    return !(it == blocks_to_localize.end());
}

std::vector<std::string> LocalizeVisitor::variables_to_optimize() {
    // clang-format off
    using NmodlInfo = symtab::details::NmodlInfo;
    const SymbolInfo excluded_var_properties = NmodlInfo::extern_var
                                     | NmodlInfo::extern_neuron_variable
                                     | NmodlInfo::read_ion_var
                                     | NmodlInfo::write_ion_var
                                     | NmodlInfo::prime_name
                                     | NmodlInfo::nonspe_cur_var
                                     | NmodlInfo::pointer_var
                                     | NmodlInfo::bbcore_pointer_var
                                     | NmodlInfo::electrode_cur_var
                                     | NmodlInfo::section_var;

    SymbolInfo global_var_properties = NmodlInfo::range_var
                                       | NmodlInfo::dependent_def
                                       | NmodlInfo::param_assign;
    // clang-format on


    /**
     *  \todo Voltage v can be global variable as well as external. In order
     *        to avoid optimizations, we need to handle this case properly
     *  \todo Instead of ast node, use symbol properties to check variable type
     */
    auto variables = program_symtab->get_variables_with_properties(global_var_properties);

    std::vector<std::string> result;
    for (auto& variable : variables) {
        if (!variable->has_properties(excluded_var_properties)) {
            result.push_back(variable->get_name());
        }
    }
    return result;
}

void LocalizeVisitor::visit_program(Program* node) {
    /// symtab visitor pass must be run before
    program_symtab = node->get_symbol_table();
    if (program_symtab == nullptr) {
        throw std::runtime_error("localizer error : program node doesn't have symbol table");
    }

    auto variables = variables_to_optimize();
    for (const auto& variable : variables) {
        auto blocks = node->blocks;
        std::map<DUState, std::vector<std::shared_ptr<ast::Node>>> block_usage;

        /// compute def use chains
        for (auto& block : blocks) {
            if (node_to_localize(block.get())) {
                DefUseAnalyzeVisitor v(program_symtab);
                auto usages = v.analyze(block.get(), variable);
                auto result = usages.eval();
                block_usage[result].push_back(block);
            }
        }

        /// as we are doing global analysis, if any global block is "using"
        /// variable then we can't localize the variable
        auto it = block_usage.find(DUState::U);
        if (it == block_usage.end()) {
            /// all blocks that are using variable should get local variable
            for (auto& block : block_usage[DUState::D]) {
                auto block_ptr = dynamic_cast<Block*>(block.get());
                auto statement_block = block_ptr->get_statement_block();
                add_local_variable(statement_block.get(), variable);
                auto symbol = program_symtab->lookup(variable);
                symbol->localized();
            }
        }
    }
}
