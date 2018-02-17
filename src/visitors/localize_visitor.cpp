#include <algorithm>

#include "visitors/localize_visitor.hpp"
#include "visitors/defuse_analyze_visitor.hpp"

using namespace ast;
using namespace symtab;

bool LocalizeVisitor::node_for_def_use_analysis(ast::Node* node) {
    auto type = node->get_type();

    /**
     * Blocks where we should compute def-use chains. We are excluding
     * procedures and functions because we expect those to be "inlined".
     * If procedure/function is not inlined then DefUse pass returns
     * result as "Use". So it's safe.
     */
    // clang-format off
    const std::vector<ast::Type> blocks_to_analyze = {
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
        auto symbol = program_symtab->lookup(node->get_name());
        if (symbol && symbol->has_properties(NmodlInfo::to_solve)) {
            return true;
        }
    }
    return false;
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
            if (node_for_def_use_analysis(block.get())) {
                DefUseAnalyzeVisitor v(program_symtab, ignore_verbatim);
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
