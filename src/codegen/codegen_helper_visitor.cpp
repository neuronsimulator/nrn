/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "codegen/codegen_helper_visitor.hpp"

#include <algorithm>
#include <cmath>

#include "ast/all.hpp"
#include "codegen/codegen_naming.hpp"
#include "visitors/visitor_utils.hpp"


namespace nmodl {
namespace codegen {

using namespace ast;

using symtab::syminfo::NmodlType;
using symtab::syminfo::Status;

/**
 * How symbols are stored in NEURON? See notes written in markdown file.
 *
 * Some variables get printed by iterating over symbol table in mod2c.
 * The example of this is thread variables (and also ions?). In this
 * case we must have to arrange order if we are going keep compatibility
 * with NEURON.
 *
 * Suppose there are three global variables: bcd, abc, abd, abe
 * They will be in the 'a' bucket in order:
 *      abe, abd, abc
 * and in 'b' bucket
 *      bcd
 * So when we print thread variables, we first have to sort in the opposite
 * order in which they come and then again order by first character in increasing
 * order.
 *
 * Note that variables in double array do not need this transformation
 * and it seems like they should just follow definition order.
 */
void CodegenHelperVisitor::sort_with_mod2c_symbol_order(std::vector<SymbolType>& symbols) const {
    /// first sort by global id to get in reverse order
    std::sort(symbols.begin(),
              symbols.end(),
              [](const SymbolType& first, const SymbolType& second) -> bool {
                  return first->get_id() > second->get_id();
              });

    /// now order by name (to be same as neuron's bucket)
    std::sort(symbols.begin(),
              symbols.end(),
              [](const SymbolType& first, const SymbolType& second) -> bool {
                  return first->get_name()[0] < second->get_name()[0];
              });
}


/**
 * Find all ions used in mod file
 */
void CodegenHelperVisitor::find_ion_variables(const ast::Program& node) {
    // collect all use ion statements
    const auto& ion_nodes = collect_nodes(node, {AstNodeType::USEION});

    // ion names, read ion variables and write ion variables
    std::vector<std::string> ion_vars;
    std::vector<std::string> read_ion_vars;
    std::vector<std::string> write_ion_vars;

    for (const auto& ion_node: ion_nodes) {
        const auto& ion = std::dynamic_pointer_cast<const ast::Useion>(ion_node);
        ion_vars.push_back(ion->get_node_name());
        for (const auto& var: ion->get_readlist()) {
            read_ion_vars.push_back(var->get_node_name());
        }
        for (const auto& var: ion->get_writelist()) {
            write_ion_vars.push_back(var->get_node_name());
        }
    }

    /**
     * Check if given variable belongs to given ion.
     * For example, eca belongs to ca ion, nai belongs to na ion.
     * We just check if we exclude first/last char, if that is ion name.
     */
    auto ion_variable = [](const std::string& var, const std::string& ion) -> bool {
        auto len = var.size() - 1;
        return (var.substr(1, len) == ion || var.substr(0, len) == ion);
    };

    /// iterate over all ion types and construct the Ion objects
    for (auto& ion_name: ion_vars) {
        Ion ion(ion_name);
        for (auto& read_var: read_ion_vars) {
            if (ion_variable(read_var, ion_name)) {
                ion.reads.push_back(read_var);
            }
        }
        for (auto& write_var: write_ion_vars) {
            if (ion_variable(write_var, ion_name)) {
                ion.writes.push_back(write_var);
                if (ion.is_intra_cell_conc(write_var) || ion.is_extra_cell_conc(write_var)) {
                    ion.need_style = true;
                    info.write_concentration = true;
                }
            }
        }
        info.ions.push_back(std::move(ion));
    }

    /// once ions are populated, we can find all currents
    auto vars = psymtab->get_variables_with_properties(NmodlType::nonspecific_cur_var);
    for (auto& var: vars) {
        info.currents.push_back(var->get_name());
    }
    vars = psymtab->get_variables_with_properties(NmodlType::electrode_cur_var);
    for (auto& var: vars) {
        info.currents.push_back(var->get_name());
    }
    for (auto& ion: info.ions) {
        for (auto& var: ion.writes) {
            if (ion.is_ionic_current(var)) {
                info.currents.push_back(var);
            }
        }
    }

    /// check if write_conc(...) will be needed
    for (const auto& ion: info.ions) {
        for (const auto& var: ion.writes) {
            if (!ion.is_ionic_current(var) && !ion.is_rev_potential(var)) {
                info.require_wrote_conc = true;
            }
        }
    }
}


/**
 * Find non-range variables i.e. ones that are not belong to per instance allocation
 *
 * Certain variables like pointers, global, parameters are not necessary to be per
 * instance variables. NEURON apply certain rules to determine which variables become
 * thread, static or global variables. Here we construct those variables.
 */
void CodegenHelperVisitor::find_non_range_variables() {
    /**
     * Top local variables are local variables appear in global scope. All local
     * variables in program symbol table are in global scope.
     */
    info.constant_variables = psymtab->get_variables_with_properties(NmodlType::constant_var);
    info.top_local_variables = psymtab->get_variables_with_properties(NmodlType::local_var);

    /**
     * All global variables remain global if mod file is not marked thread safe.
     * Otherwise, global variables written at least once gets promoted to thread variables.
     */

    std::string variables;

    auto vars = psymtab->get_variables_with_properties(NmodlType::global_var);
    for (auto& var: vars) {
        if (info.thread_safe && var->get_write_count() > 0) {
            var->mark_thread_safe();
            info.thread_variables.push_back(var);
            info.thread_var_data_size += var->get_length();
            variables += " " + var->get_name();
        } else {
            info.global_variables.push_back(var);
        }
    }

    /**
     * If parameter is not a range and used only as read variable then it becomes global
     * variable. To qualify it as thread variable it must be be written at least once and
     * mod file must be marked as thread safe.
     * To exclusively get parameters only, we exclude all other variables (in without)
     * and then sort them with neuron/mod2c order.
     */
    // clang-format off
    auto with = NmodlType::param_assign;
    auto without = NmodlType::range_var
                   | NmodlType::assigned_definition
                   | NmodlType::global_var
                   | NmodlType::pointer_var
                   | NmodlType::bbcore_pointer_var
                   | NmodlType::read_ion_var
                   | NmodlType::write_ion_var;
    // clang-format on
    vars = psymtab->get_variables(with, without);
    for (auto& var: vars) {
        // some variables like area and diam are declared in parameter
        // block but they are not global
        if (var->get_name() == naming::DIAM_VARIABLE || var->get_name() == naming::AREA_VARIABLE ||
            var->has_any_property(NmodlType::extern_neuron_variable)) {
            continue;
        }

        // if model is thread safe and if parameter is being written then
        // those variables should be promoted to thread safe variable
        if (info.thread_safe && var->get_write_count() > 0) {
            var->mark_thread_safe();
            info.thread_variables.push_back(var);
            info.thread_var_data_size += var->get_length();
        } else {
            info.global_variables.push_back(var);
        }
    }
    sort_with_mod2c_symbol_order(info.thread_variables);

    /**
     * \todo Below we calculate thread related id and sizes. This will
     * need to do from global analysis pass as here we are handling
     * top local variables, global variables, derivimplicit method.
     * There might be more use cases with other solver methods.
     */

    /**
     * If derivimplicit is used, then first three thread ids get assigned to:
     * 1st thread is used for: deriv_advance
     * 2nd thread is used for: dith
     * 3rd thread is used for: newtonspace
     *
     * slist and dlist represent the offsets for prime variables used. For
     * euler or derivimplicit methods its always first number.
     */
    if (info.derivimplicit_used()) {
        info.derivimplicit_var_thread_id = 0;
        info.thread_data_index = 3;
        info.derivimplicit_list_num = 1;
        info.thread_callback_register = true;
    }

    /// next thread id is allocated for top local variables
    if (info.vectorize && !info.top_local_variables.empty()) {
        info.top_local_thread_id = info.thread_data_index++;
        info.thread_callback_register = true;
    }

    /// next thread id is allocated for thread promoted variables
    if (info.vectorize && !info.thread_variables.empty()) {
        info.thread_var_thread_id = info.thread_data_index++;
        info.thread_callback_register = true;
    }

    /// find total size of local variables in global scope
    for (auto& var: info.top_local_variables) {
        info.top_local_thread_size += var->get_length();
    }

    /// find number of prime variables and total size
    auto primes = psymtab->get_variables_with_properties(NmodlType::prime_name);
    info.num_primes = primes.size();
    for (auto& variable: primes) {
        info.primes_size += variable->get_length();
    }

    /// find pointer or bbcore pointer variables
    // clang-format off
    auto properties = NmodlType::pointer_var
                      | NmodlType::bbcore_pointer_var;
    // clang-format on
    info.pointer_variables = psymtab->get_variables_with_properties(properties);

    // find special variables like diam, area
    // clang-format off
    properties = NmodlType::assigned_definition
            | NmodlType::param_assign;
    vars = psymtab->get_variables_with_properties(properties);
    for (auto& var : vars) {
        if (var->get_name() == naming::AREA_VARIABLE) {
            info.area_used = true;
        }
        if (var->get_name() == naming::DIAM_VARIABLE) {
            info.diam_used = true;
        }
    }
    // clang-format on
}

/**
 * Find range variables i.e. ones that are belong to per instance allocation
 *
 * In order to be compatible with NEURON, we need to print all variables in
 * exact order as NEURON/MOD2C implementation. This is important because memory
 * for all variables is allocated in single 1-D array with certain offset
 * for each variable. The order of variables determine the offset and hence
 * they must be in same order as NEURON.
 *
 * Here is how order is determined into NEURON/MOD2C implementation:
 *
 * First, following three lists are created
 * - variables with parameter and range property (List 1)
 * - variables with state and range property (List 2)
 * - variables with assigned and range property (List 3)
 *
 * Once created, we remove some variables due to the following criteria:
 * - In NEURON/MOD2C implementation, we remove variables with NRNPRANGEIN
 *   or NRNPRANGEOUT type
 * - So who has NRNPRANGEIN and NRNPRANGEOUT type? these are USEION read
 *   or write variables that are not ionic currents.
 * - This is the reason for mod files CaDynamics_E2.mod or cal_mig.mod, ica variable
 *   is printed earlier in the list but other variables like cai, cao don't appear
 *   in same order.
 *
 * Finally we create 4th list:
 *  - variables with assigned property and not in the previous 3 lists
 *
 * We now print the variables in following order:
 *
 * - List 1 i.e. range + parameter variables are printed first
 * - List 3 i.e. range + assigned variables are printed next
 * - List 2 i.e. range + state variables are printed next
 * - List 4 i.e. assigned and ion variables not present in the previous 3 lists
 *
 * NOTE:
 * - State variables also have the property `assigned_definition` but these variables
 *   are not from ASSIGNED block.
 * - Variable can not be range as well as state, it's redeclaration error
 * - Variable can be parameter as well as range. Without range, parameter
 *   is considered as global variable i.e. one value for all instances.
 * - If a variable is only defined as RANGE and not in assigned or parameter
 *   or state block then it's not printed.
 * - Note that a variable property is different than the variable type. For example,
 *   if variable has range property, it doesn't mean the variable is declared as RANGE.
 *   Other variables like STATE and ASSIGNED block variables also get range property
 *   without being explicitly declared as RANGE in the mod file.
 * - Also, there is difference between declaration order vs. definition order. For
 *   example, POINTER variable in NEURON block is just declaration and doesn't
 *   determine the order in which they will get printed. Below we query symbol table
 *   and order all instance variables into certain order.
 */
void CodegenHelperVisitor::find_range_variables() {
    /// comparator to decide the order based on definition
    auto comparator = [](const SymbolType& first, const SymbolType& second) -> bool {
        return first->get_definition_order() < second->get_definition_order();
    };

    /// from symbols vector `vars`, remove all ion variables which are not ionic currents
    auto remove_non_ioncur_vars = [](SymbolVectorType& vars, const CodegenInfo& info) -> void {
        vars.erase(std::remove_if(vars.begin(),
                                  vars.end(),
                                  [&](SymbolType& s) {
                                      return info.is_ion_variable(s->get_name()) &&
                                             !info.is_ionic_current(s->get_name());
                                  }),
                   vars.end());
    };

    /// if `secondary` vector contains any symbol that exist in the `primary` then remove it
    auto remove_var_exist = [](SymbolVectorType& primary, SymbolVectorType& secondary) -> void {
        secondary.erase(std::remove_if(secondary.begin(),
                                       secondary.end(),
                                       [&primary](const SymbolType& tosearch) {
                                           return std::find_if(primary.begin(),
                                                               primary.end(),
                                                               // compare by symbol name
                                                               [&tosearch](
                                                                   const SymbolType& symbol) {
                                                                   return tosearch->get_name() ==
                                                                          symbol->get_name();
                                                               }) != primary.end();
                                       }),
                        secondary.end());
    };

    /**
     * First come parameters which are range variables.
     */
    // clang-format off
    auto with = NmodlType::range_var
                | NmodlType::param_assign;
    auto without = NmodlType::global_var
                   | NmodlType::pointer_var
                   | NmodlType::bbcore_pointer_var
                   | NmodlType::state_var;

    // clang-format on
    info.range_parameter_vars = psymtab->get_variables(with, without);
    remove_non_ioncur_vars(info.range_parameter_vars, info);
    std::sort(info.range_parameter_vars.begin(), info.range_parameter_vars.end(), comparator);

    /**
     * Second come assigned variables which are range variables.
     */
    // clang-format off
    with = NmodlType::range_var
           | NmodlType::assigned_definition;
    without = NmodlType::global_var
              | NmodlType::pointer_var
              | NmodlType::bbcore_pointer_var
              | NmodlType::state_var
              | NmodlType::param_assign;

    // clang-format on
    info.range_assigned_vars = psymtab->get_variables(with, without);
    remove_non_ioncur_vars(info.range_assigned_vars, info);
    std::sort(info.range_assigned_vars.begin(), info.range_assigned_vars.end(), comparator);


    /**
     * Third come state variables. All state variables are kind of range by default.
     * Note that some mod files like CaDynamics_E2.mod use cai as state variable which
     * appear in USEION read/write list. These variables are not considered in this
     * variables because non ionic-current variables are removed and printed later.
     */
    // clang-format off
    with = NmodlType::state_var;
    without = NmodlType::global_var
              | NmodlType::pointer_var
              | NmodlType::bbcore_pointer_var;

    // clang-format on
    info.state_vars = psymtab->get_variables(with, without);
    std::sort(info.state_vars.begin(), info.state_vars.end(), comparator);

    /// range_state_vars is copy of state variables but without non ionic current variables
    info.range_state_vars = info.state_vars;
    remove_non_ioncur_vars(info.range_state_vars, info);

    /// Remaining variables are assigned and ion variables which are not in the previous 3 lists

    // clang-format off
    with = NmodlType::assigned_definition
           | NmodlType::read_ion_var
           | NmodlType::write_ion_var;
    without = NmodlType::global_var
              | NmodlType::pointer_var
              | NmodlType::bbcore_pointer_var
              | NmodlType::extern_neuron_variable;
    // clang-format on
    const auto& variables = psymtab->get_variables_with_properties(with, false);
    for (const auto& variable: variables) {
        if (!variable->has_any_property(without)) {
            info.assigned_vars.push_back(variable);
        }
    }

    /// make sure that variables already present in previous lists
    /// are removed to avoid any duplication
    remove_var_exist(info.range_parameter_vars, info.assigned_vars);
    remove_var_exist(info.range_assigned_vars, info.assigned_vars);
    remove_var_exist(info.range_state_vars, info.assigned_vars);

    /// sort variables with their definition order
    std::sort(info.assigned_vars.begin(), info.assigned_vars.end(), comparator);
}


void CodegenHelperVisitor::find_table_variables() {
    auto property = NmodlType::table_statement_var;
    info.table_statement_variables = psymtab->get_variables_with_properties(property);
    property = NmodlType::table_assigned_var;
    info.table_assigned_variables = psymtab->get_variables_with_properties(property);
}


void CodegenHelperVisitor::visit_suffix(const Suffix& node) {
    const auto& type = node.get_type()->get_node_name();
    if (type == naming::POINT_PROCESS) {
        info.point_process = true;
    }
    if (type == naming::ARTIFICIAL_CELL) {
        info.artificial_cell = true;
        info.point_process = true;
    }
    info.mod_suffix = node.get_node_name();
}


void CodegenHelperVisitor::visit_electrode_current(const ElectrodeCurrent& node) {
    info.electrode_current = true;
}


void CodegenHelperVisitor::visit_initial_block(const InitialBlock& node) {
    if (under_net_receive_block) {
        info.net_receive_initial_node = &node;
    } else {
        info.initial_node = &node;
    }
    node.visit_children(*this);
}


void CodegenHelperVisitor::visit_constructor_block(const ConstructorBlock& node) {
    info.constructor_node = &node;
    node.visit_children(*this);
}


void CodegenHelperVisitor::visit_destructor_block(const DestructorBlock& node) {
    info.destructor_node = &node;
    node.visit_children(*this);
}


void CodegenHelperVisitor::visit_net_receive_block(const NetReceiveBlock& node) {
    under_net_receive_block = true;
    info.net_receive_node = &node;
    info.num_net_receive_parameters = node.get_parameters().size();
    node.visit_children(*this);
    under_net_receive_block = false;
}


void CodegenHelperVisitor::visit_derivative_block(const DerivativeBlock& node) {
    under_derivative_block = true;
    node.visit_children(*this);
    under_derivative_block = false;
}

void CodegenHelperVisitor::visit_derivimplicit_callback(const ast::DerivimplicitCallback& node) {
    info.derivimplicit_callbacks.push_back(&node);
}


void CodegenHelperVisitor::visit_breakpoint_block(const BreakpointBlock& node) {
    under_breakpoint_block = true;
    info.breakpoint_node = &node;
    node.visit_children(*this);
    under_breakpoint_block = false;
}


void CodegenHelperVisitor::visit_nrn_state_block(const ast::NrnStateBlock& node) {
    info.nrn_state_block = &node;
    node.visit_children(*this);
}


void CodegenHelperVisitor::visit_procedure_block(const ast::ProcedureBlock& node) {
    info.procedures.push_back(&node);
    node.visit_children(*this);
    if (table_statement_used) {
        table_statement_used = false;
        info.functions_with_table.push_back(&node);
    }
}


void CodegenHelperVisitor::visit_function_block(const ast::FunctionBlock& node) {
    info.functions.push_back(&node);
    node.visit_children(*this);
    if (table_statement_used) {
        table_statement_used = false;
        info.functions_with_table.push_back(&node);
    }
}


void CodegenHelperVisitor::visit_eigen_newton_solver_block(
    const ast::EigenNewtonSolverBlock& node) {
    info.eigen_newton_solver_exist = true;
    node.visit_children(*this);
}

void CodegenHelperVisitor::visit_eigen_linear_solver_block(
    const ast::EigenLinearSolverBlock& node) {
    info.eigen_linear_solver_exist = true;
    node.visit_children(*this);
}

void CodegenHelperVisitor::visit_function_call(const FunctionCall& node) {
    auto name = node.get_node_name();
    if (name == naming::NET_SEND_METHOD) {
        info.net_send_used = true;
    }
    if (name == naming::NET_EVENT_METHOD) {
        info.net_event_used = true;
    }
}


void CodegenHelperVisitor::visit_conductance_hint(const ConductanceHint& node) {
    const auto& ion = node.get_ion();
    const auto& variable = node.get_conductance();
    std::string ion_name;
    if (ion) {
        ion_name = ion->get_node_name();
    }
    info.conductances.push_back({ion_name, variable->get_node_name()});
}


/**
 * Visit statement block and find prime symbols appear in derivative block
 *
 * Equation statements in derivative block has prime on the lhs. The order
 * of primes could be different that declaration state block. Also, not all
 * state variables need to appear in equation block. In this case, we want
 * to find out the the primes in the order of equation definition. This is
 * just to keep the same order as neuron implementation.
 *
 * The primes are already solved and replaced by Dstate or name. And hence
 * we need to check if the lhs variable is derived from prime name. If it's
 * Dstate then we have to lookup state to find out corresponding symbol. This
 * is because prime_variables_by_order should contain state variable name and
 * not the one replaced by solver pass.
 */
void CodegenHelperVisitor::visit_statement_block(const ast::StatementBlock& node) {
    const auto& statements = node.get_statements();
    for (auto& statement: statements) {
        statement->accept(*this);
        if (under_derivative_block && assign_lhs &&
            (assign_lhs->is_name() || assign_lhs->is_var_name())) {
            auto name = assign_lhs->get_node_name();
            auto symbol = psymtab->lookup(name);
            if (symbol != nullptr) {
                auto is_prime = symbol->has_any_property(NmodlType::prime_name);
                auto from_state = symbol->has_any_status(Status::from_state);
                if (is_prime || from_state) {
                    if (from_state) {
                        symbol = psymtab->lookup(name.substr(1, name.size()));
                    }
                    info.prime_variables_by_order.push_back(symbol);
                    info.num_equations++;
                }
            }
        }
        assign_lhs = nullptr;
    }
}

void CodegenHelperVisitor::visit_factor_def(const ast::FactorDef& node) {
    info.factor_definitions.push_back(&node);
}


void CodegenHelperVisitor::visit_binary_expression(const BinaryExpression& node) {
    if (node.get_op().eval() == "=") {
        assign_lhs = node.get_lhs();
    }
    node.get_lhs()->accept(*this);
    node.get_rhs()->accept(*this);
}


void CodegenHelperVisitor::visit_bbcore_pointer(const BbcorePointer& node) {
    info.bbcore_pointer_used = true;
}


void CodegenHelperVisitor::visit_watch(const ast::Watch& node) {
    info.watch_count++;
}


void CodegenHelperVisitor::visit_watch_statement(const ast::WatchStatement& node) {
    info.watch_statements.push_back(&node);
    node.visit_children(*this);
}


void CodegenHelperVisitor::visit_for_netcon(const ast::ForNetcon& node) {
    info.for_netcon_used = true;
}


void CodegenHelperVisitor::visit_table_statement(const ast::TableStatement& node) {
    info.table_count++;
    table_statement_used = true;
}


void CodegenHelperVisitor::visit_program(const ast::Program& node) {
    psymtab = node.get_symbol_table();
    auto blocks = node.get_blocks();
    for (auto& block: blocks) {
        info.top_blocks.push_back(block.get());
        if (block->is_verbatim()) {
            info.top_verbatim_blocks.push_back(block.get());
        }
    }
    node.visit_children(*this);
    find_ion_variables(node);  // Keep this before find_*_range_variables()
    find_range_variables();
    find_non_range_variables();
    find_table_variables();
}


CodegenInfo CodegenHelperVisitor::analyze(const ast::Program& node) {
    node.accept(*this);
    return info;
}

void CodegenHelperVisitor::visit_linear_block(const ast::LinearBlock& node) {
    info.vectorize = false;
}

void CodegenHelperVisitor::visit_non_linear_block(const ast::NonLinearBlock& node) {
    info.vectorize = false;
}

void CodegenHelperVisitor::visit_discrete_block(const ast::DiscreteBlock& node) {
    info.vectorize = false;
}

void CodegenHelperVisitor::visit_partial_block(const ast::PartialBlock& node) {
    info.vectorize = false;
}

void CodegenHelperVisitor::visit_update_dt(const ast::UpdateDt& node) {
    info.changed_dt = node.get_value()->eval();
}

}  // namespace codegen
}  // namespace nmodl
