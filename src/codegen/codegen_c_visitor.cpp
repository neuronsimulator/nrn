/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <algorithm>
#include <cmath>
#include <ctime>

#include "codegen/codegen_c_visitor.hpp"
#include "codegen/codegen_helper_visitor.hpp"
#include "codegen/codegen_naming.hpp"
#include "config/config.h"
#include "parser/c11_driver.hpp"
#include "utils/logger.hpp"
#include "utils/string_utils.hpp"
#include "visitors/lookup_visitor.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/var_usage_visitor.hpp"
#include "visitors/visitor_utils.hpp"

using namespace fmt::literals;

namespace nmodl {
namespace codegen {

using namespace ast;

using visitor::AstLookupVisitor;
using visitor::RenameVisitor;
using visitor::VarUsageVisitor;

using symtab::syminfo::NmodlType;
using SymbolType = std::shared_ptr<symtab::Symbol>;

/****************************************************************************************/
/*                            Overloaded visitor routines                               */
/****************************************************************************************/


void CodegenCVisitor::visit_string(String* node) {
    if (!codegen) {
        return;
    }
    std::string name = node->eval();
    if (enable_variable_name_lookup) {
        name = get_variable_name(name);
    }
    printer->add_text(name);
}


void CodegenCVisitor::visit_integer(Integer* node) {
    if (!codegen) {
        return;
    }
    auto macro = node->get_macro();
    auto value = node->get_value();
    if (macro) {
        macro->accept(this);
    } else {
        printer->add_text(std::to_string(value));
    }
}


void CodegenCVisitor::visit_float(Float* node) {
    if (!codegen) {
        return;
    }
    auto value = node->eval();
    printer->add_text(float_to_string(value));
}


void CodegenCVisitor::visit_double(Double* node) {
    if (!codegen) {
        return;
    }
    auto value = node->eval();
    printer->add_text(double_to_string(value));
}


void CodegenCVisitor::visit_boolean(Boolean* node) {
    if (!codegen) {
        return;
    }
    printer->add_text(std::to_string(static_cast<int>(node->eval())));
}


void CodegenCVisitor::visit_name(Name* node) {
    if (!codegen) {
        return;
    }
    node->visit_children(this);
}


void CodegenCVisitor::visit_unit(ast::Unit* node) {
    // do not print units
}


void CodegenCVisitor::visit_prime_name(PrimeName* node) {
    throw std::runtime_error("PRIME encountered during code generation, ODEs not solved?");
}


/// \todo : validate how @ is being handled in neuron implementation
void CodegenCVisitor::visit_var_name(VarName* node) {
    if (!codegen) {
        return;
    }
    auto name = node->get_name();
    auto at_index = node->get_at();
    auto index = node->get_index();
    name->accept(this);
    if (at_index) {
        printer->add_text("@");
        at_index->accept(this);
    }
    if (index) {
        printer->add_text("[");
        index->accept(this);
        printer->add_text("]");
    }
}


void CodegenCVisitor::visit_indexed_name(IndexedName* node) {
    if (!codegen) {
        return;
    }
    node->get_name()->accept(this);
    printer->add_text("[");
    node->get_length()->accept(this);
    printer->add_text("]");
}


void CodegenCVisitor::visit_local_list_statement(LocalListStatement* node) {
    if (!codegen) {
        return;
    }
    auto type = local_var_type() + " ";
    printer->add_text(type);
    print_vector_elements(node->get_variables(), ", ");
}


void CodegenCVisitor::visit_if_statement(IfStatement* node) {
    if (!codegen) {
        return;
    }
    printer->add_text("if (");
    node->get_condition()->accept(this);
    printer->add_text(") ");
    node->get_statement_block()->accept(this);
    print_vector_elements(node->get_elseifs(), "");
    auto elses = node->get_elses();
    if (elses) {
        elses->accept(this);
    }
}


void CodegenCVisitor::visit_else_if_statement(ElseIfStatement* node) {
    if (!codegen) {
        return;
    }
    printer->add_text(" else if (");
    node->get_condition()->accept(this);
    printer->add_text(") ");
    node->get_statement_block()->accept(this);
}


void CodegenCVisitor::visit_else_statement(ElseStatement* node) {
    if (!codegen) {
        return;
    }
    printer->add_text(" else ");
    node->visit_children(this);
}


void CodegenCVisitor::visit_while_statement(WhileStatement* node) {
    printer->add_text("while (");
    node->get_condition()->accept(this);
    printer->add_text(") ");
    node->get_statement_block()->accept(this);
}


void CodegenCVisitor::visit_from_statement(ast::FromStatement* node) {
    if (!codegen) {
        return;
    }
    auto name = node->get_node_name();
    auto from = node->get_from();
    auto to = node->get_to();
    auto inc = node->get_increment();
    auto block = node->get_statement_block();
    printer->add_text("for(int {}="_format(name));
    from->accept(this);
    printer->add_text("; {}<="_format(name));
    to->accept(this);
    if (inc) {
        printer->add_text("; {}+="_format(name));
        inc->accept(this);
    } else {
        printer->add_text("; {}++"_format(name));
    }
    printer->add_text(")");
    block->accept(this);
}


void CodegenCVisitor::visit_paren_expression(ParenExpression* node) {
    if (!codegen) {
        return;
    }
    printer->add_text("(");
    node->get_expression()->accept(this);
    printer->add_text(")");
}


void CodegenCVisitor::visit_binary_expression(BinaryExpression* node) {
    if (!codegen) {
        return;
    }
    auto op = node->get_op().eval();
    auto lhs = node->get_lhs();
    auto rhs = node->get_rhs();
    if (op == "^") {
        printer->add_text("pow(");
        lhs->accept(this);
        printer->add_text(", ");
        rhs->accept(this);
        printer->add_text(")");
    } else {
        lhs->accept(this);
        printer->add_text(" " + op + " ");
        rhs->accept(this);
    }
}


void CodegenCVisitor::visit_binary_operator(BinaryOperator* node) {
    if (!codegen) {
        return;
    }
    printer->add_text(node->eval());
}


void CodegenCVisitor::visit_unary_operator(UnaryOperator* node) {
    if (!codegen) {
        return;
    }
    printer->add_text(" " + node->eval());
}


/**
 * Statement block is top level construct (for every nmodl block).
 * Sometime we want to analyse ast nodes even if code generation is
 * false. Hence we visit children even if code generation is false.
 */
void CodegenCVisitor::visit_statement_block(StatementBlock* node) {
    if (!codegen) {
        node->visit_children(this);
        return;
    }
    print_statement_block(node);
}


void CodegenCVisitor::visit_function_call(FunctionCall* node) {
    if (!codegen) {
        return;
    }
    print_function_call(node);
}


void CodegenCVisitor::visit_verbatim(Verbatim* node) {
    if (!codegen) {
        return;
    }
    auto text = node->get_statement()->eval();
    auto result = process_verbatim_text(text);

    auto statements = stringutils::split_string(result, '\n');
    for (auto& statement: statements) {
        stringutils::trim_newline(statement);
        if (statement.find_first_not_of(' ') != std::string::npos) {
            printer->add_line(statement);
        }
    }
}


/****************************************************************************************/
/*                               Common helper routines                                 */
/****************************************************************************************/


/**
 * Check if given statement needs to be skipped during code generation
 *
 * Certain statements like unit, comment, solve can/need to be skipped
 * during code generation. Note that solve block is wrapped in expression
 * statement and hence we have to check inner expression. It's also true
 * for the initial block defined inside net receive block.
 */
bool CodegenCVisitor::statement_to_skip(Statement* node) {
    // clang-format off
    if (node->is_unit_state()
        || node->is_line_comment()
        || node->is_block_comment()
        || node->is_solve_block()
        || node->is_conductance_hint()
        || node->is_table_statement()) {
        return true;
    }
    // clang-format on
    if (node->is_expression_statement()) {
        auto expression = dynamic_cast<ExpressionStatement*>(node)->get_expression();
        if (expression->is_solve_block()) {
            return true;
        }
        if (expression->is_initial_block()) {
            return true;
        }
    }
    return false;
}


bool CodegenCVisitor::net_send_buffer_required() {
    if (net_receive_required() && !info.artificial_cell) {
        if (info.net_event_used || info.net_send_used || info.is_watch_used()) {
            return true;
        }
    }
    return false;
}


bool CodegenCVisitor::net_receive_buffering_required() {
    return info.point_process && !info.artificial_cell && info.net_receive_node != nullptr;
}


bool CodegenCVisitor::nrn_state_required() {
    if (info.artificial_cell) {
        return false;
    }
    return info.nrn_state_block != nullptr || info.currents.empty();
}


bool CodegenCVisitor::nrn_cur_required() {
    return info.breakpoint_node != nullptr && !info.currents.empty();
}


bool CodegenCVisitor::net_receive_exist() {
    return info.net_receive_node != nullptr;
}


bool CodegenCVisitor::breakpoint_exist() {
    return info.breakpoint_node != nullptr;
}


bool CodegenCVisitor::net_receive_required() {
    return net_receive_exist();
}


/**
 * When floating point data type is not default (i.e. double) then we
 * have to copy old array to new type (for range variables).
 */
bool CodegenCVisitor::range_variable_setup_required() {
    return codegen::naming::DEFAULT_FLOAT_TYPE != float_data_type();
}


bool CodegenCVisitor::state_variable(std::string name) {
    // clang-format off
    auto result = std::find_if(info.state_vars.begin(),
                               info.state_vars.end(),
                               [&name](const SymbolType& sym) {
                                   return name == sym->get_name();
                               }
    );
    // clang-format on
    return result != info.state_vars.end();
}


int CodegenCVisitor::position_of_float_var(const std::string& name) {
    int index = 0;
    for (const auto& var: codegen_float_variables) {
        if (var->get_name() == name) {
            return index;
        }
        index += var->get_length();
    }
    throw std::logic_error(name + " variable not found");
}


int CodegenCVisitor::position_of_int_var(const std::string& name) {
    int index = 0;
    for (const auto& var: codegen_int_variables) {
        if (var.symbol->get_name() == name) {
            return index;
        }
        index += var.symbol->get_length();
    }
    throw std::logic_error(name + " variable not found");
}


/**
 * Convert double value to string
 *
 * We can directly use to_string method but if user specify 7.0 then it gets
 * printed as 7 (as integer). To avoid this, we use below wrapper. But note
 * that there are still issues. For example, if 1.1 is not exactly represented
 * in floating point, then it gets printed as 1.0999999999999. May be better
 * to use std::to_string in else part?
 */
std::string CodegenCVisitor::double_to_string(double value) {
    if (ceilf(value) == value) {
        return "{:.1f}"_format(value);
    }
    return "{:.16g}"_format(value);
}


std::string CodegenCVisitor::float_to_string(float value) {
    if (ceilf(value) == value) {
        return "{:.1f}f"_format(value);
    }
    return "{:.16g}f"_format(value);
}


/**
 * Check if given statement needs semicolon at the end of statement
 *
 * Statements like if, else etc. don't need semicolon at the end.
 * (Note that it's valid to have "extraneous" semicolon). Also, statement
 * block can appear as statement using expression statement which need to
 * be inspected.
 */
bool CodegenCVisitor::need_semicolon(Statement* node) {
    // clang-format off
    if (node->is_if_statement()
        || node->is_else_if_statement()
        || node->is_else_statement()
        || node->is_from_statement()
        || node->is_verbatim()
        || node->is_for_all_statement()
        || node->is_from_statement()
        || node->is_conductance_hint()
        || node->is_while_statement()) {
        return false;
    }
    if (node->is_expression_statement()) {
        auto expression = dynamic_cast<ExpressionStatement*>(node)->get_expression();
        if (expression->is_statement_block()
            || expression->is_eigen_newton_solver_block()
            || expression->is_eigen_linear_solver_block()
            || expression->is_solution_expression()) {
            return false;
        }
    }
    // clang-format on
    return true;
}


// check if there is a function or procedure defined with given name
bool CodegenCVisitor::defined_method(const std::string& name) {
    auto function = program_symtab->lookup(name);
    auto properties = NmodlType::function_block | NmodlType::procedure_block;
    return function && function->has_any_property(properties);
}


/**
 * Return "current" for variable name used in breakpoint block
 *
 * Current variable used in breakpoint block could be local variable.
 * In this case, neuron has already renamed the variable name by prepending
 * "_l". In our implementation, the variable could have been renamed by
 * one of the pass. And hence, we search all local variables and check if
 * the variable is renamed. Note that we have to look into the symbol table
 * of statement block and not breakpoint.
 */
std::string CodegenCVisitor::breakpoint_current(std::string current) {
    auto breakpoint = info.breakpoint_node;
    if (breakpoint == nullptr) {
        return current;
    }
    auto symtab = breakpoint->get_statement_block()->get_symbol_table();
    auto variables = symtab->get_variables_with_properties(NmodlType::local_var);
    for (const auto& var: variables) {
        auto renamed_name = var->get_name();
        auto original_name = var->get_original_name();
        if (current == original_name) {
            current = renamed_name;
            break;
        }
    }
    return current;
}


int CodegenCVisitor::float_variables_size() {
    auto count_length = [](std::vector<SymbolType>& variables) -> int {
        int length = 0;
        for (const auto& variable: variables) {
            length += variable->get_length();
        }
        return length;
    };

    int float_size = count_length(info.range_parameter_vars);
    float_size += count_length(info.range_dependent_vars);
    float_size += count_length(info.state_vars);
    float_size += count_length(info.dependent_vars);

    /// for state variables we add Dstate variables
    float_size += info.state_vars.size();
    float_size += info.ion_state_vars.size();

    /// for v_unused variable
    if (info.vectorize) {
        float_size++;
    }
    /// for g_unused variable
    if (breakpoint_exist()) {
        float_size++;
    }
    /// for tsave variable
    if (net_receive_exist()) {
        float_size++;
    }
    return float_size;
}


int CodegenCVisitor::int_variables_size() {
    int num_variables = 0;
    for (const auto& semantic: info.semantics) {
        num_variables += semantic.size;
    }
    return num_variables;
}


/**
 * For given block type, return statements for all read ion variables
 *
 * Depending upon the block type, we have to print read/write ion variables
 * during code generation. Depending on block/procedure being printed, this
 * method return statements as vector. As different code backends could have
 * different variable names, we rely on backend-specific read_ion_variable_name
 * and write_ion_variable_name method which will be overloaded.
 *
 * \todo: After looking into mod2c and neuron implementation, it seems like
 * Ode block type is not used (?). Need to look into implementation details.
 */
std::vector<std::string> CodegenCVisitor::ion_read_statements(BlockType type) {
    if (optimize_ion_variable_copies()) {
        return ion_read_statements_optimized(type);
    }
    std::vector<std::string> statements;
    for (const auto& ion: info.ions) {
        auto name = ion.name;
        for (const auto& var: ion.reads) {
            if (type == BlockType::Ode && ion.is_ionic_conc(var) && state_variable(var)) {
                continue;
            }
            auto variable_names = read_ion_variable_name(var);
            auto first = get_variable_name(variable_names.first);
            auto second = get_variable_name(variable_names.second);
            statements.push_back("{} = {};"_format(first, second));
        }
        for (const auto& var: ion.writes) {
            if (type == BlockType::Ode && ion.is_ionic_conc(var) && state_variable(var)) {
                continue;
            }
            if (ion.is_ionic_conc(var)) {
                auto variables = read_ion_variable_name(var);
                auto first = get_variable_name(variables.first);
                auto second = get_variable_name(variables.second);
                statements.push_back("{} = {};"_format(first, second));
            }
        }
    }
    return statements;
}


std::vector<std::string> CodegenCVisitor::ion_read_statements_optimized(BlockType type) {
    std::vector<std::string> statements;
    for (const auto& ion: info.ions) {
        for (const auto& var: ion.writes) {
            if (type == BlockType::Ode && ion.is_ionic_conc(var) && state_variable(var)) {
                continue;
            }
            if (ion.is_ionic_conc(var)) {
                auto variables = read_ion_variable_name(var);
                auto first = "ionvar." + variables.first;
                auto second = get_variable_name(variables.second);
                statements.push_back("{} = {};"_format(first, second));
            }
        }
    }
    return statements;
}


std::vector<ShadowUseStatement> CodegenCVisitor::ion_write_statements(BlockType type) {
    std::vector<ShadowUseStatement> statements;
    for (const auto& ion: info.ions) {
        std::string concentration;
        auto name = ion.name;
        for (const auto& var: ion.writes) {
            auto variable_names = write_ion_variable_name(var);
            if (ion.is_ionic_current(var)) {
                if (type == BlockType::Equation) {
                    auto current = breakpoint_current(var);
                    auto lhs = variable_names.first;
                    auto op = "+=";
                    auto rhs = get_variable_name(current);
                    if (info.point_process) {
                        auto area = get_variable_name(naming::NODE_AREA_VARIABLE);
                        rhs += "*(1.e2/{})"_format(area);
                    }
                    statements.push_back(ShadowUseStatement{lhs, op, rhs});
                }
            } else {
                if (!ion.is_rev_potential(var)) {
                    concentration = var;
                }
                auto lhs = variable_names.first;
                auto op = "=";
                auto rhs = get_variable_name(variable_names.second);
                statements.push_back(ShadowUseStatement{lhs, op, rhs});
            }
        }

        if (type == BlockType::Initial && !concentration.empty()) {
            int index = 0;
            if (ion.is_intra_cell_conc(concentration)) {
                index = 1;
            } else if (ion.is_extra_cell_conc(concentration)) {
                index = 2;
            } else {
                /// \todo: unhandled case in neuron implementation
                throw std::logic_error("codegen error for {} ion"_format(ion.name));
            }
            auto ion_type_name = "{}_type"_format(ion.name);
            auto lhs = "int {}"_format(ion_type_name);
            auto op = "=";
            auto rhs = get_variable_name(ion_type_name);
            statements.push_back(ShadowUseStatement{lhs, op, rhs});
            auto statement = conc_write_statement(ion.name, concentration, index);
            statements.push_back(ShadowUseStatement{statement, "", ""});
        }
    }
    return statements;
}


/**
 * Often top level verbatim blocks use variables with old names.
 * Here we process if we are processing verbatim block at global scope.
 */
std::string CodegenCVisitor::process_verbatim_token(const std::string& token) {
    const std::string& name = token;

    /**
     * If given token is procedure name and if it's defined
     * in the current mod file then it must be replaced
     */
    if (program_symtab->is_method_defined(token)) {
        return method_name(token);
    }

    /**
     * Check if token is commongly used variable name in
     * verbatim block like nt, _threadargs etc. If so, replace
     * it and return.
     */
    auto new_name = replace_if_verbatim_variable(name);
    if (new_name != name) {
        return get_variable_name(new_name, false);
    }

    /**
     * For top level verbatim blocks we shouldn't replace variable
     * names with Instance because arguments are provided from coreneuron
     * and they are missing inst.
     */
    auto use_instance = !printing_top_verbatim_blocks;
    return get_variable_name(token, use_instance);
}


bool CodegenCVisitor::ion_variable_struct_required() {
    return optimize_ion_variable_copies() && info.ion_has_write_variable();
}


/**
 * Check if variable is qualified as constant
 *
 * This can be override in the backend. For example, parameters can be constant
 * except in INITIAL block where they are set to 0. As initial block is/can be
 * executed on c/cpu backend, gpu/cuda backend can mark the parameter as constnat.
 */
bool CodegenCVisitor::is_constant_variable(std::string name) {
    auto symbol = program_symtab->lookup_in_scope(name);
    bool is_constant = false;
    if (symbol != nullptr) {
        if (symbol->has_any_property(NmodlType::read_ion_var)) {
            is_constant = true;
        }
        if (symbol->has_any_property(NmodlType::param_assign) && symbol->get_write_count() == 0) {
            is_constant = true;
        }
    }
    return is_constant;
}


/**
 * Once variables are populated, update index semantics to register with coreneuron
 */
void CodegenCVisitor::update_index_semantics() {
    int index = 0;
    info.semantics.clear();

    if (info.point_process) {
        info.semantics.emplace_back(index++, naming::AREA_SEMANTIC, 1);
        info.semantics.emplace_back(index++, naming::POINT_PROCESS_SEMANTIC, 1);
    }
    for (const auto& ion: info.ions) {
        for (auto& var: ion.reads) {
            info.semantics.emplace_back(index++, ion.name + "_ion", 1);
        }
        for (auto& var: ion.writes) {
            info.semantics.emplace_back(index++, ion.name + "_ion", 1);
            if (ion.is_ionic_current(var)) {
                info.semantics.emplace_back(index++, ion.name + "_ion", 1);
            }
        }
        if (ion.need_style) {
            info.semantics.emplace_back(index++, "#{}_ion"_format(ion.name), 1);
        }
    }
    for (auto& var: info.pointer_variables) {
        if (info.first_pointer_var_index == -1) {
            info.first_pointer_var_index = index;
        }
        int size = var->get_length();
        if (var->has_any_property(NmodlType::pointer_var)) {
            info.semantics.emplace_back(index, naming::POINTER_SEMANTIC, size);
        } else {
            info.semantics.emplace_back(index, naming::CORE_POINTER_SEMANTIC, size);
        }
        index += size;
    }

    if (info.diam_used) {
        info.semantics.emplace_back(index++, naming::DIAM_VARIABLE, 1);
    }

    if (info.area_used) {
        info.semantics.emplace_back(index++, naming::AREA_VARIABLE, 1);
    }

    if (info.net_send_used) {
        info.semantics.emplace_back(index++, naming::NET_SEND_SEMANTIC, 1);
    }

    /**
     * Number of semantics for watch is one greater than number of
     * actual watch statements in the mod file
     */
    if (!info.watch_statements.empty()) {
        for (int i = 0; i < info.watch_statements.size() + 1; i++) {
            info.semantics.emplace_back(index++, naming::WATCH_SEMANTIC, 1);
        }
    }

    if (info.for_netcon_used) {
        info.semantics.emplace_back(index++, naming::FOR_NETCON_SEMANTIC, 1);
    }
}


/**
 * Return all floating point variables required for code generation
 */
std::vector<SymbolType> CodegenCVisitor::get_float_variables() {
    /// sort with definition order
    auto comparator = [](const SymbolType& first, const SymbolType& second) -> bool {
        return first->get_definition_order() < second->get_definition_order();
    };

    auto dependents = info.dependent_vars;
    auto states = info.state_vars;
    states.insert(states.end(), info.ion_state_vars.begin(), info.ion_state_vars.end());

    /// each state variable has corresponding Dstate variable
    for (auto& variable: states) {
        auto name = "D" + variable->get_name();
        auto symbol = make_symbol(name);
        symbol->set_definition_order(variable->get_definition_order());
        dependents.push_back(symbol);
    }
    std::sort(dependents.begin(), dependents.end(), comparator);

    auto variables = info.range_parameter_vars;
    variables.insert(variables.end(),
                     info.range_dependent_vars.begin(),
                     info.range_dependent_vars.end());
    variables.insert(variables.end(), info.state_vars.begin(), info.state_vars.end());
    variables.insert(variables.end(), dependents.begin(), dependents.end());

    if (info.vectorize) {
        variables.push_back(make_symbol(naming::VOLTAGE_UNUSED_VARIABLE));
    }
    if (breakpoint_exist()) {
        std::string name = info.vectorize ? naming::CONDUCTANCE_UNUSED_VARIABLE
                                          : naming::CONDUCTANCE_VARIABLE;
        variables.push_back(make_symbol(name));
    }
    if (net_receive_exist()) {
        variables.push_back(make_symbol(naming::T_SAVE_VARIABLE));
    }
    return variables;
}


/**
 * Return all integer variables required for code generation
 *
 * IndexVariableInfo has following constructor arguments:
 *      - symbol
 *      - is_vdata   (false)
 *      - is_index   (false
 *      - is_integer (false)
 *
 * Which variables are constant qualified?
 *
 *  - node area is read only
 *  - read ion variables are read only
 *  - style_ionname is index / offset
 */
std::vector<IndexVariableInfo> CodegenCVisitor::get_int_variables() {
    std::vector<IndexVariableInfo> variables;
    if (info.point_process) {
        variables.emplace_back(make_symbol(naming::NODE_AREA_VARIABLE));
        variables.back().is_constant = true;
        /// note that this variable is not printed in neuron implementation
        if (info.artificial_cell) {
            variables.emplace_back(make_symbol(naming::POINT_PROCESS_VARIABLE), true);
        } else {
            variables.emplace_back(make_symbol(naming::POINT_PROCESS_VARIABLE), false, false, true);
            variables.back().is_constant = true;
        }
    }

    for (const auto& ion: info.ions) {
        bool need_style = false;
        for (const auto& var: ion.reads) {
            variables.emplace_back(make_symbol("ion_" + var));
            variables.back().is_constant = true;
        }
        for (const auto& var: ion.writes) {
            variables.emplace_back(make_symbol("ion_" + var));
            if (ion.is_ionic_current(var)) {
                variables.emplace_back(make_symbol("ion_di" + ion.name + "dv"));
            }
            if (ion.is_intra_cell_conc(var) || ion.is_extra_cell_conc(var)) {
                need_style = true;
            }
        }
        if (need_style) {
            variables.emplace_back(make_symbol("style_" + ion.name), false, true);
            variables.back().is_constant = true;
        }
    }

    for (const auto& var: info.pointer_variables) {
        auto name = var->get_name();
        if (var->has_any_property(NmodlType::pointer_var)) {
            variables.emplace_back(make_symbol(name));
        } else {
            variables.emplace_back(make_symbol(name), true);
        }
    }

    if (info.diam_used) {
        variables.emplace_back(make_symbol(naming::DIAM_VARIABLE));
    }

    if (info.area_used) {
        variables.emplace_back(make_symbol(naming::AREA_VARIABLE));
    }

    // for non-artificial cell, when net_receive buffering is enabled
    // then tqitem is an offset
    if (info.net_send_used) {
        if (info.artificial_cell) {
            variables.emplace_back(make_symbol(naming::TQITEM_VARIABLE), true);
        } else {
            variables.emplace_back(make_symbol(naming::TQITEM_VARIABLE), false, false, true);
            variables.back().is_constant = true;
        }
        info.tqitem_index = variables.size() - 1;
    }

    /**
     * Variables for watch statements : note that there is one extra variable
     * used in coreneuron compared to actual watch statements for compatibility
     * with neuron (which uses one extra Datum variable)
     */
    if (!info.watch_statements.empty()) {
        for (int i = 0; i < info.watch_statements.size() + 1; i++) {
            variables.emplace_back(make_symbol("watch{}"_format(i)), false, false, true);
        }
    }
    return variables;
}


/**
 * Return all ion write variables that require shadow vectors during code generation
 *
 * When we enable fine level parallelism at channel level, we have do updates
 * to ion variables in atomic way. As cpus don't have atomic instructions in
 * simd loop, we have to use shadow vectors for every ion variables. Here
 * we return list of all such variables.
 *
 * \todo: if conductances are specified, we don't need all below variables
 */
std::vector<SymbolType> CodegenCVisitor::get_shadow_variables() {
    std::vector<SymbolType> variables;
    for (const auto& ion: info.ions) {
        for (const auto& var: ion.writes) {
            variables.push_back({make_symbol(shadow_varname("ion_" + var))});
            if (ion.is_ionic_current(var)) {
                variables.push_back({make_symbol(shadow_varname("ion_di" + ion.name + "dv"))});
            }
        }
    }
    variables.push_back({make_symbol("ml_rhs")});
    variables.push_back({make_symbol("ml_d")});
    return variables;
}


/****************************************************************************************/
/*                      Routines must be overloaded in backend                          */
/****************************************************************************************/

/**
 * Print parameters
 */
std::string CodegenCVisitor::get_parameter_str(const ParamVector& params) {
    std::stringstream param_ss;
    for (auto iter = params.begin(); iter != params.end(); iter++) {
        param_ss << "{}{} {}{}"_format(std::get<0>(*iter),
                                       std::get<1>(*iter),
                                       std::get<2>(*iter),
                                       std::get<3>(*iter));
        if (!utils::is_last(iter, params)) {
            param_ss << ", ";
        }
    }
    return param_ss.str();
}


void CodegenCVisitor::print_channel_iteration_task_begin(BlockType type) {
    // backend specific, do nothing
}


void CodegenCVisitor::print_channel_iteration_task_end() {
    // backend specific, do nothing
}


/*
 * Depending on the backend, print loop for tiling channel iterations
 */
void CodegenCVisitor::print_channel_iteration_tiling_block_begin(BlockType type) {
    // no tiling for cpu backend, just get loop bounds
    printer->add_line("int start = 0;");
    printer->add_line("int end = nodecount;");
}


/**
 * End of tiled channel iteration block
 */
void CodegenCVisitor::print_channel_iteration_tiling_block_end() {
    // backend specific, do nothing
}


/**
 * Each kernel like nrn_init, nrn_state and nrn_cur could be offloaded
 * to accelerator. In this case, at very top level, we print pragma
 * for data present. For example:
 *
 * \code{.cpp}
 *  void nrn_state(...) {
 *      #pragma acc data present (nt, ml...)
 *      {
 *
 *      }
 *  }
 *  \endcode
 */
void CodegenCVisitor::print_kernel_data_present_annotation_block_begin() {
    // backend specific, do nothing
}


/**
 * End of print_kernel_enter_data_begin
 */
void CodegenCVisitor::print_kernel_data_present_annotation_block_end() {
    // backend specific, do nothing
}


/**
 * Depending programming model and compiler, we print compiler hint
 * for parallelization. For example:
 *
 *      #pragma ivdep
 *      for(int id = 0; id < nodecount; id++) {
 *
 *      #pragma acc parallel loop
 *      for(int id = 0; id < nodecount; id++) {
 *
 */
void CodegenCVisitor::print_channel_iteration_block_parallel_hint(BlockType type) {
    printer->add_line("#pragma ivdep");
}


/// if reduction block in nrn_cur required
bool CodegenCVisitor::nrn_cur_reduction_loop_required() {
    return channel_task_dependency_enabled() || info.point_process;
}

/// if shadow vectors required
bool CodegenCVisitor::shadow_vector_setup_required() {
    return (channel_task_dependency_enabled() && !codegen_shadow_variables.empty());
}


/*
 * Depending on the backend, print condition/loop for iterating over channels
 *
 * For CPU backend we iterate over all node counts. For cuda we use thread
 * index to check if block needs to be executed or not.
 */
void CodegenCVisitor::print_channel_iteration_block_begin(BlockType type) {
    print_channel_iteration_block_parallel_hint(type);
    printer->start_block("for (int id = start; id < end; id++) ");
}


void CodegenCVisitor::print_channel_iteration_block_end() {
    printer->end_block(1);
}


void CodegenCVisitor::print_rhs_d_shadow_variables() {
    if (info.point_process) {
        printer->add_line("double* shadow_rhs = nt->{};"_format(naming::NTHREAD_RHS_SHADOW));
        printer->add_line("double* shadow_d = nt->{};"_format(naming::NTHREAD_D_SHADOW));
    }
}


void CodegenCVisitor::print_nrn_cur_matrix_shadow_update() {
    if (channel_task_dependency_enabled()) {
        auto rhs = get_variable_name("ml_rhs");
        auto d = get_variable_name("ml_d");
        printer->add_line("{} = rhs;"_format(rhs));
        printer->add_line("{} = g;"_format(d));
    } else {
        if (info.point_process) {
            printer->add_line("shadow_rhs[id] = rhs;");
            printer->add_line("shadow_d[id] = g;");
        } else {
            auto rhs_op = operator_for_rhs();
            auto d_op = operator_for_d();
            print_atomic_reduction_pragma();
            printer->add_line("vec_rhs[node_id] {} rhs;"_format(rhs_op));
            print_atomic_reduction_pragma();
            printer->add_line("vec_d[node_id] {} g;"_format(d_op));
        }
    }
}


void CodegenCVisitor::print_nrn_cur_matrix_shadow_reduction() {
    auto rhs_op = operator_for_rhs();
    auto d_op = operator_for_d();
    if (channel_task_dependency_enabled()) {
        auto rhs = get_variable_name("ml_rhs");
        auto d = get_variable_name("ml_d");
        printer->add_line("int node_id = node_index[id];");
        print_atomic_reduction_pragma();
        printer->add_line("vec_rhs[node_id] {} {};"_format(rhs_op, rhs));
        print_atomic_reduction_pragma();
        printer->add_line("vec_d[node_id] {} {};"_format(d_op, d));
    } else {
        if (info.point_process) {
            printer->add_line("int node_id = node_index[id];");
            print_atomic_reduction_pragma();
            printer->add_line("vec_rhs[node_id] {} shadow_rhs[id];"_format(rhs_op));
            print_atomic_reduction_pragma();
            printer->add_line("vec_d[node_id] {} shadow_d[id];"_format(d_op));
        }
    }
}


void CodegenCVisitor::print_atomic_reduction_pragma() {
    // backend specific, do nothing
}


void CodegenCVisitor::print_shadow_reduction_block_begin() {
    printer->start_block("for (int id = start; id < end; id++) ");
}


void CodegenCVisitor::print_shadow_reduction_statements() {
    for (const auto& statement: shadow_statements) {
        print_atomic_reduction_pragma();
        auto lhs = get_variable_name(statement.lhs);
        auto rhs = get_variable_name(shadow_varname(statement.lhs));
        auto text = "{} {} {};"_format(lhs, statement.op, rhs);
        printer->add_line(text);
    }
    shadow_statements.clear();
}


void CodegenCVisitor::print_shadow_reduction_block_end() {
    printer->end_block(1);
}


void CodegenCVisitor::print_device_method_annotation() {
    // backend specific, nothing for cpu
}


void CodegenCVisitor::print_global_method_annotation() {
    // backend specific, nothing for cpu
}


void CodegenCVisitor::print_backend_namespace_start() {
    // no separate namespace for C (cpu) backend
}


void CodegenCVisitor::print_backend_namespace_stop() {
    // no separate namespace for C (cpu) backend
}


void CodegenCVisitor::print_backend_includes() {
    // backend specific, nothing for cpu
}


std::string CodegenCVisitor::backend_name() {
    return "C (api-compatibility)";
}


bool CodegenCVisitor::block_require_shadow_update(BlockType type) {
    return false;
}


bool CodegenCVisitor::channel_task_dependency_enabled() {
    return false;
}


bool CodegenCVisitor::optimize_ion_variable_copies() {
    return true;
}


void CodegenCVisitor::print_memory_allocation_routine() {
    printer->add_newline(2);
    auto args = "size_t num, size_t size, size_t alignment = 16";
    printer->add_line("static inline void* mem_alloc({}) {}"_format(args, "{"));
    printer->add_line("    void* ptr;");
    printer->add_line("    posix_memalign(&ptr, alignment, num*size);");
    printer->add_line("    memset(ptr, 0, size);");
    printer->add_line("    return ptr;");
    printer->add_line("}");

    printer->add_newline(2);
    printer->add_line("static inline void mem_free(void* ptr) {");
    printer->add_line("    free(ptr);");
    printer->add_line("}");
}


std::string CodegenCVisitor::compute_method_name(BlockType type) {
    if (type == BlockType::Initial) {
        return method_name(naming::NRN_INIT_METHOD);
    }
    if (type == BlockType::State) {
        return method_name(naming::NRN_STATE_METHOD);
    }
    if (type == BlockType::Equation) {
        return method_name(naming::NRN_CUR_METHOD);
    }
    if (type == BlockType::Watch) {
        return method_name(naming::NRN_WATCH_CHECK_METHOD);
    }
    throw std::logic_error("compute_method_name not implemented");
}


// note extra empty space for pretty-printing if we skip the symbol
std::string CodegenCVisitor::ptr_type_qualifier() {
    return "__restrict__ ";
}


std::string CodegenCVisitor::k_const() {
    return "const ";
}


/****************************************************************************************/
/*              printing routines for code generation                                   */
/****************************************************************************************/


void CodegenCVisitor::visit_watch_statement(ast::WatchStatement* node) {
    printer->add_text(
        "nrn_watch_activate(inst, id, pnodecount, {})"_format(current_watch_statement++));
}


void CodegenCVisitor::print_statement_block(ast::StatementBlock* node,
                                            bool open_brace,
                                            bool close_brace) {
    if (open_brace) {
        printer->start_block();
    }

    auto statements = node->get_statements();
    for (const auto& statement: statements) {
        if (statement_to_skip(statement.get())) {
            continue;
        }
        /// not necessary to add indent for verbatim block (pretty-printing)
        if (!statement->is_verbatim()) {
            printer->add_indent();
        }
        statement->accept(this);
        if (need_semicolon(statement.get())) {
            printer->add_text(";");
        }
        printer->add_newline();
    }

    if (close_brace) {
        printer->end_block();
    }
}


void CodegenCVisitor::print_function_call(FunctionCall* node) {
    auto name = node->get_node_name();
    auto function_name = name;
    if (defined_method(name)) {
        function_name = method_name(name);
    }

    if (is_net_send(name)) {
        print_net_send_call(node);
        return;
    }

    if (is_net_move(name)) {
        print_net_move_call(node);
        return;
    }

    if (is_net_event(name)) {
        print_net_event_call(node);
        return;
    }

    auto arguments = node->get_arguments();
    printer->add_text("{}("_format(function_name));

    if (defined_method(name)) {
        printer->add_text(internal_method_arguments());
        if (!arguments.empty()) {
            printer->add_text(", ");
        }
    }

    print_vector_elements(arguments, ", ");
    printer->add_text(")");
}


void CodegenCVisitor::print_top_verbatim_blocks() {
    if (info.top_verbatim_blocks.empty()) {
        return;
    }
    print_namespace_stop();

    printer->add_newline(2);
    printer->add_line("using namespace coreneuron;");
    codegen = true;
    printing_top_verbatim_blocks = true;

    for (const auto& block: info.top_blocks) {
        if (block->is_verbatim()) {
            printer->add_newline(2);
            block->accept(this);
        }
    }

    printing_top_verbatim_blocks = false;
    codegen = false;
    print_namespace_start();
}


/**
 * Rename function arguments that have same name with default inbuilt arguments
 *
 * \todo: issue with verbatim renaming. e.g. pattern.mod has info struct with
 * index variable. If we use "index" instead of "indexes" as default argument
 * then during verbatim replacement we don't know the index is which one. This
 * is because verbatim renaming pass has already stripped out prefixes from
 * the text.
 */
void CodegenCVisitor::rename_function_arguments() {
    auto default_arguments = stringutils::split_string(nrn_thread_arguments(), ',');
    for (auto& arg: default_arguments) {
        stringutils::trim(arg);
        RenameVisitor v(arg, "arg_" + arg);
        for (const auto& function: info.functions) {
            if (has_parameter_of_name(function, arg)) {
                function->accept(&v);
            }
        }
        for (const auto& function: info.procedures) {
            if (has_parameter_of_name(function, arg)) {
                function->accept(&v);
            }
        }
    }
}


void CodegenCVisitor::print_function_prototypes() {
    if (info.functions.empty() && info.procedures.empty()) {
        return;
    }
    codegen = true;
    printer->add_newline(2);
    for (const auto& node: info.functions) {
        print_function_declaration(node, node->get_node_name());
        printer->add_text(";");
        printer->add_newline();
    }
    for (const auto& node: info.procedures) {
        print_function_declaration(node, node->get_node_name());
        printer->add_text(";");
        printer->add_newline();
    }
    codegen = false;
}


static TableStatement* get_table_statement(ast::Block* node) {
    // TableStatementVisitor v;

    AstLookupVisitor v(AstNodeType::TABLE_STATEMENT);
    node->accept(&v);

    auto table_statements = v.get_nodes();

    if (table_statements.size() != 1) {
        auto message =
            "One table statement expected in {} found {}"_format(node->get_node_name(),
                                                                 table_statements.size());
        throw std::runtime_error(message);
    }
    return dynamic_cast<TableStatement*>(table_statements.front().get());
}


void CodegenCVisitor::print_table_check_function(ast::Block* node) {
    auto statement = get_table_statement(node);
    auto table_variables = statement->get_table_vars();
    auto depend_variables = statement->get_depend_vars();
    auto from = statement->get_from();
    auto to = statement->get_to();
    auto name = node->get_node_name();
    auto internal_params = internal_method_parameters();
    auto with = statement->get_with()->eval();
    auto use_table_var = get_variable_name(naming::USE_TABLE_VARIABLE);
    auto tmin_name = get_variable_name("tmin_" + name);
    auto mfac_name = get_variable_name("mfac_" + name);
    auto float_type = default_float_data_type();

    printer->add_newline(2);
    print_device_method_annotation();
    printer->start_block(
        "void check_{}({})"_format(method_name(name), get_parameter_str(internal_params)));
    {
        printer->add_line("if ( {} == 0) {}"_format(use_table_var, "{"));
        printer->add_line("    return;");
        printer->add_line("}");

        printer->add_line("static bool make_table = true;");
        for (const auto& variable: depend_variables) {
            printer->add_line("static {} save_{};"_format(float_type, variable->get_node_name()));
        }

        for (const auto& variable: depend_variables) {
            auto name = variable->get_node_name();
            auto instance_name = get_variable_name(name);
            printer->add_line("if (save_{} != {}) {}"_format(name, instance_name, "{"));
            printer->add_line("    make_table = true;");
            printer->add_line("}");
        }

        printer->start_block("if (make_table)");
        {
            printer->add_line("make_table = false;");

            printer->add_indent();
            printer->add_text("{} = "_format(tmin_name));
            from->accept(this);
            printer->add_text(";");
            printer->add_newline();

            printer->add_indent();
            printer->add_text("double tmax = ");
            to->accept(this);
            printer->add_text(";");
            printer->add_newline();


            printer->add_line("double dx = (tmax-{})/{}.0;"_format(tmin_name, with));
            printer->add_line("{} = 1.0/dx;"_format(mfac_name));

            printer->add_line("int i = 0;");
            printer->add_line("double x = 0;");
            printer->add_line(
                "for(i = 0, x = {}; i < {}; x += dx, i++) {}"_format(tmin_name, with + 1, "{"));
            auto function = method_name("f_" + name);
            printer->add_line("    {}({}, x);"_format(function, internal_method_arguments()));
            for (const auto& variable: table_variables) {
                auto name = variable->get_node_name();
                auto instance_name = get_variable_name(name);
                auto table_name = get_variable_name("t_" + name);
                printer->add_line("    {}[i] = {};"_format(table_name, instance_name));
            }
            printer->add_line("}");

            for (const auto& variable: depend_variables) {
                auto name = variable->get_node_name();
                auto instance_name = get_variable_name(name);
                printer->add_line("save_{} = {};"_format(name, instance_name));
            }
        }
        printer->end_block();
    }
    printer->end_block(1);
}


void CodegenCVisitor::print_table_replacement_function(ast::Block* node) {
    auto name = node->get_node_name();
    auto statement = get_table_statement(node);
    auto table_variables = statement->get_table_vars();
    auto with = statement->get_with()->eval();
    auto use_table_var = get_variable_name(naming::USE_TABLE_VARIABLE);
    auto float_type = default_float_data_type();
    auto tmin_name = get_variable_name("tmin_" + name);
    auto mfac_name = get_variable_name("mfac_" + name);
    auto function_name = method_name("f_" + name);

    printer->add_newline(2);
    print_function_declaration(node, name);
    printer->start_block();
    {
        printer->add_line("if ( {} == 0) {}"_format(use_table_var, "{"));
        printer->add_line("    {}({}, arg_v);"_format(function_name, internal_method_arguments()));
        printer->add_line("     return 0;");
        printer->add_line("}");

        printer->add_line("double xi = {} * (arg_v - {});"_format(mfac_name, tmin_name));
        printer->add_line("if (isnan(xi)) {");
        for (const auto& var: table_variables) {
            auto name = get_variable_name(var->get_node_name());
            printer->add_line("    {} = xi;"_format(name));
        }
        printer->add_line("    return 0;");
        printer->add_line("}");

        printer->add_line("if (xi <= 0.0 || xi >= {}) {}"_format(with, "{"));
        printer->add_line("    int index = (xi <= 0.0) ? 0 : {};"_format(with));
        for (const auto& variable: table_variables) {
            auto name = variable->get_node_name();
            auto instance_name = get_variable_name(name);
            auto table_name = get_variable_name("t_" + name);
            printer->add_line("    {} = {}[index];"_format(instance_name, table_name));
        }
        printer->add_line("    return 0;");
        printer->add_line("}");

        printer->add_line("int i = int(xi);");
        printer->add_line("double theta = xi - double(i);");
        for (const auto& var: table_variables) {
            auto instance_name = get_variable_name(var->get_node_name());
            auto table_name = get_variable_name("t_" + var->get_node_name());
            printer->add_line(
                "{0} = {1}[i] + theta*({1}[i+1]-{1}[i]);"_format(instance_name, table_name));
        }

        printer->add_line("return 0;");
    }
    printer->end_block(1);
}


void CodegenCVisitor::print_check_table_thread_function() {
    if (info.table_count == 0) {
        return;
    }

    printer->add_newline(2);
    auto name = method_name("check_table_thread");
    auto parameters = external_method_parameters(true);

    printer->add_line("static void {} ({}) {}"_format(name, parameters, "{"));
    printer->add_line("    Memb_list* ml = nt->_ml_list[tml_id];");
    printer->add_line("    setup_instance(nt, ml);");
    printer->add_line("    {0}* inst = ({0}*) ml->instance;"_format(instance_struct()));
    printer->add_line("    double v = 0;");
    printer->add_line("    IonCurVar ionvar;");

    for (const auto& function: info.functions_with_table) {
        auto name = method_name("check_" + function->get_node_name());
        auto arguments = internal_method_arguments();
        printer->add_line("    {}({});"_format(name, arguments));
    }

    /// todo : check_table_thread is called multiple times from coreneuron including
    /// after finitialize. If we cleaup the instance then it will result in segfault
    /// but if we don't then there is memory leak
    printer->add_line("    // cleanup_instance(ml);");
    printer->add_line("}");
}


void CodegenCVisitor::print_function_or_procedure(ast::Block* node, std::string& name) {
    printer->add_newline(2);
    print_function_declaration(node, name);
    printer->add_text(" ");
    printer->start_block();

    // function requires return variable declaration
    if (node->is_function_block()) {
        auto type = default_float_data_type();
        printer->add_line("{} ret_{} = 0.0;"_format(type, name));
    } else {
        printer->add_line("int ret_{} = 0;"_format(name));
    }

    print_statement_block(node->get_statement_block().get(), false, false);
    printer->add_line("return ret_{};"_format(name));
    printer->end_block(1);
}


void CodegenCVisitor::print_procedure(ast::ProcedureBlock* node) {
    codegen = true;
    auto name = node->get_node_name();

    if (info.function_uses_table(name)) {
        auto new_name = "f_" + name;
        print_function_or_procedure(node, new_name);
        print_table_check_function(node);
        print_table_replacement_function(node);
    } else {
        print_function_or_procedure(node, name);
    }

    codegen = false;
}


void CodegenCVisitor::print_function(ast::FunctionBlock* node) {
    codegen = true;
    auto name = node->get_node_name();
    auto return_var = "ret_" + name;

    /// first rename return variable name
    auto block = node->get_statement_block().get();
    RenameVisitor v(name, return_var);
    block->accept(&v);

    print_function_or_procedure(node, name);
    codegen = false;
}


void CodegenCVisitor::visit_eigen_newton_solver_block(ast::EigenNewtonSolverBlock* node) {
    // solution vector to store copy of state vars for Newton solver
    printer->add_newline();
    auto float_type = default_float_data_type();
    int N = node->get_n_state_vars()->get_value();
    printer->add_line("Eigen::Matrix<{}, {}, 1> X;"_format(float_type, N));

    print_statement_block(node->get_setup_x_block().get(), false, false);

    // functor that evaluates F(X) and J(X) for Newton solver
    printer->start_block("struct functor");
    printer->add_line("NrnThread* nt;");
    printer->add_line("{0}* inst;"_format(instance_struct()));
    printer->add_line("int id, pnodecount;");
    printer->add_line("double v;");
    printer->add_line("Datum* indexes;");
    if (ion_variable_struct_required()) {
        print_ion_variable();
    }

    print_statement_block(node->get_variable_block().get(), false, false);
    printer->add_newline();

    printer->start_block("void initialize()");
    print_statement_block(node->get_initialize_block().get(), false, false);
    printer->end_block(2);

    printer->add_line(
        "functor(NrnThread* nt, {}* inst, int id, int pnodecount, double v, Datum* indexes) : nt(nt), inst(inst), id(id), pnodecount(pnodecount), v(v), indexes(indexes) {}"_format(
            instance_struct(), "{}"));

    printer->add_indent();
    printer->add_text(
        "void operator()(const Eigen::Matrix<{0}, {1}, 1>& X, Eigen::Matrix<{0}, {1}, "
        "1>& F, "
        "Eigen::Matrix<{0}, {1}, {1}>& Jm) const"_format(float_type, N));
    printer->start_block();
    printer->add_line("{}* J = Jm.data();"_format(float_type));
    print_statement_block(node->get_functor_block().get(), false, false);
    printer->end_block(2);

    // assign newton solver results in matrix X to state vars
    printer->start_block("void finalize()");
    print_statement_block(node->get_finalize_block().get(), false, false);
    printer->end_block(1);

    printer->end_block(0);
    printer->add_text(";");
    printer->add_newline();

    // call newton solver with functor and X matrix that contains state vars
    printer->add_line("// call newton solver");
    printer->add_line("functor newton_functor(nt, inst, id, pnodecount, v, indexes);");
    printer->add_line("newton_functor.initialize();");
    printer->add_line("int newton_iterations = nmodl::newton::newton_solver(X, newton_functor);");

    // assign newton solver results in matrix X to state vars
    print_statement_block(node->get_update_states_block().get(), false, false);
    printer->add_line("newton_functor.finalize();");
}

void CodegenCVisitor::visit_eigen_linear_solver_block(ast::EigenLinearSolverBlock* node) {
    printer->add_newline();
    std::string float_type = default_float_data_type();
    int N = node->get_n_state_vars()->get_value();
    printer->add_line("Eigen::Matrix<{0}, {1}, 1> X, F;"_format(float_type, N));
    printer->add_line("Eigen::Matrix<{0}, {1}, {1}> Jm;"_format(float_type, N));
    printer->add_line("{}* J = Jm.data();"_format(float_type));
    print_statement_block(node->get_variable_block().get(), false, false);
    print_statement_block(node->get_initialize_block().get(), false, false);
    print_statement_block(node->get_setup_x_block().get(), false, false);

    printer->add_newline();
    printer->add_line(
        "X = Eigen::PartialPivLU<Eigen::Ref<Eigen::Matrix<{0}, {1}, {1}>>>(Jm).solve(F);"_format(
            float_type, N));
    print_statement_block(node->get_update_states_block().get(), false, false);
    print_statement_block(node->get_finalize_block().get(), false, false);
}

/****************************************************************************************/
/*                           Code-specific helper routines                              */
/****************************************************************************************/


std::string CodegenCVisitor::internal_method_arguments() {
    if (ion_variable_struct_required()) {
        return "id, pnodecount, inst, ionvar, data, indexes, thread, nt, v";
    }
    return "id, pnodecount, inst, data, indexes, thread, nt, v";
}


std::string CodegenCVisitor::param_type_qualifier() {
    return "";
}


std::string CodegenCVisitor::param_ptr_qualifier() {
    return "";
}


/// @todo: figure out how to correctly handle qualifiers
CodegenCVisitor::ParamVector CodegenCVisitor::internal_method_parameters() {
    auto params = ParamVector();
    params.emplace_back("", "int", "", "id");
    params.emplace_back(param_type_qualifier(), "int", "", "pnodecount");
    params.emplace_back(param_type_qualifier(),
                        "{}*"_format(instance_struct()),
                        param_ptr_qualifier(),
                        "inst");
    if (ion_variable_struct_required()) {
        params.emplace_back("", "IonCurVar&", "", "ionvar");
    }
    params.emplace_back("", "double*", "", "data");
    params.emplace_back(k_const(), "Datum*", "", "indexes");
    params.emplace_back(param_type_qualifier(), "ThreadDatum*", "", "thread");
    params.emplace_back(param_type_qualifier(), "NrnThread*", param_ptr_qualifier(), "nt");
    params.emplace_back("", "double", "", "v");
    return params;
}


std::string CodegenCVisitor::external_method_arguments() {
    return "id, pnodecount, data, indexes, thread, nt, v";
}


std::string CodegenCVisitor::external_method_parameters(bool table) {
    if (table) {
        return "int id, int pnodecount, double* data, Datum* indexes, "
               "ThreadDatum* thread, NrnThread* nt, int tml_id";
    }
    return "int id, int pnodecount, double* data, Datum* indexes, "
           "ThreadDatum* thread, NrnThread* nt, double v";
}


/**
 * Function call arguments when function or procedure is external (i.e.
 * not visible at nmodl level)
 */
std::string CodegenCVisitor::nrn_thread_arguments() {
    if (ion_variable_struct_required()) {
        return "id, pnodecount, ionvar, data, indexes, thread, nt, v";
    }
    return "id, pnodecount, data, indexes, thread, nt, v";
}

/**
 * Function call arguments when function or procedure is defined in the
 * same mod file itself
 */
std::string CodegenCVisitor::nrn_thread_internal_arguments() {
    if (ion_variable_struct_required()) {
        return "id, pnodecount, inst, ionvar, data, indexes, thread, nt, v";
    }
    return "id, pnodecount, inst, data, indexes, thread, nt, v";
}


/**
 * Commonly used variables in the verbatim blocks and their corresponding
 * variable name in the new code generation backend.
 */
std::string CodegenCVisitor::replace_if_verbatim_variable(std::string name) {
    if (naming::VERBATIM_VARIABLES_MAPPING.find(name) != naming::VERBATIM_VARIABLES_MAPPING.end()) {
        name = naming::VERBATIM_VARIABLES_MAPPING.at(name);
    }

    /// if function is defined the same mod file then the arguments must
    /// contain mechanism instance as well.
    if (name == naming::THREAD_ARGS) {
        if (internal_method_call_encountered) {
            name = nrn_thread_internal_arguments();
            internal_method_call_encountered = false;
        } else {
            name = nrn_thread_arguments();
        }
    }
    if (name == naming::THREAD_ARGS_PROTO) {
        name = external_method_parameters();
    }
    return name;
}


/**
 * Processing commonly used constructs in the verbatim blocks.
 * @todo : this is still ad-hoc and requires re-implementation to
 * handle it more elegantly.
 */
std::string CodegenCVisitor::process_verbatim_text(std::string text) {
    parser::CDriver driver;
    driver.scan_string(text);
    auto tokens = driver.all_tokens();
    std::string result;
    for (size_t i = 0; i < tokens.size(); i++) {
        auto token = tokens[i];

        /// check if we have function call in the verbatim block where
        /// function is defined in the same mod file
        if (program_symtab->is_method_defined(token) && tokens[i + 1] == "(") {
            internal_method_call_encountered = true;
        }
        auto name = process_verbatim_token(token);

        if (token == ("_" + naming::TQITEM_VARIABLE)) {
            name = "&" + name;
        }
        if (token == "_STRIDE") {
            name = (layout == LayoutType::soa) ? "pnodecount+id" : "1";
        }
        result += name;
    }
    return result;
}


std::string CodegenCVisitor::register_mechanism_arguments() {
    auto nrn_cur = nrn_cur_required() ? method_name(naming::NRN_CUR_METHOD) : "NULL";
    auto nrn_state = nrn_state_required() ? method_name(naming::NRN_STATE_METHOD) : "NULL";
    auto nrn_alloc = method_name(naming::NRN_ALLOC_METHOD);
    auto nrn_init = method_name(naming::NRN_INIT_METHOD);
    return "mechanism, {}, {}, NULL, {}, {}, first_pointer_var_index()"
           ""_format(nrn_alloc, nrn_cur, nrn_state, nrn_init);
}


std::pair<std::string, std::string> CodegenCVisitor::read_ion_variable_name(std::string name) {
    return {name, "ion_" + name};
}


std::pair<std::string, std::string> CodegenCVisitor::write_ion_variable_name(std::string name) {
    return {"ion_" + name, name};
}


std::string CodegenCVisitor::conc_write_statement(const std::string& ion_name,
                                                  const std::string& concentration,
                                                  int index) {
    auto conc_var_name = get_variable_name("ion_" + concentration);
    auto style_var_name = get_variable_name("style_" + ion_name);
    return "nrn_wrote_conc({}_type,"
           " &({}),"
           " {},"
           " {},"
           " nrn_ion_global_map,"
           " celsius,"
           " nt->_ml_list[{}_type]->_nodecount_padded)"
           ""_format(ion_name, conc_var_name, index, style_var_name, ion_name);
}


/**
 * If mechanisms dependency level execution is enabled then certain updates
 * like ionic current contributions needs to be atomically updated. In this
 * case we first update current mechanism's shadow vector and then add statement
 * to queue that will be used in reduction queue.
 *
 * @param statement Statement that might require reduction
 * @param type Type of the block
 * @return Original statement is reduction requires otherwise original statement
 */
std::string CodegenCVisitor::process_shadow_update_statement(ShadowUseStatement& statement,
                                                             BlockType type) {
    /// when there is no operator or rhs then that statement doesn't need shadow update
    if (statement.op.empty() && statement.rhs.empty()) {
        auto text = statement.lhs + ";";
        return text;
    }

    /// blocks like initial doesn't use shadow update (e.g. due to wrote_conc call)
    if (block_require_shadow_update(type)) {
        shadow_statements.push_back(statement);
        auto lhs = get_variable_name(shadow_varname(statement.lhs));
        auto rhs = statement.rhs;
        auto text = "{} = {};"_format(lhs, rhs);
        return text;
    }

    /// return regular statement
    auto lhs = get_variable_name(statement.lhs);
    auto text = "{} {} {};"_format(lhs, statement.op, statement.rhs);
    return text;
}


/****************************************************************************************/
/*               Code-specific printing routines for code generation                    */
/****************************************************************************************/


/**
 * NMODL constants from unit database
 *
 * todo : this should be replaced with constant handling from unit database
 */

void CodegenCVisitor::print_nmodl_constant() {
    printer->add_newline(2);
    printer->add_line("/** constants used in nmodl */");
    printer->add_line("static const double FARADAY = 96485.3;");
    printer->add_line("static const double PI = 3.14159;");
    printer->add_line("static const double R = 8.3145;");
}


void CodegenCVisitor::print_memory_layout_getter() {
    printer->add_newline(2);
    printer->add_line("static inline int get_memory_layout() {");
    if (layout == LayoutType::aos) {
        printer->add_line("    return 1;  //aos");
    } else {
        printer->add_line("    return 0;  //soa");
    }
    printer->add_line("}");
}


void CodegenCVisitor::print_first_pointer_var_index_getter() {
    printer->add_newline(2);
    print_device_method_annotation();
    printer->add_line("static inline int first_pointer_var_index() {");
    printer->add_line("    return {};"_format(info.first_pointer_var_index));
    printer->add_line("}");
}


void CodegenCVisitor::print_num_variable_getter() {
    printer->add_newline(2);
    print_device_method_annotation();
    printer->add_line("static inline int float_variables_size() {");
    printer->add_line("    return {};"_format(float_variables_size()));
    printer->add_line("}");

    printer->add_newline(2);
    print_device_method_annotation();
    printer->add_line("static inline int int_variables_size() {");
    printer->add_line("    return {};"_format(int_variables_size()));
    printer->add_line("}");
}


void CodegenCVisitor::print_net_receive_arg_size_getter() {
    if (!net_receive_exist()) {
        return;
    }
    printer->add_newline(2);
    print_device_method_annotation();
    printer->add_line("static inline int num_net_receive_args() {");
    printer->add_line("    return {};"_format(info.num_net_receive_parameters));
    printer->add_line("}");
}


void CodegenCVisitor::print_mech_type_getter() {
    printer->add_newline(2);
    print_device_method_annotation();
    printer->add_line("static inline int get_mech_type() {");
    printer->add_line("    return {};"_format(get_variable_name("mech_type")));
    printer->add_line("}");
}


void CodegenCVisitor::print_memb_list_getter() {
    printer->add_newline(2);
    print_device_method_annotation();
    printer->add_line("static inline Memb_list* get_memb_list(NrnThread* nt) {");
    printer->add_line("    if (nt->_ml_list == NULL) {");
    printer->add_line("        return NULL;");
    printer->add_line("    }");
    printer->add_line("    return nt->_ml_list[get_mech_type()];");
    printer->add_line("}");
}


void CodegenCVisitor::print_post_channel_iteration_common_code() {
    if (layout == LayoutType::aos) {
        printer->add_line("data = ml->data + id*{};"_format(float_variables_size()));
        printer->add_line("indexes = ml->pdata + id*{};"_format(int_variables_size()));
    }
}


void CodegenCVisitor::print_namespace_start() {
    printer->add_newline(2);
    printer->start_block("namespace coreneuron");
}


void CodegenCVisitor::print_namespace_stop() {
    printer->end_block(1);
}


/**
 * Print getter methods used for accessing thread variables
 *
 * There are three types of thread variables currently considered:
 *      - top local thread variables
 *      - thread variables in the mod file
 *      - thread variables for solver
 *
 * These variables are allocated into different thread structures and have
 * corresponding thread ids. Thread id start from 0. In mod2c implementation,
 * thread_data_index is increased at various places and it is used to
 * decide the index of thread.
 */

void CodegenCVisitor::print_thread_getters() {
    if (info.vectorize && info.derivimplicit_coreneuron_solver()) {
        int tid = info.derivimplicit_var_thread_id;
        int list = info.derivimplicit_list_num;

        // clang-format off
        printer->add_newline(2);
        printer->add_line("/** thread specific helper routines for derivimplicit */");

        printer->add_newline(1);
        printer->add_line("static inline int* deriv{}_advance(ThreadDatum* thread) {}"_format(list, "{"));
        printer->add_line("    return &(thread[{}].i);"_format(tid));
        printer->add_line("}");

        printer->add_newline(1);
        printer->add_line("static inline int dith{}() {}"_format(list, "{"));
        printer->add_line("    return {};"_format(tid+1));
        printer->add_line("}");

        printer->add_newline(1);
        printer->add_line("static inline void** newtonspace{}(ThreadDatum* thread) {}"_format(list, "{"));
        printer->add_line("    return &(thread[{}]._pvoid);"_format(tid+2));
        printer->add_line("}");
    }

    if (info.vectorize && !info.thread_variables.empty()) {
        printer->add_newline(2);
        printer->add_line("/** tid for thread variables */");
        printer->add_line("static inline int thread_var_tid() {");
        printer->add_line("    return {};"_format(info.thread_var_thread_id));
        printer->add_line("}");
    }

    if (info.vectorize && !info.top_local_variables.empty()) {
        printer->add_newline(2);
        printer->add_line("/** tid for top local tread variables */");
        printer->add_line("static inline int top_local_var_tid() {");
        printer->add_line("    return {};"_format(info.top_local_thread_id));
        printer->add_line("}");
    }
    // clang-format on
}


/****************************************************************************************/
/*                         Routines for returning variable name                         */
/****************************************************************************************/


std::string CodegenCVisitor::float_variable_name(SymbolType& symbol, bool use_instance) {
    auto name = symbol->get_name();
    auto dimension = symbol->get_length();
    auto num_float = float_variables_size();
    auto position = position_of_float_var(name);
    // clang-format off
    if (symbol->is_array()) {
        if (use_instance) {
            auto stride = (layout == LayoutType::soa) ? dimension : num_float;
            return "(inst->{}+id*{})"_format(name, stride);
        }
        auto stride = (layout == LayoutType::soa) ? "{}*pnodecount+id*{}"_format(position, dimension) : "{}"_format(position);
        return "(data+{})"_format(stride);
    }
    if (use_instance) {
        auto stride = (layout == LayoutType::soa) ? "id" : "id*{}"_format(num_float);
        return "inst->{}[{}]"_format(name, stride);
    }
    auto stride = (layout == LayoutType::soa) ? "{}*pnodecount+id"_format(position) : "{}"_format(position);
    return "data[{}]"_format(stride);
    // clang-format on
}


std::string CodegenCVisitor::int_variable_name(IndexVariableInfo& symbol,
                                               const std::string& name,
                                               bool use_instance) {
    auto position = position_of_int_var(name);
    auto num_int = int_variables_size();
    std::string offset;
    // clang-format off
    if (symbol.is_index) {
        offset = std::to_string(position);
        if (use_instance) {
            return "inst->{}[{}]"_format(name, offset);
        }
        return "indexes[{}]"_format(offset);
    }
    if (symbol.is_integer) {
        if (use_instance) {
            offset = (layout == LayoutType::soa) ? "{}*pnodecount+id"_format(position) : "id*{}+{}"_format(num_int, position);
            return "inst->{}[{}]"_format(name, offset);
        }
        offset = (layout == LayoutType::soa) ? "{}*pnodecount+id"_format(position) : "id";
        return "indexes[{}]"_format(offset);
    }
    offset = (layout == LayoutType::soa) ? "{}*pnodecount+id"_format(position) : "{}"_format(position);
    if (use_instance) {
        return "inst->{}[indexes[{}]]"_format(name, offset);
    }
    auto data = symbol.is_vdata ? "_vdata" : "_data";
    return "nt->{}[indexes[{}]]"_format(data, offset);
    // clang-format on
}


std::string CodegenCVisitor::global_variable_name(SymbolType& symbol) {
    return "{}_global.{}"_format(info.mod_suffix, symbol->get_name());
}


std::string CodegenCVisitor::ion_shadow_variable_name(SymbolType& symbol) {
    return "inst->{}[id]"_format(symbol->get_name());
}


std::string CodegenCVisitor::update_if_ion_variable_name(const std::string& name) {
    std::string result(name);
    if (ion_variable_struct_required()) {
        if (info.is_ion_read_variable(name)) {
            result = "ion_" + name;
        }
        if (info.is_ion_write_variable(name)) {
            result = "ionvar." + name;
        }
        if (info.is_current(name)) {
            result = "ionvar." + name;
        }
    }
    return result;
}


/**
 * Return variable name in the structure of mechanism properties
 *
 * @param name variable name that is being printed
 * @param use_instance if variable name should be with the instance object qualifier
 * @return use_instance whether print name using Instance structure (or data array if false)
 */
std::string CodegenCVisitor::get_variable_name(const std::string& name, bool use_instance) {
    std::string varname = update_if_ion_variable_name(name);

    // clang-format off
    auto symbol_comparator = [&varname](const SymbolType& sym) {
                            return varname == sym->get_name();
                         };

    auto index_comparator = [&varname](const IndexVariableInfo& var) {
                            return varname == var.symbol->get_name();
                         };
    // clang-format on

    /// float variable
    auto f = std::find_if(codegen_float_variables.begin(),
                          codegen_float_variables.end(),
                          symbol_comparator);
    if (f != codegen_float_variables.end()) {
        return float_variable_name(*f, use_instance);
    }

    /// integer variable
    auto i =
        std::find_if(codegen_int_variables.begin(), codegen_int_variables.end(), index_comparator);
    if (i != codegen_int_variables.end()) {
        return int_variable_name(*i, varname, use_instance);
    }

    /// global variable
    auto g = std::find_if(codegen_global_variables.begin(),
                          codegen_global_variables.end(),
                          symbol_comparator);
    if (g != codegen_global_variables.end()) {
        return global_variable_name(*g);
    }

    /// shadow variable
    auto s = std::find_if(codegen_shadow_variables.begin(),
                          codegen_shadow_variables.end(),
                          symbol_comparator);
    if (s != codegen_shadow_variables.end()) {
        return ion_shadow_variable_name(*s);
    }

    if (varname == naming::NTHREAD_DT_VARIABLE) {
        return "nt->_" + naming::NTHREAD_DT_VARIABLE;
    }

    /// t in net_receive method is an argument to function and hence it should
    /// ne used instead of nt->_t which is current time of thread
    if (varname == naming::NTHREAD_T_VARIABLE && !printing_net_receive) {
        return "nt->_" + naming::NTHREAD_T_VARIABLE;
    }

    /// otherwise return original name
    return varname;
}


/****************************************************************************************/
/*                      Main printing routines for code generation                      */
/****************************************************************************************/


void CodegenCVisitor::print_backend_info() {
    time_t tr;
    time(&tr);
    auto date = std::string(asctime(localtime(&tr)));
    auto version = nmodl::Version::NMODL_VERSION + " [" + nmodl::Version::GIT_REVISION + "]";

    printer->add_line("/*********************************************************");
    printer->add_line("Model Name      : {}"_format(info.mod_suffix));
    printer->add_line("Filename        : {}"_format(info.mod_file + ".mod"));
    printer->add_line("NMODL Version   : {}"_format(nmodl_version()));
    printer->add_line("Vectorized      : {}"_format(info.vectorize));
    printer->add_line("Threadsafe      : {}"_format(info.thread_safe));
    printer->add_line("Created         : {}"_format(stringutils::trim(date)));
    printer->add_line("Backend         : {}"_format(backend_name()));
    printer->add_line("NMODL Compiler  : {}"_format(version));
    printer->add_line("*********************************************************/");
}


void CodegenCVisitor::print_standard_includes() {
    printer->add_newline();
    printer->add_line("#include <math.h>");
    printer->add_line("#include <stdio.h>");
    printer->add_line("#include <stdlib.h>");
    printer->add_line("#include <string.h>");
}


void CodegenCVisitor::print_coreneuron_includes() {
    printer->add_newline();
    printer->add_line("#include <coreneuron/mech/cfile/scoplib.h>");
    printer->add_line("#include <coreneuron/nrnconf.h>");
    printer->add_line("#include <coreneuron/nrnoc/multicore.h>");
    printer->add_line("#include <coreneuron/nrnoc/register_mech.hpp>");
    printer->add_line("#include <coreneuron/nrniv/nrn_acc_manager.h>");
    printer->add_line("#include <coreneuron/utils/randoms/nrnran123.h>");
    printer->add_line("#include <coreneuron/nrniv/nrniv_decl.h>");
    printer->add_line("#include <coreneuron/nrniv/ivocvect.h>");
    printer->add_line("#include <coreneuron/mech/mod2c_core_thread.h>");
    printer->add_line("#include <coreneuron/scopmath_core/newton_struct.h>");
    printer->add_line("#include <_kinderiv.h>");
    if (info.eigen_newton_solver_exist) {
        printer->add_line("#include <newton/newton.hpp>");
    }
    if (info.eigen_linear_solver_exist) {
        printer->add_line("#include <Eigen/LU>");
    }
}


/**
 * Print all static variables at file scope
 *
 * Variables required for type of ion, type of point process etc. are
 * of static int type. For any backend type (C,C++), it's ok to have
 * these variables as file scoped static variables.
 *
 * Initial values of state variables (h0) are also defined as static
 * variables. Note that the state could be ion variable and it could
 * be also range variable. Hence lookup into symbol table before.
 *
 * When model is not vectorized (shouldn't be the case in coreneuron)
 * the top local variables become static variables.
 *
 * Note that static variables are already initialized to 0. We do the
 * same for some variables to keep same code as neuron.
 */
void CodegenCVisitor::print_mechanism_global_var_structure(bool wrapper) {
    auto float_type = default_float_data_type();
    printer->add_newline(2);
    printer->add_line("/** all global variables */");
    printer->add_line("struct {} {}"_format(global_struct(), "{"));
    printer->increase_indent();

    if (!info.ions.empty()) {
        for (const auto& ion: info.ions) {
            auto name = "{}_type"_format(ion.name);
            printer->add_line("int {};"_format(name));
            codegen_global_variables.push_back(make_symbol(name));
        }
    }

    if (info.point_process) {
        printer->add_line("int point_type;");
        codegen_global_variables.push_back(make_symbol("point_type"));
    }

    if (!info.state_vars.empty()) {
        for (const auto& var: info.state_vars) {
            auto name = var->get_name() + "0";
            auto symbol = program_symtab->lookup(name);
            if (symbol == nullptr) {
                printer->add_line("{} {};"_format(float_type, name));
                codegen_global_variables.push_back(make_symbol(name));
            }
        }
    }

    /// Neuron and Coreneuron adds "v" to global variables when vectorize
    /// is false. But as v is always local variable and passed as argument,
    /// we don't need to use global variable v

    auto& top_locals = info.top_local_variables;
    if (!info.vectorize && !top_locals.empty()) {
        for (const auto& var: top_locals) {
            auto name = var->get_name();
            auto length = var->get_length();
            if (var->is_array()) {
                printer->add_line("{} {}[{}];"_format(float_type, name, length));
            } else {
                printer->add_line("{} {};"_format(float_type, name));
            }
            codegen_global_variables.push_back(var);
        }
    }

    if (!info.thread_variables.empty()) {
        printer->add_line("int thread_data_in_use;");
        printer->add_line("{} thread_data[{}];"_format(float_type, info.thread_var_data_size));
        codegen_global_variables.push_back(make_symbol("thread_data_in_use"));
        auto symbol = make_symbol("thread_data");
        symbol->set_as_array(info.thread_var_data_size);
        codegen_global_variables.push_back(symbol);
    }

    printer->add_line("int reset;");
    codegen_global_variables.push_back(make_symbol("reset"));

    printer->add_line("int mech_type;");
    codegen_global_variables.push_back(make_symbol("mech_type"));

    auto& globals = info.global_variables;
    auto& constants = info.constant_variables;

    if (!globals.empty()) {
        for (const auto& var: globals) {
            auto name = var->get_name();
            auto length = var->get_length();
            if (var->is_array()) {
                printer->add_line("{} {}[{}];"_format(float_type, name, length));
            } else {
                printer->add_line("{} {};"_format(float_type, name));
            }
            codegen_global_variables.push_back(var);
        }
    }

    if (!constants.empty()) {
        for (const auto& var: constants) {
            auto name = var->get_name();
            auto value_ptr = var->get_value();
            printer->add_line("{} {};"_format(float_type, name));
            codegen_global_variables.push_back(var);
        }
    }

    if (info.primes_size != 0) {
        printer->add_line("int* slist1;");
        printer->add_line("int* dlist1;");
        codegen_global_variables.push_back(make_symbol("slist1"));
        codegen_global_variables.push_back(make_symbol("dlist1"));
        if (info.derivimplicit_coreneuron_solver()) {
            printer->add_line("int* slist2;");
            codegen_global_variables.push_back(make_symbol("slist2"));
        }
    }

    if (info.table_count > 0) {
        printer->add_line("double usetable;");
        codegen_global_variables.push_back(make_symbol(naming::USE_TABLE_VARIABLE));

        for (const auto& block: info.functions_with_table) {
            auto name = block->get_node_name();
            printer->add_line("{} tmin_{};"_format(float_type, name));
            printer->add_line("{} mfac_{};"_format(float_type, name));
            codegen_global_variables.push_back(make_symbol("tmin_" + name));
            codegen_global_variables.push_back(make_symbol("mfac_" + name));
        }

        for (const auto& variable: info.table_statement_variables) {
            auto name = "t_" + variable->get_name();
            printer->add_line("{}* {};"_format(float_type, name));
            codegen_global_variables.push_back(make_symbol(name));
        }
    }

    if (info.vectorize) {
        printer->add_line("ThreadDatum* {}ext_call_thread;"_format(ptr_type_qualifier()));
        codegen_global_variables.push_back(make_symbol("ext_call_thread"));
    }

    printer->decrease_indent();
    printer->add_line("};");

    printer->add_newline(1);
    printer->add_line("/** holds object of global variable */");
    printer->add_line("{} {}_global;"_format(global_struct(), info.mod_suffix));

    /// create copy on the device
    print_global_variable_device_create_annotation();
}


void CodegenCVisitor::print_mechanism_info() {
    auto variable_printer = [&](std::vector<SymbolType>& variables) {
        for (const auto& v: variables) {
            auto name = v->get_name();
            if (!info.point_process) {
                name += "_" + info.mod_suffix;
            }
            if (v->is_array()) {
                name += "[{}]"_format(v->get_length());
            }
            printer->add_line(add_escape_quote(name) + ",");
        }
    };

    printer->add_newline(2);
    printer->add_line("/** channel information */");
    printer->add_line("static const char *mechanism[] = {");
    printer->increase_indent();
    printer->add_line(add_escape_quote(nmodl_version()) + ",");
    printer->add_line(add_escape_quote(info.mod_suffix) + ",");
    variable_printer(info.range_parameter_vars);
    printer->add_line("0,");
    variable_printer(info.range_dependent_vars);
    printer->add_line("0,");
    variable_printer(info.state_vars);
    printer->add_line("0,");
    variable_printer(info.pointer_variables);
    printer->add_line("0");
    printer->decrease_indent();
    printer->add_line("};");
}


/**
 * Print structs that encapsulate information about scalar and
 * vector elements of type global and thread variables.
 */
void CodegenCVisitor::print_global_variables_for_hoc() {
    auto variable_printer =
        [&](const std::vector<SymbolType>& variables, bool if_array, bool if_vector) {
            for (const auto& variable: variables) {
                if (variable->is_array() == if_array) {
                    auto name = get_variable_name(variable->get_name());
                    auto ename = add_escape_quote(variable->get_name() + "_" + info.mod_suffix);
                    auto length = variable->get_length();
                    if (if_vector) {
                        printer->add_line("{}, {}, {},"_format(ename, name, length));
                    } else {
                        printer->add_line("{}, &{},"_format(ename, name));
                    }
                }
            }
        };

    auto globals = info.global_variables;
    auto thread_vars = info.thread_variables;

    if (info.table_count > 0) {
        globals.push_back(make_symbol(naming::USE_TABLE_VARIABLE));
    }

    printer->add_newline(2);
    printer->add_line("/** connect global (scalar) variables to hoc -- */");
    printer->add_line("static DoubScal hoc_scalar_double[] = {");
    printer->increase_indent();
    variable_printer(globals, false, false);
    variable_printer(thread_vars, false, false);
    printer->add_line("0, 0");
    printer->decrease_indent();
    printer->add_line("};");

    printer->add_newline(2);
    printer->add_line("/** connect global (array) variables to hoc -- */");
    printer->add_line("static DoubVec hoc_vector_double[] = {");
    printer->increase_indent();
    variable_printer(globals, true, true);
    variable_printer(thread_vars, true, true);
    printer->add_line("0, 0, 0");
    printer->decrease_indent();
    printer->add_line("};");
}


/**
 * Print register function for mechanisms
 *
 * Every mod file has register function to connect with the simulator.
 * Various information about mechanism and callbacks get registered with
 * the simulator using suffix_reg() function.
 *
 * Here are details:
 *  - setup_global_variables function used to create vectors necessary for specific
 *    solvers like euler and derivimplicit. All global variables are initialized as well.
 *    We should exclude that callback based on the solver, watch statements.
 *  - If nrn_get_mechtype is < -1 means that mechanism is not used in the
 *    context of neuron execution and hence could be ignored in coreneuron
 *    execution.
 *  - Each mechanism could have different layout and hence we register the
 *    layout with the simulator. In practice all mechanisms have same layout.
 *  - Ions are internally defined and their types can be queried similar to
 *    other mechanisms.
 *  - hoc_register_var may not be needed in the context of coreneuron
 *  - We assume net receive buffer is on. This is because generated code is
 *    compatible for cpu as well as gpu target.
 */
void CodegenCVisitor::print_mechanism_register() {
    printer->add_newline(2);
    printer->add_line("/** register channel with the simulator */");
    printer->start_block("void _{}_reg() "_format(info.mod_file));

    /// allocate global variables
    printer->add_line("setup_global_variables();");

    /// type related information
    auto mech_type = get_variable_name("mech_type");
    auto suffix = add_escape_quote(info.mod_suffix);
    printer->add_newline();
    printer->add_line("int mech_type = nrn_get_mechtype({});"_format(suffix));
    printer->add_line("{} = mech_type;"_format(mech_type));
    printer->add_line("if (mech_type == -1) {");
    printer->add_line("    return;");
    printer->add_line("}");

    printer->add_newline();
    printer->add_line("_nrn_layout_reg(mech_type, get_memory_layout());");

    /// register mechanism
    auto args = register_mechanism_arguments();
    auto nobjects = num_thread_objects();
    if (info.point_process) {
        printer->add_line("point_register_mech({}, NULL, NULL, {});"_format(args, nobjects));
    } else {
        printer->add_line("register_mech({}, {});"_format(args, nobjects));
    }

    /// types for ion
    for (const auto& ion: info.ions) {
        auto type = get_variable_name(ion.name + "_type");
        auto name = add_escape_quote(ion.name + "_ion");
        printer->add_line(type + " = nrn_get_mechtype(" + name + ");");
    }
    printer->add_newline();

    /**
     *  If threads are used then memory is allocated in setup_global_variables.
     *  Register callbacks for thread allocation and cleanup. Note that thread_data_index
     *  represent total number of thread used minus 1 (i.e. index of last thread).
     */
    if (info.vectorize && (info.thread_data_index != 0)) {
        auto name = get_variable_name("ext_call_thread");
        printer->add_line("thread_mem_init({});"_format(name));
    }

    if (!info.thread_variables.empty()) {
        printer->add_line("{} = 0;"_format(get_variable_name("thread_data_in_use")));
    }

    if (info.thread_callback_register) {
        printer->add_line("_nrn_thread_reg0(mech_type, thread_mem_cleanup);");
        printer->add_line("_nrn_thread_reg1(mech_type, thread_mem_init);");
    }

    if (info.emit_table_thread()) {
        auto name = method_name("check_table_thread");
        printer->add_line("_nrn_thread_table_reg(mech_type, {});"_format(name));
    }

    /// register read/write callbacks for pointers
    if (info.bbcore_pointer_used) {
        printer->add_line("hoc_reg_bbcore_read(mech_type, bbcore_read);");
        printer->add_line("hoc_reg_bbcore_write(mech_type, bbcore_write);");
    }

    /// register size of double and int elements
    // clang-format off
    printer->add_line("hoc_register_prop_size(mech_type, float_variables_size(), int_variables_size());");
    // clang-format on

    /// register semantics for index variables
    for (auto& semantic: info.semantics) {
        auto args = "mech_type, {}, {}"_format(semantic.index, add_escape_quote(semantic.name));
        printer->add_line("hoc_register_dparam_semantics({});"_format(args));
    }

    if (info.write_concentration) {
        printer->add_line("nrn_writes_conc(mech_type, 0);");
    }

    /// register various information for point process type
    if (info.net_event_used) {
        printer->add_line("add_nrn_has_net_event(mech_type);");
    }
    if (info.artificial_cell) {
        printer->add_line("add_nrn_artcell(mech_type, {});"_format(info.tqitem_index));
    }
    if (net_receive_buffering_required()) {
        printer->add_line("hoc_register_net_receive_buffering({}, mech_type);"_format(
            method_name("net_buf_receive")));
    }
    if (info.num_net_receive_parameters != 0) {
        printer->add_line("pnt_receive[mech_type] = {};"_format(method_name("net_receive")));
        printer->add_line("pnt_receive_size[mech_type] = num_net_receive_args();");
        if (info.net_receive_initial_node != nullptr) {
            printer->add_line("pnt_receive_init[mech_type] = net_init;");
        }
    }
    if (info.net_event_used || info.net_send_used) {
        printer->add_line("hoc_register_net_send_buffering(mech_type);");
    }

    /// register variables for hoc
    printer->add_line("hoc_register_var(hoc_scalar_double, hoc_vector_double, NULL);");
    printer->end_block(1);
}


void CodegenCVisitor::print_thread_memory_callbacks() {
    if (!info.thread_callback_register) {
        return;
    }

    /// thread_mem_init callback
    printer->add_newline(2);
    printer->add_line("/** thread memory allocation callback */");
    printer->start_block("static void thread_mem_init(ThreadDatum* thread) ");

    if (info.vectorize && info.derivimplicit_coreneuron_solver()) {
        printer->add_line("thread[dith{}()].pval = NULL;"_format(info.derivimplicit_list_num));
    }
    if (info.vectorize && (info.top_local_thread_size != 0)) {
        auto length = info.top_local_thread_size;
        auto allocation = "(double*)mem_alloc({}, sizeof(double))"_format(length);
        auto line = "thread[top_local_var_tid()].pval = {};"_format(allocation);
        printer->add_line(line);
    }
    if (info.thread_var_data_size != 0) {
        auto length = info.thread_var_data_size;
        auto thread_data = get_variable_name("thread_data");
        auto thread_data_in_use = get_variable_name("thread_data_in_use");
        auto allocation = "(double*)mem_alloc({}, sizeof(double))"_format(length);
        printer->add_line("if ({}) {}"_format(thread_data_in_use, "{"));
        printer->add_line("    thread[thread_var_tid()].pval = {};"_format(allocation));
        printer->add_line("} else {");
        printer->add_line("    thread[thread_var_tid()].pval = {};"_format(thread_data));
        printer->add_line("    {} = 1;"_format(thread_data_in_use));
        printer->add_line("}");
    }
    printer->end_block(3);


    /// thread_mem_cleanup callback
    printer->add_line("/** thread memory cleanup callback */");
    printer->start_block("static void thread_mem_cleanup(ThreadDatum* thread) ");

    // clang-format off
    if (info.vectorize && info.derivimplicit_coreneuron_solver()) {
        int n = info.derivimplicit_list_num;
        printer->add_line("free(thread[dith{}()].pval);"_format(n));
        printer->add_line("nrn_destroy_newtonspace(static_cast<NewtonSpace*>(*newtonspace{}(thread)));"_format(n));
    }
    // clang-format on

    if (info.top_local_thread_size != 0) {
        auto line = "free(thread[top_local_var_tid()].pval);";
        printer->add_line(line);
    }
    if (info.thread_var_data_size != 0) {
        auto thread_data = get_variable_name("thread_data");
        auto thread_data_in_use = get_variable_name("thread_data_in_use");
        printer->add_line("if (thread[thread_var_tid()].pval == {}) {}"_format(thread_data, "{"));
        printer->add_line("    {} = 0;"_format(thread_data_in_use));
        printer->add_line("} else {");
        printer->add_line("    free(thread[thread_var_tid()].pval);");
        printer->add_line("}");
    }
    printer->end_block(1);
}


void CodegenCVisitor::print_mechanism_range_var_structure() {
    auto float_type = default_float_data_type();
    auto int_type = default_int_data_type();
    printer->add_newline(2);
    printer->add_line("/** all mechanism instance variables */");
    printer->start_block("struct {} "_format(instance_struct()));
    for (auto& var: codegen_float_variables) {
        auto name = var->get_name();
        auto type = get_range_var_float_type(var);
        auto qualifier = is_constant_variable(name) ? k_const() : "";
        printer->add_line("{}{}* {}{};"_format(qualifier, type, ptr_type_qualifier(), name));
    }
    for (auto& var: codegen_int_variables) {
        auto name = var.symbol->get_name();
        if (var.is_index || var.is_integer) {
            auto qualifier = var.is_constant ? k_const() : "";
            printer->add_line(
                "{}{}* {}{};"_format(qualifier, int_type, ptr_type_qualifier(), name));
        } else {
            auto qualifier = var.is_constant ? k_const() : "";
            auto type = var.is_vdata ? "void*" : default_float_data_type();
            printer->add_line("{}{}* {}{};"_format(qualifier, type, ptr_type_qualifier(), name));
        }
    }
    if (channel_task_dependency_enabled()) {
        for (auto& var: codegen_shadow_variables) {
            auto name = var->get_name();
            printer->add_line("{}* {}{};"_format(float_type, ptr_type_qualifier(), name));
        }
    }
    printer->end_block();
    printer->add_text(";");
    printer->add_newline();
}


void CodegenCVisitor::print_ion_var_structure() {
    if (!ion_variable_struct_required()) {
        return;
    }
    printer->add_newline(2);
    printer->add_line("/** ion write variables */");
    printer->start_block("struct IonCurVar");

    std::string float_type = default_float_data_type();
    std::vector<std::string> members;

    for (auto& ion: info.ions) {
        for (auto& var: ion.writes) {
            printer->add_line("{} {};"_format(float_type, var));
            members.push_back(var);
        }
    }
    for (auto& var: info.currents) {
        if (!info.is_ion_variable(var)) {
            printer->add_line("{} {};"_format(float_type, var));
            members.push_back(var);
        }
    }

    print_ion_var_constructor(members);

    printer->end_block();
    printer->add_text(";");
    printer->add_newline();
}


void CodegenCVisitor::print_ion_var_constructor(const std::vector<std::string>& members) {
    /// constructor
    printer->add_newline();
    printer->add_line("IonCurVar() : ", 0);
    for (int i = 0; i < members.size(); i++) {
        printer->add_text("{}(0)"_format(members[i]));
        if (i + 1 < members.size()) {
            printer->add_text(", ");
        }
    }
    printer->add_text(" {}");
    printer->add_newline();
}


void CodegenCVisitor::print_ion_variable() {
    printer->add_line("IonCurVar ionvar;");
}


void CodegenCVisitor::print_global_variable_device_create_annotation() {
    // nothing for cpu
}


void CodegenCVisitor::print_global_variable_device_update_annotation() {
    // nothing for cpu
}


void CodegenCVisitor::print_global_variable_setup() {
    std::vector<std::string> allocated_variables;

    printer->add_newline(2);
    printer->add_line("/** initialize global variables */");
    printer->start_block("static inline void setup_global_variables() ");

    printer->add_line("static int setup_done = 0;");
    printer->add_line("if (setup_done) {");
    printer->add_line("    return;");
    printer->add_line("}");

    /// offsets for state variables
    if (info.primes_size != 0) {
        auto slist1 = get_variable_name("slist1");
        auto dlist1 = get_variable_name("dlist1");
        auto n = info.primes_size;
        printer->add_line("{} = (int*) mem_alloc({}, sizeof(int));"_format(slist1, n));
        printer->add_line("{} = (int*) mem_alloc({}, sizeof(int));"_format(dlist1, n));
        allocated_variables.push_back(slist1);
        allocated_variables.push_back(dlist1);

        int id = 0;
        for (auto& prime: info.prime_variables_by_order) {
            auto name = prime->get_name();
            printer->add_line("{}[{}] = {};"_format(slist1, id, position_of_float_var(name)));
            printer->add_line("{}[{}] = {};"_format(dlist1, id, position_of_float_var("D" + name)));
            id++;
        }
    }

    /// additional list for derivimplicit method
    if (info.derivimplicit_coreneuron_solver()) {
        auto primes = program_symtab->get_variables_with_properties(NmodlType::prime_name);
        auto slist2 = get_variable_name("slist2");
        auto nprimes = info.primes_size;
        printer->add_line("{} = (int*) mem_alloc({}, sizeof(int));"_format(slist2, nprimes));
        int id = 0;
        for (auto& variable: primes) {
            auto name = variable->get_name();
            printer->add_line("{}[{}] = {};"_format(slist2, id, position_of_float_var(name)));
            id++;
        }
        allocated_variables.push_back(slist2);
    }

    /// memory for thread member
    if (info.vectorize && (info.thread_data_index != 0)) {
        auto n = info.thread_data_index;
        auto alloc = "(ThreadDatum*) mem_alloc({}, sizeof(ThreadDatum))"_format(n);
        auto name = get_variable_name("ext_call_thread");
        printer->add_line("{} = {};"_format(name, alloc));
        allocated_variables.push_back(name);
    }

    /// initialize global variables
    for (auto& var: info.state_vars) {
        auto name = var->get_name() + "0";
        auto symbol = program_symtab->lookup(name);
        if (symbol == nullptr) {
            auto global_name = get_variable_name(name);
            printer->add_line("{} = 0.0;"_format(global_name));
        }
    }

    /// note : v is not needed in global structure for nmodl even if vectorize is false

    if (!info.thread_variables.empty()) {
        printer->add_line("{} = 0;"_format(get_variable_name("thread_data_in_use")));
    }

    /// initialize global variables
    for (auto& var: info.global_variables) {
        if (!var->is_array()) {
            auto name = get_variable_name(var->get_name());
            double value = 0;
            auto value_ptr = var->get_value();
            if (value_ptr != nullptr) {
                value = *value_ptr;
            }
            /// use %g to be same as nocmodl in neuron
            printer->add_line("{} = {};"_format(name, "{:g}"_format(value)));
        }
    }

    /// initialize constant variables
    for (auto& var: info.constant_variables) {
        auto name = get_variable_name(var->get_name());
        auto value_ptr = var->get_value();
        double value = 0;
        if (value_ptr != nullptr) {
            value = *value_ptr;
        }
        /// use %g to be same as nocmodl in neuron
        printer->add_line("{} = {};"_format(name, "{:g}"_format(value)));
    }

    if (info.table_count > 0) {
        auto name = get_variable_name(naming::USE_TABLE_VARIABLE);
        printer->add_line("{} = 1;"_format(name));

        for (auto& variable: info.table_statement_variables) {
            auto name = get_variable_name("t_" + variable->get_name());
            int num_values = variable->get_num_values();
            printer->add_line(
                "{} = (double*) mem_alloc({}, sizeof(double));"_format(name, num_values));
        }
    }

    /// update device copy
    print_global_variable_device_update_annotation();

    printer->add_newline();
    printer->add_line("setup_done = 1;");
    printer->end_block(3);

    printer->add_line("/** free global variables */");
    printer->start_block("static inline void free_global_variables() ");
    if (allocated_variables.empty()) {
        printer->add_line("// do nothing");
    } else {
        for (auto& var: allocated_variables) {
            printer->add_line("mem_free({});"_format(var));
        }
    }
    printer->end_block(1);
}


void CodegenCVisitor::print_shadow_vector_setup() {
    printer->add_newline(2);
    printer->add_line("/** allocate and initialize shadow vector */");
    auto args = "{}* inst, Memb_list* ml"_format(instance_struct());
    printer->start_block("static inline void setup_shadow_vectors({}) "_format(args));
    if (channel_task_dependency_enabled()) {
        printer->add_line("int nodecount = ml->nodecount;");
        for (auto& var: codegen_shadow_variables) {
            auto name = var->get_name();
            auto type = default_float_data_type();
            auto allocation = "({0}*) mem_alloc(nodecount, sizeof({0}))"_format(type);
            printer->add_line("inst->{0} = {1};"_format(name, allocation));
        }
    }
    printer->end_block(3);

    printer->add_line("/** free shadow vector */");
    args = "{}* inst"_format(instance_struct());
    printer->start_block("static inline void free_shadow_vectors({}) "_format(args));
    if (channel_task_dependency_enabled()) {
        for (auto& var: codegen_shadow_variables) {
            auto name = var->get_name();
            printer->add_line("mem_free(inst->{});"_format(name));
        }
    }
    printer->end_block(1);
}


void CodegenCVisitor::print_setup_range_variable() {
    auto type = float_data_type();
    printer->add_newline(2);
    printer->add_line("/** allocate and setup array for range variable */");
    printer->start_block(
        "static inline {}* setup_range_variable(double* variable, int n) "_format(type));
    printer->add_line("{0}* data = ({0}*) mem_alloc(n, sizeof({0}));"_format(type));
    printer->add_line("for(size_t i = 0; i < n; i++) {");
    printer->add_line("    data[i] = variable[i];");
    printer->add_line("}");
    printer->add_line("return data;");
    printer->end_block(1);
}


/**
 * Floating point type for the given range variable (symbol)
 *
 * If floating point type like "float" is specified on command line then
 * we can't turn all variables to new type. This is because certain variables
 * are pointers to internal variables (e.g. ions). Hence, we check if given
 * variable can be safely converted to new type. If so, return new type.
 *
 * @param symbol Symbol for the range variable
 * @return Floating point type (float/double)
 */
std::string CodegenCVisitor::get_range_var_float_type(const SymbolType& symbol) {
    // clang-format off
    auto with   =   NmodlType::read_ion_var
                    | NmodlType::write_ion_var
                    | NmodlType::pointer_var
                    | NmodlType::bbcore_pointer_var
                    | NmodlType::extern_neuron_variable;
    // clang-format on
    bool need_default_type = symbol->has_any_property(with);
    if (need_default_type) {
        return default_float_data_type();
    }
    return float_data_type();
}


void CodegenCVisitor::print_instance_variable_setup() {
    if (range_variable_setup_required()) {
        print_setup_range_variable();
    }

    if (shadow_vector_setup_required()) {
        print_shadow_vector_setup();
    }
    printer->add_newline(2);
    printer->add_line("/** initialize mechanism instance variables */");
    printer->start_block("static inline void setup_instance(NrnThread* nt, Memb_list* ml) ");
    printer->add_line("{0}* inst = ({0}*) mem_alloc(1, sizeof({0}));"_format(instance_struct()));
    if (channel_task_dependency_enabled() && !codegen_shadow_variables.empty()) {
        printer->add_line("setup_shadow_vectors(inst, ml);");
    }

    std::string stride;
    if (layout == LayoutType::soa) {
        printer->add_line("int pnodecount = ml->_nodecount_padded;");
        stride = "*pnodecount";
    }

    printer->add_line("Datum* indexes = ml->pdata;");
    int id = 0;
    std::vector<std::string> variables_to_free;
    for (auto& var: codegen_float_variables) {
        auto name = var->get_name();
        auto default_type = default_float_data_type();
        auto range_var_type = get_range_var_float_type(var);
        if (default_type == range_var_type) {
            printer->add_line("inst->{} = ml->data+{}{};"_format(name, id, stride));
        } else {
            printer->add_line("inst->{} = setup_range_variable(ml->data+{}{}, pnodecount);"_format(
                name, id, stride));
            variables_to_free.push_back(name);
        }
        id += var->get_length();
    }

    for (auto& var: codegen_int_variables) {
        auto name = var.symbol->get_name();
        if (var.is_index || var.is_integer) {
            printer->add_line("inst->{} = ml->pdata;"_format(name));
        } else {
            auto data = var.is_vdata ? "_vdata" : "_data";
            printer->add_line("inst->{} = nt->{};"_format(name, data));
        }
    }

    printer->add_line("ml->instance = (void*) inst;");
    printer->end_block(3);

    printer->add_line("/** cleanup mechanism instance variables */");
    printer->start_block("static inline void cleanup_instance(Memb_list* ml) ");
    printer->add_line("{0}* inst = ({0}*) ml->instance;"_format(instance_struct()));
    if (range_variable_setup_required()) {
        for (auto& var: variables_to_free) {
            printer->add_line("mem_free((void*)inst->{});"_format(var));
        }
    }
    printer->add_line("mem_free((void*)inst);");
    printer->end_block(1);
}


void CodegenCVisitor::print_initial_block(InitialBlock* node) {
    if (info.artificial_cell) {
        printer->add_line("double v = 0.0;");
    } else {
        printer->add_line("int node_id = node_index[id];");
        printer->add_line("double v = voltage[node_id];");
    }

    if (ion_variable_struct_required()) {
        printer->add_line("IonCurVar ionvar;");
    }

    /// read ion statements
    auto read_statements = ion_read_statements(BlockType::Initial);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    /// initialize state variables (excluding ion state)
    for (auto& var: info.state_vars) {
        auto name = var->get_name();
        auto lhs = get_variable_name(name);
        auto rhs = get_variable_name(name + "0");
        printer->add_line("{} = {};"_format(lhs, rhs));
    }

    /// initial block
    if (node != nullptr) {
        auto block = node->get_statement_block();
        print_statement_block(block.get(), false, false);
    }

    /// write ion statements
    auto write_statements = ion_write_statements(BlockType::Initial);
    for (auto& statement: write_statements) {
        auto text = process_shadow_update_statement(statement, BlockType::Initial);
        printer->add_line(text);
    }
}


void CodegenCVisitor::print_global_function_common_code(BlockType type) {
    std::string method = compute_method_name(type);
    auto args = "NrnThread* nt, Memb_list* ml, int type";

    print_global_method_annotation();
    printer->start_block("void {}({})"_format(method, args));
    print_kernel_data_present_annotation_block_begin();
    printer->add_line("int nodecount = ml->nodecount;");
    printer->add_line("int pnodecount = ml->_nodecount_padded;");
    printer->add_line(
        "{}int* {}node_index = ml->nodeindices;"_format(k_const(), ptr_type_qualifier()));
    printer->add_line("double* {}data = ml->data;"_format(ptr_type_qualifier()));
    printer->add_line(
        "{}double* {}voltage = nt->_actual_v;"_format(k_const(), ptr_type_qualifier()));

    if (type == BlockType::Equation) {
        printer->add_line("double* {} vec_rhs = nt->_actual_rhs;"_format(ptr_type_qualifier()));
        printer->add_line("double* {} vec_d = nt->_actual_d;"_format(ptr_type_qualifier()));
        print_rhs_d_shadow_variables();
    }
    printer->add_line("Datum* {}indexes = ml->pdata;"_format(ptr_type_qualifier()));
    printer->add_line("ThreadDatum* {}thread = ml->_thread;"_format(ptr_type_qualifier()));

    if (type == BlockType::Initial) {
        printer->add_newline();
        printer->add_line("setup_instance(nt, ml);");
    }
    // clang-format off
    printer->add_line("{0}* {1}inst = ({0}*) ml->instance;"_format(instance_struct(), ptr_type_qualifier()));
    // clang-format on
    printer->add_newline(1);
}


void CodegenCVisitor::print_nrn_init(bool skip_init_check) {
    codegen = true;
    printer->add_newline(2);
    printer->add_line("/** initialize channel */");

    print_global_function_common_code(BlockType::Initial);
    if (info.derivimplicit_coreneuron_solver()) {
        printer->add_newline();
        int nequation = info.num_equations;
        int list_num = info.derivimplicit_list_num;
        // clang-format off
        printer->add_line("*deriv{}_advance(thread) = 0;"_format(list_num));
        printer->add_line("if (*newtonspace{}(thread) == NULL) {}"_format(list_num, "{"));
        printer->add_line("    *newtonspace{}(thread) = nrn_cons_newtonspace({}, pnodecount);"_format(list_num, nequation));
        printer->add_line("    thread[dith{}()].pval = makevector(2*{}*pnodecount*sizeof(double));"_format(list_num, nequation));
        printer->add_line("}");
        // clang-format on
    }

    if (skip_init_check) {
        printer->start_block("if (_nrn_skip_initmodel == 0)");
    }

    print_channel_iteration_tiling_block_begin(BlockType::Initial);
    print_channel_iteration_block_begin(BlockType::Initial);

    print_post_channel_iteration_common_code();

    if (info.net_receive_node != nullptr) {
        printer->add_line("{} = -1e20;"_format(get_variable_name("tsave")));
    }

    print_initial_block(info.initial_node);
    print_channel_iteration_block_end();
    print_shadow_reduction_statements();
    print_channel_iteration_tiling_block_end();
    printer->end_block(1);

    if (info.derivimplicit_coreneuron_solver()) {
        printer->add_line("*deriv{}_advance(thread) = 1;"_format(info.derivimplicit_list_num));
    }
    print_kernel_data_present_annotation_block_end();
    if (skip_init_check) {
        printer->end_block(1);
    }
    codegen = false;
}


void CodegenCVisitor::print_nrn_alloc() {
    printer->add_newline(2);
    auto method = method_name("nrn_alloc");
    printer->start_block("static void {}(double* data, Datum* indexes, int type) "_format(method));
    printer->add_line("// do nothing");
    printer->end_block(1);
}

/**
 * Print nrn_watch_activate function used as a callback for every
 * WATCH statement in the mod file.
 * Todo : number of watch could be more than number of statements
 * according to grammar. Check if this is correctly handled in neuron
 * and coreneuron.
 */
void CodegenCVisitor::print_watch_activate() {
    if (info.watch_statements.empty()) {
        return;
    }
    codegen = true;
    printer->add_newline(2);
    auto inst = "{}* inst"_format(instance_struct());

    printer->start_block(
        "static void nrn_watch_activate({}, int id, int pnodecount, int watch_id) "_format(inst));

    /// initialize all variables only during first watch statement
    printer->add_line("if (watch_id == 0) {");
    for (int i = 0; i < info.watch_count; i++) {
        auto name = get_variable_name("watch{}"_format(i + 1));
        printer->add_line("    {} = 0;"_format(name));
    }
    printer->add_line("}");

    // todo : similar to neuron/coreneuron we are using
    // first watch and ignoring rest.
    for (int i = 0; i < info.watch_statements.size(); i++) {
        auto statement = info.watch_statements[i];
        printer->start_block("if (watch_id == {})"_format(i));

        auto varname = get_variable_name("watch{}"_format(i + 1));
        printer->add_indent();
        printer->add_text("{} = 2 + "_format(varname));
        auto watch = statement->get_statements().front();
        watch->get_expression()->visit_children(this);
        printer->add_text(";");
        printer->add_newline();

        printer->end_block(1);
    }
    printer->end_block(1);
    codegen = false;
}


/**
 * Print kernel for watch activation
 * todo : similar to print_watch_activate, we are using only
 * first watch. need to verify with neuron/coreneuron about rest.
 */
void CodegenCVisitor::print_watch_check() {
    if (info.watch_statements.empty()) {
        return;
    }
    codegen = true;
    printer->add_newline(2);
    printer->add_line("/** routine to check watch activation */");
    print_global_function_common_code(BlockType::Watch);
    print_channel_iteration_tiling_block_begin(BlockType::Watch);
    print_channel_iteration_block_begin(BlockType::Watch);
    print_post_channel_iteration_common_code();

    for (int i = 0; i < info.watch_statements.size(); i++) {
        auto statement = info.watch_statements[i];
        auto watch = statement->get_statements().front();
        auto varname = get_variable_name("watch{}"_format(i + 1));

        // start block 1
        printer->start_block("if ({}&2)"_format(varname));

        // start block 2
        printer->add_indent();
        printer->add_text("if (");
        watch->get_expression()->accept(this);
        printer->add_text(") {");
        printer->add_newline();
        printer->increase_indent();

        // start block 3
        printer->start_block("if (({}&1) == 0)"_format(varname));

        auto tqitem = get_variable_name("tqitem");
        auto point_process = get_variable_name("point_process");
        printer->add_indent();
        printer->add_text("net_send_buffering(");
        auto t = get_variable_name("t");
        printer->add_text(
            "ml->_net_send_buffer, 0, {}, 0, {}, {}+0.0, "_format(tqitem, point_process, t));
        watch->get_value()->accept(this);
        printer->add_text(");");
        printer->add_newline();

        printer->add_line("{} = 3;"_format(varname));
        printer->end_block(1);
        // end block 3

        // start block 3
        printer->start_block("else"_format());
        printer->add_line("{} = 2;"_format(varname));
        printer->end_block(1);
        // end block 3

        printer->decrease_indent();
        printer->add_line("}");
        // end block 2

        printer->end_block(1);
        // end block 1
    }

    print_channel_iteration_block_end();
    print_send_event_move();
    printer->end_block(1);
    codegen = false;
}


void CodegenCVisitor::print_net_receive_common_code(Block* node, bool need_mech_inst) {
    printer->add_line("int tid = pnt->_tid;");
    printer->add_line("int id = pnt->_i_instance;");
    printer->add_line("double v = 0;");
    if (info.artificial_cell) {
        printer->add_line("NrnThread* nt = nrn_threads + tid;");
        printer->add_line("Memb_list* ml = nt->_ml_list[pnt->_type];");
    }
    printer->add_line("{}int nodecount = ml->nodecount;"_format(param_type_qualifier()));
    printer->add_line("{}int pnodecount = ml->_nodecount_padded;"_format(param_type_qualifier()));
    printer->add_line("double* data = ml->data;");
    printer->add_line("double* weights = nt->weights;");
    printer->add_line("Datum* indexes = ml->pdata;");
    printer->add_line("ThreadDatum* thread = ml->_thread;");
    if (need_mech_inst) {
        printer->add_line("{0}* inst = ({0}*) ml->instance;"_format(instance_struct()));
    }

    /// rename variables but need to see if they are actually used
    auto parameters = info.net_receive_node->get_parameters();
    if (!parameters.empty()) {
        int i = 0;
        printer->add_newline();
        for (auto& parameter: parameters) {
            auto name = parameter->get_node_name();
            VarUsageVisitor vu;
            auto var_used = vu.variable_used(node, "(*" + name + ")");
            if (var_used) {
                auto statement = "double* {} = weights + weight_index + {};"_format(name, i++);
                printer->add_line(statement);
                RenameVisitor vr(name, "*" + name);
                node->visit_children(&vr);
            }
        }
    }
}


void CodegenCVisitor::print_net_send_call(FunctionCall* node) {
    auto arguments = node->get_arguments();
    auto tqitem = get_variable_name("tqitem");
    std::string weight_index = "weight_index";
    std::string pnt = "pnt";

    /// for non-net-receieve functions there is no weight index argument
    /// and artificial cell is in vdata which is void**
    if (!printing_net_receive) {
        weight_index = "-1";
        auto var = get_variable_name("point_process");
        if (info.artificial_cell) {
            pnt = "(Point_process*)" + var;
        }
    }

    /// artificial cells don't use spike buffering
    // clang-format off
    if (info.artificial_cell) {
        printer->add_text("artcell_net_send(&{}, {}, {}, nt->_t+"_format(tqitem, weight_index, pnt));
    } else {
        auto point_process = get_variable_name("point_process");
        std::string t = get_variable_name("t");
        printer->add_text("net_send_buffering(");
        printer->add_text("ml->_net_send_buffer, 0, {}, {}, {}, {}+"_format(tqitem, weight_index, point_process, t));
    }
    // clang-format off
    print_vector_elements(arguments, ", ");
    printer->add_text(")");
}


void CodegenCVisitor::print_net_move_call(FunctionCall* node) {
    if (!printing_net_receive) {
        std::cout << "Error : net_move only allowed in NET_RECEIVE block" << std::endl;
        abort();
    }

    auto arguments = node->get_arguments();
    auto tqitem = get_variable_name("tqitem");
    std::string weight_index = "-1";
    std::string pnt = "pnt";

    /// artificial cells don't use spike buffering
    // clang-format off
    if (info.artificial_cell) {
        printer->add_text("artcell_net_move(&{}, {}, {}, nt->_t+"_format(tqitem, weight_index, pnt));
    } else {
        auto point_process = get_variable_name("point_process");
        std::string t = get_variable_name("t");
        printer->add_text("net_send_buffering(");
        printer->add_text("ml->_net_send_buffer, 2, {}, {}, {}, {}+"_format(tqitem, weight_index, point_process, t));
    }
    // clang-format off
    print_vector_elements(arguments, ", ");
    printer->add_text(", 0.0");
    printer->add_text(")");
}


void CodegenCVisitor::print_net_event_call(FunctionCall* node) {
    auto arguments = node->get_arguments();
    if (info.artificial_cell) {
        printer->add_text("net_event(pnt, ");
        print_vector_elements(arguments, ", ");
    } else {
        auto point_process = get_variable_name("point_process");
        printer->add_text("net_send_buffering(");
        printer->add_text("ml->_net_send_buffer, 1, -1, -1, {}, "_format(point_process));
        print_vector_elements(arguments, ", ");
        printer->add_text(", 0.0");
    }
    printer->add_text(")");
}


void CodegenCVisitor::print_net_init() {
    auto node = info.net_receive_initial_node;
    if (node == nullptr) {
        return;
    }
    codegen = true;
    auto args = "Point_process* pnt, int weight_index, double flag";
    printer->add_newline(2);
    printer->add_line("/** initialize block for net receive */");
    printer->start_block("static void net_init({}) "_format(args));
    auto block = node->get_statement_block().get();
    if (block->get_statements().empty()) {
        printer->add_line("// do nothing");
    } else {
        print_net_receive_common_code(node);
        print_statement_block(block, false, false);
    }
    printer->end_block(1);
    codegen = false;
}


void CodegenCVisitor::print_send_event_move() {
    printer->add_newline();
    printer->add_line("NetSendBuffer_t* nsb = ml->_net_send_buffer;");
    // todo : update net send buffer on host
    printer->add_line("for (int i=0; i < nsb->_cnt; i++) {");
    printer->add_line("    int type = nsb->_sendtype[i];");
    printer->add_line("    int tid = nt->id;");
    printer->add_line("    double t = nsb->_nsb_t[i];");
    printer->add_line("    double flag = nsb->_nsb_flag[i];");
    printer->add_line("    int vdata_index = nsb->_vdata_index[i];");
    printer->add_line("    int weight_index = nsb->_weight_index[i];");
    printer->add_line("    int point_index = nsb->_pnt_index[i];");
    // clang-format off
    printer->add_line("    net_sem_from_gpu(type, vdata_index, weight_index, tid, point_index, t, flag);");
    // clang-format on
    printer->add_line("}");
    printer->add_line("nsb->_cnt = 0;");
    // todo : update net send buffer count on device
}


std::string CodegenCVisitor::net_receive_buffering_declaration() {
    return "void {}(NrnThread* nt)"_format(method_name("net_buf_receive"));
}


void CodegenCVisitor::print_get_memb_list() {
    printer->add_line("Memb_list* ml = get_memb_list(nt);");
    printer->add_line("if (ml == NULL) {");
    printer->add_line("    return;");
    printer->add_line("}");
    printer->add_newline();
}


void CodegenCVisitor::print_net_receive_loop_begin() {
    printer->add_line("int count = nrb->_displ_cnt;");
    printer->start_block("for (int i = 0; i < count; i++)");
}


void CodegenCVisitor::print_net_receive_loop_end() {
    printer->end_block(1);
}


void CodegenCVisitor::print_net_receive_buffering(bool need_mech_inst) {
    if (!net_receive_required() || info.artificial_cell) {
        return;
    }
    printer->add_newline(2);
    printer->start_block(net_receive_buffering_declaration());

    print_get_memb_list();

    auto net_receive = method_name("net_receive_kernel");

    printer->add_line(
        "NetReceiveBuffer_t* {}nrb = ml->_net_receive_buffer;"_format(ptr_type_qualifier()));
    if (need_mech_inst) {
        printer->add_line("{0}* inst = ({0}*) ml->instance;"_format(instance_struct()));
    }
    print_net_receive_loop_begin();
    printer->add_line("int start = nrb->_displ[i];");
    printer->add_line("int end = nrb->_displ[i+1];");
    printer->start_block("for (int j = start; j < end; j++)");
    printer->add_line("int index = nrb->_nrb_index[j];");
    printer->add_line("int offset = nrb->_pnt_index[index];");
    printer->add_line("double t = nrb->_nrb_t[index];");
    printer->add_line("int weight_index = nrb->_weight_index[index];");
    printer->add_line("double flag = nrb->_nrb_flag[index];");
    printer->add_line("Point_process* point_process = nt->pntprocs + offset;");
    printer->add_line(
        "{}(t, point_process, inst, nt, ml, weight_index, flag);"_format(net_receive));
    printer->end_block(1);
    print_net_receive_loop_end();

    if (info.net_send_used || info.net_event_used) {
        print_send_event_move();
    }

    printer->add_newline();
    printer->add_line("nrb->_displ_cnt = 0;");
    printer->add_line("nrb->_cnt = 0;");

    printer->end_block(1);
}


void CodegenCVisitor::print_net_send_buffering() {
    if (!net_send_buffer_required()) {
        return;
    }

    auto error = add_escape_quote("Error : netsend buffer size (%d) exceeded\\n");
    printer->add_newline(2);
    print_device_method_annotation();
    auto args =
        "NetSendBuffer_t* nsb, int type, int vdata_index, "
        "int weight_index, int point_index, double t, double flag";
    printer->start_block("static inline void net_send_buffering({}) "_format(args));
    printer->add_line("int i = nsb->_cnt;");
    printer->add_line("nsb->_cnt++;");
    printer->add_line("if(nsb->_cnt >= nsb->_size) {");
    printer->add_line("    printf({}, nsb->_cnt);"_format(error));
    printer->add_line("    abort();");
    printer->add_line("}");
    printer->add_line("nsb->_sendtype[i] = type;");
    printer->add_line("nsb->_vdata_index[i] = vdata_index;");
    printer->add_line("nsb->_weight_index[i] = weight_index;");
    printer->add_line("nsb->_pnt_index[i] = point_index;");
    printer->add_line("nsb->_nsb_t[i] = t;");
    printer->add_line("nsb->_nsb_flag[i] = flag;");
    printer->end_block(1);
}


void CodegenCVisitor::print_net_receive_kernel() {
    if (!net_receive_required()) {
        return;
    }
    codegen = true;
    printing_net_receive = true;
    auto node = info.net_receive_node;

    /// rename arguments if same name is used
    auto parameters = node->get_parameters();
    for (auto& parameter: parameters) {
        auto name = parameter->get_node_name();
        auto var_used = VarUsageVisitor().variable_used(node, name);
        if (var_used) {
            RenameVisitor vr(name, "(*" + name + ")");
            node->get_statement_block()->visit_children(&vr);
        }
    }

    std::string name;
    auto params = ParamVector();
    if (!info.artificial_cell) {
        name = method_name("net_receive_kernel");
        params.emplace_back("", "double", "", "t");
        params.emplace_back("", "Point_process*", "", "pnt");
        params.emplace_back(param_type_qualifier(),
                            "{}*"_format(instance_struct()),
                            param_ptr_qualifier(),
                            "inst");
        params.emplace_back(param_type_qualifier(), "NrnThread*", param_ptr_qualifier(), "nt");
        params.emplace_back(param_type_qualifier(), "Memb_list*", param_ptr_qualifier(), "ml");
        params.emplace_back("", "int", "", "weight_index");
        params.emplace_back("", "double", "", "flag");
    } else {
        name = method_name("net_receive");
        params.emplace_back("", "Point_process*", "", "pnt");
        params.emplace_back("", "int", "", "weight_index");
        params.emplace_back("", "double", "", "flag");
    }

    printer->add_newline(2);
    printer->start_block("static inline void {}({}) "_format(name, get_parameter_str(params)));
    print_net_receive_common_code(node, info.artificial_cell);
    if (info.artificial_cell) {
        printer->add_line("double t = nt->_t;");
    }
    printer->add_line("{} = t;"_format(get_variable_name("tsave")));
    printer->add_indent();
    node->get_statement_block()->accept(this);
    printer->add_newline();
    printer->end_block();
    printer->add_newline();

    printing_net_receive = false;
    codegen = false;
}


void CodegenCVisitor::print_net_receive() {
    if (!net_receive_required()) {
        return;
    }
    codegen = true;
    printing_net_receive = true;
    if (!info.artificial_cell) {
        std::string name = method_name("net_receive");
        auto params = ParamVector();
        params.emplace_back("", "Point_process*", "", "pnt");
        params.emplace_back("", "int", "", "weight_index");
        params.emplace_back("", "double", "", "flag");
        printer->add_newline(2);
        printer->start_block("static void {}({}) "_format(name, get_parameter_str(params)));
        printer->add_line("NrnThread* nt = nrn_threads + pnt->_tid;");
        printer->add_line("Memb_list* ml = get_memb_list(nt);");
        printer->add_line("NetReceiveBuffer_t* nrb = ml->_net_receive_buffer;");
        printer->add_line("if (nrb->_cnt >= nrb->_size) {");
        printer->add_line("    realloc_net_receive_buffer(nt, ml);");
        printer->add_line("}");
        printer->add_line("int id = nrb->_cnt;");
        printer->add_line("nrb->_pnt_index[id] = pnt-nt->pntprocs;");
        printer->add_line("nrb->_weight_index[id] = weight_index;");
        printer->add_line("nrb->_nrb_t[id] = nt->_t;");
        printer->add_line("nrb->_nrb_flag[id] = flag;");
        printer->add_line("nrb->_cnt++;");
        printer->end_block(1);
    }
    printing_net_receive = false;
    codegen = false;
}


/**
 * Todo: data is not derived. Need to add instance into instance struct?
 * data used here is wrong in AoS because as in original implementation,
 * data is not incremented every iteration for AoS. May be better to derive
 * actual variable names? [resolved now?]
 * slist needs to added as local variable
 */
void CodegenCVisitor::print_derivimplicit_kernel(Block* block) {
    auto ext_args = external_method_arguments();
    auto ext_params = external_method_parameters();
    auto suffix = info.mod_suffix;
    auto list_num = info.derivimplicit_list_num;
    auto block_name = block->get_node_name();
    auto primes_size = info.primes_size;
    auto stride = (layout == LayoutType::aos) ? "" : "*pnodecount+id";

    printer->add_newline(2);

    // clang-format off
    printer->start_block("int {}_{}({})"_format(block_name, suffix, ext_params));
    auto instance = "{0}* inst = ({0}*)get_memb_list(nt)->instance;"_format(instance_struct());
    auto slist1 = "int* slist{} = {};"_format(list_num, get_variable_name("slist{}"_format(list_num)));
    auto slist2 = "int* slist{} = {};"_format(list_num+1, get_variable_name("slist{}"_format(list_num+1)));
    auto dlist1 = "int* dlist{} = {};"_format(list_num, get_variable_name("dlist{}"_format(list_num)));
    auto dlist2 = "double* dlist{} = (double*) thread[dith{}()].pval + ({}*pnodecount);"_format(list_num + 1, list_num, info.primes_size);

    printer->add_line(instance);
    printer->add_line("double* savstate{} = (double*) thread[dith{}()].pval;"_format(list_num, list_num));
    printer->add_line(slist1);
    printer->add_line(slist2);
    printer->add_line(dlist2);
    printer->add_line("for (int i=0; i<{}; i++) {}"_format(info.num_primes, "{"));
    printer->add_line("    savstate{}[i{}] = data[slist{}[i]{}];"_format(list_num, stride, list_num, stride));
    printer->add_line("}");

    auto argument = "{}, slist{}, _derivimplicit_{}_{}, dlist{}, {}"_format(primes_size, list_num+1, block_name, suffix, list_num + 1, ext_args);
    printer->add_line("int reset = nrn_newton_thread(static_cast<NewtonSpace*>(*newtonspace{}(thread)), {});"_format(list_num, argument));
    printer->add_line("return reset;");
    printer->end_block(3);

    /*
     * TODO : To be backward compatible with mod2c we have to generate below
     * comment marker in the generated cpp file for kinderiv.py to
     * process it and generate correct _kinderiv.h
     */
    printer->add_line("/* _derivimplicit_ {} _{} */"_format(block_name, info.mod_suffix));
    printer->add_newline(1);

    printer->start_block("int _newton_{}_{}({}) "_format(block_name, info.mod_suffix, external_method_parameters()));
    printer->add_line(instance);
    if (ion_variable_struct_required()) {
        print_ion_variable();
    }
    printer->add_line("double* savstate{} = (double*) thread[dith{}()].pval;"_format(list_num, list_num));
    printer->add_line(slist1);
    printer->add_line(dlist1);
    printer->add_line(dlist2);
    print_statement_block(block->get_statement_block().get(), false, false);
    printer->add_line("int counter = -1;");
    printer->add_line("for (int i=0; i<{}; i++) {}"_format(info.num_primes, "{"));
    printer->add_line("    if (*deriv{}_advance(thread)) {}"_format(list_num, "{"));
    printer->add_line("        dlist{0}[(++counter){1}] = data[dlist{2}[i]{1}]-(data[slist{2}[i]{1}]-savstate{2}[i{1}])/dt;"_format(list_num + 1, stride, list_num));
    printer->add_line("    }");
    printer->add_line("    else {");
    printer->add_line("        dlist{0}[(++counter){1}] = data[slist{2}[i]{1}]-savstate{2}[i{1}];"_format(list_num + 1, stride, list_num));
    printer->add_line("    }");
    printer->add_line("}");
    printer->add_line("return 0;");
    printer->end_block();
    // clang-format on
}

void CodegenCVisitor::visit_derivimplicit_callback(ast::DerivimplicitCallback* node) {
    if (!codegen) {
        return;
    }
    auto thread_args = external_method_arguments();
    auto num_primes = info.num_primes;
    auto suffix = info.mod_suffix;
    int num = info.derivimplicit_list_num;
    auto slist = get_variable_name("slist{}"_format(num));
    auto dlist = get_variable_name("dlist{}"_format(num));
    auto block_name = node->get_node_to_solve()->get_node_name();

    auto args =
        "{}, {}, {}, _derivimplicit_{}_{}, {}"
        ""_format(num_primes, slist, dlist, block_name, suffix, thread_args);
    auto statement = "derivimplicit_thread({});"_format(args);
    printer->add_line(statement);
}

void CodegenCVisitor::visit_solution_expression(SolutionExpression* node) {
    auto block = node->get_node_to_solve().get();
    if (block->is_statement_block()) {
        auto statement_block = dynamic_cast<ast::StatementBlock*>(block);
        print_statement_block(statement_block, false, false);
    } else {
        block->accept(this);
    }
}


/****************************************************************************************/
/*                                Print nrn_state routine                                */
/****************************************************************************************/


void CodegenCVisitor::print_nrn_state() {
    if (!nrn_state_required()) {
        return;
    }
    codegen = true;

    printer->add_newline(2);
    printer->add_line("/** update state */");
    print_global_function_common_code(BlockType::State);
    print_channel_iteration_tiling_block_begin(BlockType::State);
    print_channel_iteration_block_begin(BlockType::State);
    print_post_channel_iteration_common_code();

    printer->add_line("int node_id = node_index[id];");
    printer->add_line("double v = voltage[node_id];");

    /// todo : eigen solver node also emits IonCurVar variable in the functor
    /// but that shouldn't update ions in derivative block
    if (ion_variable_struct_required()) {
        print_ion_variable();
    }

    auto read_statements = ion_read_statements(BlockType::State);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    if (info.nrn_state_block) {
        info.nrn_state_block->visit_children(this);
    }

    if (info.currents.empty() && info.breakpoint_node != nullptr) {
        auto block = info.breakpoint_node->get_statement_block();
        print_statement_block(block.get(), false, false);
    }

    auto write_statements = ion_write_statements(BlockType::State);
    for (auto& statement: write_statements) {
        auto text = process_shadow_update_statement(statement, BlockType::State);
        printer->add_line(text);
    }
    print_channel_iteration_block_end();
    if (!shadow_statements.empty()) {
        print_shadow_reduction_block_begin();
        print_shadow_reduction_statements();
        print_shadow_reduction_block_end();
    }
    print_channel_iteration_tiling_block_end();

    print_kernel_data_present_annotation_block_end();
    printer->end_block(1);
    codegen = false;
}


/****************************************************************************************/
/*                            Print nrn_cur related routines                            */
/****************************************************************************************/


void CodegenCVisitor::print_nrn_current(BreakpointBlock* node) {
    auto args = internal_method_parameters();
    auto block = node->get_statement_block().get();
    printer->add_newline(2);
    print_device_method_annotation();
    printer->start_block("static inline double nrn_current({})"_format(get_parameter_str(args)));
    printer->add_line("double current = 0.0;");
    print_statement_block(block, false, false);
    for (auto& current: info.currents) {
        auto name = get_variable_name(current);
        printer->add_line("current += {};"_format(name));
    }
    printer->add_line("return current;");
    printer->end_block(1);
}


void CodegenCVisitor::print_nrn_cur_conductance_kernel(BreakpointBlock* node) {
    auto block = node->get_statement_block();
    print_statement_block(block.get(), false, false);
    if (!info.currents.empty()) {
        std::string sum;
        for (const auto& current: info.currents) {
            auto var = breakpoint_current(current);
            sum += get_variable_name(var);
            if (&current != &info.currents.back()) {
                sum += "+";
            }
        }
        printer->add_line("double rhs = {};"_format(sum));
    }
    if (!info.conductances.empty()) {
        std::string sum;
        for (const auto& conductance: info.conductances) {
            auto var = breakpoint_current(conductance.variable);
            sum += get_variable_name(var);
            if (&conductance != &info.conductances.back()) {
                sum += "+";
            }
        }
        printer->add_line("double g = {};"_format(sum));
    }
    for (const auto& conductance: info.conductances) {
        if (!conductance.ion.empty()) {
            auto lhs = "ion_di" + conductance.ion + "dv";
            auto rhs = get_variable_name(conductance.variable);
            auto statement = ShadowUseStatement{lhs, "+=", rhs};
            auto text = process_shadow_update_statement(statement, BlockType::Equation);
            printer->add_line(text);
        }
    }
}


void CodegenCVisitor::print_nrn_cur_non_conductance_kernel() {
    printer->add_line("double g = nrn_current({}+0.001);"_format(internal_method_arguments()));
    for (auto& ion: info.ions) {
        for (auto& var: ion.writes) {
            if (ion.is_ionic_current(var)) {
                auto name = get_variable_name(var);
                printer->add_line("double di{} = {};"_format(ion.name, name));
            }
        }
    }
    printer->add_line("double rhs = nrn_current({});"_format(internal_method_arguments()));
    printer->add_line("g = (g-rhs)/0.001;");
    for (auto& ion: info.ions) {
        for (auto& var: ion.writes) {
            if (ion.is_ionic_current(var)) {
                auto lhs = "ion_di" + ion.name + "dv";
                auto rhs = "(di{}-{})/0.001"_format(ion.name, get_variable_name(var));
                if (info.point_process) {
                    auto area = get_variable_name(naming::NODE_AREA_VARIABLE);
                    rhs += "*1.e2/{}"_format(area);
                }
                auto statement = ShadowUseStatement{lhs, "+=", rhs};
                auto text = process_shadow_update_statement(statement, BlockType::Equation);
                printer->add_line(text);
            }
        }
    }
}


void CodegenCVisitor::print_nrn_cur_kernel(BreakpointBlock* node) {
    printer->add_line("int node_id = node_index[id];");
    printer->add_line("double v = voltage[node_id];");
    if (ion_variable_struct_required()) {
        print_ion_variable();
    }

    auto read_statements = ion_read_statements(BlockType::Equation);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    if (info.conductances.empty()) {
        print_nrn_cur_non_conductance_kernel();
    } else {
        print_nrn_cur_conductance_kernel(node);
    }

    auto write_statements = ion_write_statements(BlockType::Equation);
    for (auto& statement: write_statements) {
        auto text = process_shadow_update_statement(statement, BlockType::Equation);
        printer->add_line(text);
    }

    if (info.point_process) {
        auto area = get_variable_name(naming::NODE_AREA_VARIABLE);
        printer->add_line("double mfactor = 1.e2/{};"_format(area));
        printer->add_line("g = g*mfactor;");
        printer->add_line("rhs = rhs*mfactor;");
    }
}


void CodegenCVisitor::print_nrn_cur() {
    if (!nrn_cur_required()) {
        return;
    }

    codegen = true;
    if (info.conductances.empty()) {
        print_nrn_current(info.breakpoint_node);
    }

    printer->add_newline(2);
    printer->add_line("/** update current */");
    print_global_function_common_code(BlockType::Equation);
    print_channel_iteration_tiling_block_begin(BlockType::Equation);
    print_channel_iteration_block_begin(BlockType::Equation);
    print_post_channel_iteration_common_code();
    print_nrn_cur_kernel(info.breakpoint_node);
    print_nrn_cur_matrix_shadow_update();
    print_channel_iteration_block_end();

    if (nrn_cur_reduction_loop_required()) {
        print_shadow_reduction_block_begin();
        print_nrn_cur_matrix_shadow_reduction();
        print_shadow_reduction_statements();
        print_shadow_reduction_block_end();
    }

    print_channel_iteration_tiling_block_end();
    print_kernel_data_present_annotation_block_end();
    printer->end_block(1);
    codegen = false;
}


/****************************************************************************************/
/*                            Main code printing entry points                            */
/****************************************************************************************/

void CodegenCVisitor::print_headers_include() {
    print_standard_includes();
    print_backend_includes();
    print_coreneuron_includes();
}


void CodegenCVisitor::print_namespace_begin() {
    print_namespace_start();
    print_backend_namespace_start();
}


void CodegenCVisitor::print_namespace_end() {
    print_backend_namespace_stop();
    print_namespace_stop();
}


void CodegenCVisitor::print_common_getters() {
    print_first_pointer_var_index_getter();
    print_memory_layout_getter();
    print_net_receive_arg_size_getter();
    print_thread_getters();
    print_num_variable_getter();
    print_mech_type_getter();
    print_memb_list_getter();
}


void CodegenCVisitor::print_data_structures() {
    print_mechanism_global_var_structure();
    print_mechanism_range_var_structure();
    print_ion_var_structure();
}


void CodegenCVisitor::print_compute_functions() {
    print_top_verbatim_blocks();
    print_function_prototypes();
    for (const auto& procedure: info.procedures) {
        print_procedure(procedure);
    }
    for (const auto& function: info.functions) {
        print_function(function);
    }
    for (const auto& callback: info.derivimplicit_callbacks) {
        auto block = callback->get_node_to_solve().get();
        print_derivimplicit_kernel(block);
    }
    print_net_init();
    print_net_send_buffering();
    print_watch_activate();
    print_watch_check();
    print_net_receive_kernel();
    print_net_receive();
    print_net_receive_buffering();
    print_nrn_init();
    print_nrn_cur();
    print_nrn_state();
}


void CodegenCVisitor::print_codegen_routines() {
    codegen = true;
    print_backend_info();
    print_headers_include();
    print_namespace_begin();
    print_nmodl_constant();
    print_mechanism_info();
    print_data_structures();
    print_global_variables_for_hoc();
    print_common_getters();
    print_memory_allocation_routine();
    print_thread_memory_callbacks();
    print_global_variable_setup();
    print_instance_variable_setup();
    print_nrn_alloc();
    print_compute_functions();
    print_check_table_thread_function();
    print_mechanism_register();
    print_namespace_end();
    codegen = false;
}


void CodegenCVisitor::print_wrapper_routines() {
    // nothing to do
}


void CodegenCVisitor::set_codegen_global_variables(std::vector<SymbolType>& global_vars) {
    codegen_global_variables = global_vars;
}


void CodegenCVisitor::setup(Program* node) {
    program_symtab = node->get_symbol_table();

    CodegenHelperVisitor v;
    info = v.analyze(node);
    info.mod_file = mod_filename;

    if (!info.vectorize) {
        logger->warn("CodegenCVisitor : MOD file uses non-thread safe constructs of NMODL");
    }

    codegen_float_variables = get_float_variables();
    codegen_int_variables = get_int_variables();
    codegen_shadow_variables = get_shadow_variables();

    update_index_semantics();
    rename_function_arguments();
}


void CodegenCVisitor::visit_program(Program* node) {
    setup(node);
    print_codegen_routines();
    print_wrapper_routines();
}

}  // namespace codegen
}  // namespace nmodl
