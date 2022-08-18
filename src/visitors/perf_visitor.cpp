/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/perf_visitor.hpp"

#include <utility>

#include "ast/all.hpp"
#include "printer/json_printer.hpp"


namespace nmodl {
namespace visitor {

using printer::JSONPrinter;
using symtab::Symbol;
using symtab::syminfo::NmodlType;
using symtab::syminfo::Status;
using utils::PerfStat;

PerfVisitor::PerfVisitor(const std::string& filename)
    : printer(new JSONPrinter(filename)) {}

void PerfVisitor::compact_json(bool flag) {
    printer->compact_json(flag);
}


/// count math operations from all binary expressions
void PerfVisitor::visit_binary_expression(const ast::BinaryExpression& node) {
    bool assign_op = false;

    if (start_measurement) {
        auto value = node.get_op().get_value();
        switch (value) {
        case ast::BOP_ADDITION:
            current_block_perf.n_add++;
            break;

        case ast::BOP_SUBTRACTION:
            current_block_perf.n_sub++;
            break;

        case ast::BOP_MULTIPLICATION:
            current_block_perf.n_mul++;
            break;

        case ast::BOP_DIVISION:
            current_block_perf.n_div++;
            break;

        case ast::BOP_POWER:
            current_block_perf.n_pow++;
            break;

        case ast::BOP_AND:
            current_block_perf.n_and++;
            break;

        case ast::BOP_OR:
            current_block_perf.n_or++;
            break;

        case ast::BOP_GREATER:
            current_block_perf.n_gt++;
            break;

        case ast::BOP_GREATER_EQUAL:
            current_block_perf.n_ge++;
            break;

        case ast::BOP_LESS:
            current_block_perf.n_lt++;
            break;

        case ast::BOP_LESS_EQUAL:
            current_block_perf.n_le++;
            break;

        case ast::BOP_ASSIGN:
            current_block_perf.n_assign++;
            assign_op = true;
            break;

        case ast::BOP_NOT_EQUAL:
            current_block_perf.n_ne++;
            break;

        case ast::BOP_EXACT_EQUAL:
            current_block_perf.n_ee++;
            break;

        default:
            throw std::logic_error("Binary operator not handled in perf visitor");
        }
    }

    /// if visiting assignment expression, symbols from lhs
    /// are written and hence need flag to track
    if (assign_op) {
        visiting_lhs_expression = true;
    }

    node.get_lhs()->accept(*this);

    /// lhs is done (rhs is read only)
    visiting_lhs_expression = false;

    node.get_rhs()->accept(*this);
}

/// add performance stats to json printer
void PerfVisitor::add_perf_to_printer(const PerfStat& perf) const {
    const auto& keys = nmodl::utils::PerfStat::keys();
    const auto& values = perf.values();
    assert(keys.size() == values.size());

    for (size_t i = 0; i < keys.size(); i++) {
        printer->add_node(values[i], keys[i]);
    }
}

/** Helper function used by all ast nodes : visit all children
 *  recursively and performance stats get added on stack. Once
 *  all children visited, we get total performance by summing
 *  perfstat of all children.
 */
void PerfVisitor::measure_performance(const ast::Ast& node) {
    start_measurement = true;

    node.visit_children(*this);

    PerfStat perf;
    while (!children_blocks_perf.empty()) {
        perf = perf + children_blocks_perf.top();
        children_blocks_perf.pop();
    }

    auto symtab = node.get_symbol_table();
    if (symtab == nullptr) {
        throw std::runtime_error("Perfvisitor : symbol table not setup for " +
                                 node.get_node_type_name());
    }

    auto name = symtab->name();
    if (node.is_derivative_block()) {
        name = node.get_node_type_name();
    }

    if (printer) {
        printer->push_block(name);
    }

    perf.title = "Performance Statistics of " + name;
    perf.print(stream);

    if (printer) {
        add_perf_to_printer(perf);
        printer->pop_block();
    }

    start_measurement = false;

    /// clear var usage map
    for (auto& var_set: var_usage) {
        var_set.second.clear();
    }
}

/// count function calls and "most useful" or "commonly used" math functions
void PerfVisitor::visit_function_call(const ast::FunctionCall& node) {
    under_function_call = true;

    if (start_measurement) {
        auto name = node.get_node_name();
        if (name == "exp") {
            current_block_perf.n_exp++;
        } else if (name == "log") {
            current_block_perf.n_log++;
        } else if (name == "pow") {
            current_block_perf.n_pow++;
        }
        node.visit_children(*this);

        auto symbol = current_symtab->lookup_in_scope(name);
        auto method_property = NmodlType::procedure_block | NmodlType::function_block;
        if (symbol != nullptr && symbol->has_any_property(method_property)) {
            current_block_perf.n_int_func_call++;
        } else {
            current_block_perf.n_ext_func_call++;
        }
    }

    under_function_call = false;
}

/// every variable used is of type name, update counters
void PerfVisitor::visit_name(const ast::Name& node) {
    update_memory_ops(node.get_node_name());
    node.visit_children(*this);
}

/// prime name derived from identifier and hence need to be handled here
void PerfVisitor::visit_prime_name(const ast::PrimeName& node) {
    update_memory_ops(node.get_node_name());
    node.visit_children(*this);
}

void PerfVisitor::visit_if_statement(const ast::IfStatement& node) {
    if (start_measurement) {
        current_block_perf.n_if++;
        node.visit_children(*this);
    }
}

void PerfVisitor::visit_else_if_statement(const ast::ElseIfStatement& node) {
    if (start_measurement) {
        current_block_perf.n_elif++;
        node.visit_children(*this);
    }
}

void PerfVisitor::count_variables() {
    /// number of instance variables: range or assigned variables
    /// one caveat is that the global variables appearing in
    /// assigned block are not treated as range
    num_instance_variables = 0;

    NmodlType property = NmodlType::range_var | NmodlType::assigned_definition |
                         NmodlType::state_var;
    auto variables = current_symtab->get_variables_with_properties(property);

    for (auto& variable: variables) {
        if (!variable->has_any_property(NmodlType::global_var)) {
            num_instance_variables++;
            if (variable->has_any_property(NmodlType::param_assign)) {
                num_constant_instance_variables++;
            }
            if (variable->has_any_status(Status::localized)) {
                num_localized_instance_variables++;
            }
        }
    }

    /// state variables have state_var property
    property = NmodlType::state_var;
    variables = current_symtab->get_variables_with_properties(property);
    num_state_variables = static_cast<int>(variables.size());

    /// pointer variables have pointer/bbcorepointer
    property = NmodlType::pointer_var | NmodlType::bbcore_pointer_var;
    variables = current_symtab->get_variables_with_properties(property);
    num_pointer_variables = static_cast<int>(variables.size());


    /// number of global variables : parameters and pointers could appear also
    /// as range variables and hence need to filter out. But if anything declared
    /// as global is always global.
    property = NmodlType::global_var | NmodlType::param_assign | NmodlType::bbcore_pointer_var |
               NmodlType::pointer_var;
    variables = current_symtab->get_variables_with_properties(property);
    num_global_variables = 0;
    for (auto& variable: variables) {
        auto is_global = variable->has_any_property(NmodlType::global_var);
        property = NmodlType::range_var | NmodlType::assigned_definition;
        if (!variable->has_any_property(property) || is_global) {
            num_global_variables++;
            if (variable->has_any_property(NmodlType::param_assign)) {
                num_constant_global_variables++;
            }
            if (variable->has_any_status(Status::localized)) {
                num_localized_global_variables++;
            }
        }
    }
}

void PerfVisitor::print_memory_usage() {
    stream << std::endl;

    stream << "#VARIABLES :: ";
    stream << "  INSTANCE : " << num_instance_variables << " ";
    stream << "[ CONSTANT " << num_constant_instance_variables << ", ";
    stream << "LOCALIZED " << num_localized_instance_variables << " ]";

    stream << "  GLOBAL VARIABLES : " << num_global_variables << " ";
    stream << "[ CONSTANT " << num_constant_global_variables << ", ";
    stream << "LOCALIZED " << num_localized_global_variables << " ]";

    stream << "  STATE : " << num_state_variables;
    stream << "  POINTER : " << num_pointer_variables << std::endl;

    if (printer) {
        printer->push_block("MemoryInfo");

        printer->push_block("Instance");
        printer->add_node(std::to_string(num_instance_variables), "total");
        printer->add_node(std::to_string(num_constant_instance_variables), "const");
        printer->add_node(std::to_string(num_localized_instance_variables), "localized");
        printer->pop_block();

        printer->push_block("Global");
        printer->add_node(std::to_string(num_global_variables), "total");
        printer->add_node(std::to_string(num_global_variables), "const");
        printer->add_node(std::to_string(num_localized_global_variables), "localized");
        printer->pop_block();

        printer->push_block("State");
        printer->add_node(std::to_string(num_state_variables), "total");
        printer->pop_block();

        printer->push_block("Pointer");
        printer->add_node(std::to_string(num_pointer_variables), "total");
        printer->pop_block();

        printer->pop_block();
    }
}

void PerfVisitor::visit_program(const ast::Program& node) {
    if (printer) {
        printer->push_block("BlockPerf");
    }

    node.visit_children(*this);
    std::string title = "Total Performance Statistics";
    total_perf.title = title;
    total_perf.print(stream);

    if (printer) {
        printer->push_block("total");
        add_perf_to_printer(total_perf);
        printer->pop_block();
        printer->pop_block();
    }

    current_symtab = node.get_symbol_table();
    count_variables();
    print_memory_usage();
}

void PerfVisitor::visit_plot_block(const ast::PlotBlock& node) {
    measure_performance(node);
}

/// skip initial block under net_receive block
void PerfVisitor::visit_initial_block(const ast::InitialBlock& node) {
    if (!under_net_receive_block) {
        measure_performance(node);
    }
}

void PerfVisitor::visit_constructor_block(const ast::ConstructorBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_destructor_block(const ast::DestructorBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_derivative_block(const ast::DerivativeBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_linear_block(const ast::LinearBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_non_linear_block(const ast::NonLinearBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_discrete_block(const ast::DiscreteBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_partial_block(const ast::PartialBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_function_table_block(const ast::FunctionTableBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_function_block(const ast::FunctionBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_procedure_block(const ast::ProcedureBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_net_receive_block(const ast::NetReceiveBlock& node) {
    under_net_receive_block = true;
    measure_performance(node);
    under_net_receive_block = false;
}

void PerfVisitor::visit_breakpoint_block(const ast::BreakpointBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_terminal_block(const ast::TerminalBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_before_block(const ast::BeforeBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_after_block(const ast::AfterBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_ba_block(const ast::BABlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_for_netcon(const ast::ForNetcon& node) {
    measure_performance(node);
}

void PerfVisitor::visit_kinetic_block(const ast::KineticBlock& node) {
    measure_performance(node);
}

void PerfVisitor::visit_match_block(const ast::MatchBlock& node) {
    measure_performance(node);
}

/** Blocks like function can have multiple statement blocks and
 * blocks like net receive has nested initial blocks. Hence need
 * to maintain separate stack.
 */
void PerfVisitor::visit_statement_block(const ast::StatementBlock& node) {
    /// starting new block, store current state
    blocks_perf.push(current_block_perf);

    current_symtab = node.get_symbol_table();

    if (current_symtab == nullptr) {
        throw std::runtime_error("Perfvisitor : symbol table not setup for " +
                                 node.get_node_type_name());
    }

    /// new block perf starts from zero
    current_block_perf = PerfStat();

    node.visit_children(*this);

    /// add performance of all visited children
    total_perf = total_perf + current_block_perf;

    children_blocks_perf.push(current_block_perf);

    // go back to parent block's state
    current_block_perf = blocks_perf.top();
    blocks_perf.pop();
}

/// solve is not a statement but could have associated block
/// and hence could/should not be skipped completely
/// we can't ignore the block because it could have associated
/// statement block (in theory)
void PerfVisitor::visit_solve_block(const ast::SolveBlock& node) {
    under_solve_block = true;
    node.visit_children(*this);
    under_solve_block = false;
}

void PerfVisitor::visit_unary_expression(const ast::UnaryExpression& node) {
    if (start_measurement) {
        auto value = node.get_op().get_value();
        switch (value) {
        case ast::UOP_NEGATION:
            current_block_perf.n_neg++;
            break;

        case ast::UOP_NOT:
            current_block_perf.n_not++;
            break;

        default:
            throw std::logic_error("Unary operator not handled in perf visitor");
        }
    }
    node.visit_children(*this);
}

/** Certain statements / symbols needs extra check while measuring
 * read/write operations. For example, for expression "exp(a+b)",
 * "exp" is an external math function and we should not increment read
 * count for "exp" symbol. Same for solve statement where name will
 * be derivative block name and neuron solver method.
 */
bool PerfVisitor::symbol_to_skip(const std::shared_ptr<Symbol>& symbol) const {
    bool skip = false;

    auto is_method = symbol->has_any_property(NmodlType::extern_method | NmodlType::function_block);
    if (is_method && under_function_call) {
        skip = true;
    }

    is_method = symbol->has_any_property(NmodlType::derivative_block | NmodlType::extern_method);
    if (is_method && under_solve_block) {
        skip = true;
    }

    return skip;
}

bool PerfVisitor::is_local_variable(const std::shared_ptr<Symbol>& symbol) {
    bool is_local = false;
    /// in the function when we write to function variable then consider it as local variable
    auto properties = NmodlType::local_var | NmodlType::argument | NmodlType::function_block;
    if (symbol->has_any_property(properties)) {
        is_local = true;
    }
    return is_local;
}

bool PerfVisitor::is_constant_variable(const std::shared_ptr<Symbol>& symbol) {
    bool is_constant = false;
    auto properties = NmodlType::param_assign;
    if (symbol->has_any_property(properties)) {
        is_constant = true;
    }
    return is_constant;
}


/** Find symbol in closest scope (up to parent) and update
 * read/write count. Also update ops count in current block.
 */
void PerfVisitor::update_memory_ops(const std::string& name) {
    if (!start_measurement || current_symtab == nullptr) {
        return;
    }

    auto symbol = current_symtab->lookup_in_scope(name);
    if (symbol == nullptr || symbol_to_skip(symbol)) {
        return;
    }

    if (is_local_variable(symbol)) {
        if (visiting_lhs_expression) {
            symbol->write();
            current_block_perf.n_local_write++;
        } else {
            symbol->read();
            current_block_perf.n_local_read++;
        }
        return;
    }

    /// lhs symbols get written
    if (visiting_lhs_expression) {
        symbol->write();
        if (is_constant_variable(symbol)) {
            current_block_perf.n_constant_write++;
            if (var_usage[const_memw_key].find(name) == var_usage[const_memw_key].end()) {
                current_block_perf.n_unique_constant_write++;
                var_usage[const_memw_key].insert(name);
            }
        } else {
            current_block_perf.n_global_write++;
            if (var_usage[global_memw_key].find(name) == var_usage[global_memw_key].end()) {
                current_block_perf.n_unique_global_write++;
                var_usage[global_memw_key].insert(name);
            }
        }
        return;
    }

    /// rhs symbols get read
    symbol->read();
    if (is_constant_variable(symbol)) {
        current_block_perf.n_constant_read++;
        if (var_usage[const_memr_key].find(name) == var_usage[const_memr_key].end()) {
            current_block_perf.n_unique_constant_read++;
            var_usage[const_memr_key].insert(name);
        }
    } else {
        current_block_perf.n_global_read++;
        if (var_usage[global_memr_key].find(name) == var_usage[global_memr_key].end()) {
            current_block_perf.n_unique_global_read++;
            var_usage[global_memr_key].insert(name);
        }
    }
}

}  // namespace visitor
}  // namespace nmodl
