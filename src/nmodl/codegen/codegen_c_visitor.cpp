/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "codegen/codegen_c_visitor.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <regex>

#include "ast/all.hpp"
#include "codegen/codegen_helper_visitor.hpp"
#include "codegen/codegen_naming.hpp"
#include "codegen/codegen_utils.hpp"
#include "config/config.h"
#include "lexer/token_mapping.hpp"
#include "parser/c11_driver.hpp"
#include "utils/logger.hpp"
#include "utils/string_utils.hpp"
#include "visitors/defuse_analyze_visitor.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/var_usage_visitor.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace codegen {

using namespace ast;

using visitor::DefUseAnalyzeVisitor;
using visitor::DUChain;
using visitor::DUState;
using visitor::RenameVisitor;
using visitor::SymtabVisitor;
using visitor::VarUsageVisitor;

using symtab::syminfo::NmodlType;
using SymbolType = std::shared_ptr<symtab::Symbol>;

namespace codegen_utils = nmodl::codegen::utils;

/****************************************************************************************/
/*                            Overloaded visitor routines                               */
/****************************************************************************************/

static const std::regex regex_special_chars{R"([-[\]{}()*+?.,\^$|#\s])"};

void CodegenCVisitor::visit_string(const String& node) {
    if (!codegen) {
        return;
    }
    std::string name = node.eval();
    if (enable_variable_name_lookup) {
        name = get_variable_name(name);
    }
    printer->add_text(name);
}


void CodegenCVisitor::visit_integer(const Integer& node) {
    if (!codegen) {
        return;
    }
    const auto& value = node.get_value();
    printer->add_text(std::to_string(value));
}


void CodegenCVisitor::visit_float(const Float& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(format_float_string(node.get_value()));
}


void CodegenCVisitor::visit_double(const Double& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(format_double_string(node.get_value()));
}


void CodegenCVisitor::visit_boolean(const Boolean& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(std::to_string(static_cast<int>(node.eval())));
}


void CodegenCVisitor::visit_name(const Name& node) {
    if (!codegen) {
        return;
    }
    node.visit_children(*this);
}


void CodegenCVisitor::visit_unit(const ast::Unit& node) {
    // do not print units
}


void CodegenCVisitor::visit_prime_name(const PrimeName& /* node */) {
    throw std::runtime_error("PRIME encountered during code generation, ODEs not solved?");
}


/**
 * \todo : Validate how @ is being handled in neuron implementation
 */
void CodegenCVisitor::visit_var_name(const VarName& node) {
    if (!codegen) {
        return;
    }
    const auto& name = node.get_name();
    const auto& at_index = node.get_at();
    const auto& index = node.get_index();
    name->accept(*this);
    if (at_index) {
        printer->add_text("@");
        at_index->accept(*this);
    }
    if (index) {
        printer->add_text("[");
        printer->add_text("static_cast<int>(");
        index->accept(*this);
        printer->add_text(")");
        printer->add_text("]");
    }
}


void CodegenCVisitor::visit_indexed_name(const IndexedName& node) {
    if (!codegen) {
        return;
    }
    node.get_name()->accept(*this);
    printer->add_text("[");
    printer->add_text("static_cast<int>(");
    node.get_length()->accept(*this);
    printer->add_text(")");
    printer->add_text("]");
}


void CodegenCVisitor::visit_local_list_statement(const LocalListStatement& node) {
    if (!codegen) {
        return;
    }
    auto type = local_var_type() + " ";
    printer->add_text(type);
    print_vector_elements(node.get_variables(), ", ");
}


void CodegenCVisitor::visit_if_statement(const IfStatement& node) {
    if (!codegen) {
        return;
    }
    printer->add_text("if (");
    node.get_condition()->accept(*this);
    printer->add_text(") ");
    node.get_statement_block()->accept(*this);
    print_vector_elements(node.get_elseifs(), "");
    const auto& elses = node.get_elses();
    if (elses) {
        elses->accept(*this);
    }
}


void CodegenCVisitor::visit_else_if_statement(const ElseIfStatement& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(" else if (");
    node.get_condition()->accept(*this);
    printer->add_text(") ");
    node.get_statement_block()->accept(*this);
}


void CodegenCVisitor::visit_else_statement(const ElseStatement& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(" else ");
    node.visit_children(*this);
}


void CodegenCVisitor::visit_while_statement(const WhileStatement& node) {
    printer->add_text("while (");
    node.get_condition()->accept(*this);
    printer->add_text(") ");
    node.get_statement_block()->accept(*this);
}


void CodegenCVisitor::visit_from_statement(const ast::FromStatement& node) {
    if (!codegen) {
        return;
    }
    auto name = node.get_node_name();
    const auto& from = node.get_from();
    const auto& to = node.get_to();
    const auto& inc = node.get_increment();
    const auto& block = node.get_statement_block();
    printer->fmt_text("for(int {}=", name);
    from->accept(*this);
    printer->fmt_text("; {}<=", name);
    to->accept(*this);
    if (inc) {
        printer->fmt_text("; {}+=", name);
        inc->accept(*this);
    } else {
        printer->fmt_text("; {}++", name);
    }
    printer->add_text(")");
    block->accept(*this);
}


void CodegenCVisitor::visit_paren_expression(const ParenExpression& node) {
    if (!codegen) {
        return;
    }
    printer->add_text("(");
    node.get_expression()->accept(*this);
    printer->add_text(")");
}


void CodegenCVisitor::visit_binary_expression(const BinaryExpression& node) {
    if (!codegen) {
        return;
    }
    auto op = node.get_op().eval();
    const auto& lhs = node.get_lhs();
    const auto& rhs = node.get_rhs();
    if (op == "^") {
        printer->add_text("pow(");
        lhs->accept(*this);
        printer->add_text(", ");
        rhs->accept(*this);
        printer->add_text(")");
    } else {
        lhs->accept(*this);
        printer->add_text(" " + op + " ");
        rhs->accept(*this);
    }
}


void CodegenCVisitor::visit_binary_operator(const BinaryOperator& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(node.eval());
}


void CodegenCVisitor::visit_unary_operator(const UnaryOperator& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(" " + node.eval());
}


/**
 * \details Statement block is top level construct (for every nmodl block).
 * Sometime we want to analyse ast nodes even if code generation is
 * false. Hence we visit children even if code generation is false.
 */
void CodegenCVisitor::visit_statement_block(const StatementBlock& node) {
    if (!codegen) {
        node.visit_children(*this);
        return;
    }
    print_statement_block(node);
}


void CodegenCVisitor::visit_function_call(const FunctionCall& node) {
    if (!codegen) {
        return;
    }
    print_function_call(node);
}


void CodegenCVisitor::visit_verbatim(const Verbatim& node) {
    if (!codegen) {
        return;
    }
    auto text = node.get_statement()->eval();
    auto result = process_verbatim_text(text);

    auto statements = stringutils::split_string(result, '\n');
    for (auto& statement: statements) {
        stringutils::trim_newline(statement);
        if (statement.find_first_not_of(' ') != std::string::npos) {
            printer->add_line(statement);
        }
    }
}

void CodegenCVisitor::visit_update_dt(const ast::UpdateDt& node) {
    // dt change statement should be pulled outside already
}

/****************************************************************************************/
/*                               Common helper routines                                 */
/****************************************************************************************/


/**
 * \details Certain statements like unit, comment, solve can/need to be skipped
 * during code generation. Note that solve block is wrapped in expression
 * statement and hence we have to check inner expression. It's also true
 * for the initial block defined inside net receive block.
 */
bool CodegenCVisitor::statement_to_skip(const Statement& node) {
    // clang-format off
    if (node.is_unit_state()
        || node.is_line_comment()
        || node.is_block_comment()
        || node.is_solve_block()
        || node.is_conductance_hint()
        || node.is_table_statement()) {
        return true;
    }
    // clang-format on
    if (node.is_expression_statement()) {
        auto expression = dynamic_cast<const ExpressionStatement*>(&node)->get_expression();
        if (expression->is_solve_block()) {
            return true;
        }
        if (expression->is_initial_block()) {
            return true;
        }
    }
    return false;
}


bool CodegenCVisitor::net_send_buffer_required() const noexcept {
    if (net_receive_required() && !info.artificial_cell) {
        if (info.net_event_used || info.net_send_used || info.is_watch_used()) {
            return true;
        }
    }
    return false;
}


bool CodegenCVisitor::net_receive_buffering_required() const noexcept {
    return info.point_process && !info.artificial_cell && info.net_receive_node != nullptr;
}


bool CodegenCVisitor::nrn_state_required() const noexcept {
    if (info.artificial_cell) {
        return false;
    }
    return info.nrn_state_block != nullptr || breakpoint_exist();
}


bool CodegenCVisitor::nrn_cur_required() const noexcept {
    return info.breakpoint_node != nullptr && !info.currents.empty();
}


bool CodegenCVisitor::net_receive_exist() const noexcept {
    return info.net_receive_node != nullptr;
}


bool CodegenCVisitor::breakpoint_exist() const noexcept {
    return info.breakpoint_node != nullptr;
}


bool CodegenCVisitor::net_receive_required() const noexcept {
    return net_receive_exist();
}


/**
 * \details When floating point data type is not default (i.e. double) then we
 * have to copy old array to new type (for range variables).
 */
bool CodegenCVisitor::range_variable_setup_required() const noexcept {
    return codegen::naming::DEFAULT_FLOAT_TYPE != float_data_type();
}


bool CodegenCVisitor::state_variable(const std::string& name) const {
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


int CodegenCVisitor::position_of_float_var(const std::string& name) const {
    int index = 0;
    for (const auto& var: codegen_float_variables) {
        if (var->get_name() == name) {
            return index;
        }
        index += var->get_length();
    }
    throw std::logic_error(name + " variable not found");
}


int CodegenCVisitor::position_of_int_var(const std::string& name) const {
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
 * \details We can directly print value but if user specify value as integer then
 * then it gets printed as an integer. To avoid this, we use below wrapper.
 * If user has provided integer then it gets printed as 1.0 (similar to mod2c
 * and neuron where ".0" is appended). Otherwise we print double variables as
 * they are represented in the mod file by user. If the value is in scientific
 * representation (1e+20, 1E-15) then keep it as it is.
 */
std::string CodegenCVisitor::format_double_string(const std::string& s_value) {
    return codegen_utils::format_double_string<CodegenCVisitor>(s_value);
}


std::string CodegenCVisitor::format_float_string(const std::string& s_value) {
    return codegen_utils::format_float_string<CodegenCVisitor>(s_value);
}


/**
 * \details Statements like if, else etc. don't need semicolon at the end.
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
            || expression->is_solution_expression()
            || expression->is_for_netcon()) {
            return false;
        }
    }
    // clang-format on
    return true;
}


// check if there is a function or procedure defined with given name
bool CodegenCVisitor::defined_method(const std::string& name) const {
    const auto& function = program_symtab->lookup(name);
    auto properties = NmodlType::function_block | NmodlType::procedure_block;
    return function && function->has_any_property(properties);
}


/**
 * \details Current variable used in breakpoint block could be local variable.
 * In this case, neuron has already renamed the variable name by prepending
 * "_l". In our implementation, the variable could have been renamed by
 * one of the pass. And hence, we search all local variables and check if
 * the variable is renamed. Note that we have to look into the symbol table
 * of statement block and not breakpoint.
 */
std::string CodegenCVisitor::breakpoint_current(std::string current) const {
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


int CodegenCVisitor::float_variables_size() const {
    auto count_length = [](int l, const SymbolType& variable) {
        return l += variable->get_length();
    };

    int float_size = std::accumulate(info.range_parameter_vars.begin(),
                                     info.range_parameter_vars.end(),
                                     0,
                                     count_length);
    float_size += std::accumulate(info.range_assigned_vars.begin(),
                                  info.range_assigned_vars.end(),
                                  0,
                                  count_length);
    float_size += std::accumulate(info.range_state_vars.begin(),
                                  info.range_state_vars.end(),
                                  0,
                                  count_length);
    float_size +=
        std::accumulate(info.assigned_vars.begin(), info.assigned_vars.end(), 0, count_length);

    /// all state variables for which we add Dstate variables
    float_size += std::accumulate(info.state_vars.begin(), info.state_vars.end(), 0, count_length);

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


int CodegenCVisitor::int_variables_size() const {
    int num_variables = 0;
    for (const auto& semantic: info.semantics) {
        num_variables += semantic.size;
    }
    return num_variables;
}


/**
 * \details Depending upon the block type, we have to print read/write ion variables
 * during code generation. Depending on block/procedure being printed, this
 * method return statements as vector. As different code backends could have
 * different variable names, we rely on backend-specific read_ion_variable_name
 * and write_ion_variable_name method which will be overloaded.
 *
 * \todo After looking into mod2c and neuron implementation, it seems like
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
            statements.push_back(fmt::format("{} = {};", first, second));
        }
        for (const auto& var: ion.writes) {
            if (type == BlockType::Ode && ion.is_ionic_conc(var) && state_variable(var)) {
                continue;
            }
            if (ion.is_ionic_conc(var)) {
                auto variables = read_ion_variable_name(var);
                auto first = get_variable_name(variables.first);
                auto second = get_variable_name(variables.second);
                statements.push_back(fmt::format("{} = {};", first, second));
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
                statements.push_back(fmt::format("{} = {};", first, second));
            }
        }
    }
    return statements;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
                        rhs += fmt::format("*(1.e2/{})", area);
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
                /// \todo Unhandled case in neuron implementation
                throw std::logic_error(fmt::format("codegen error for {} ion", ion.name));
            }
            auto ion_type_name = fmt::format("{}_type", ion.name);
            auto lhs = fmt::format("int {}", ion_type_name);
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
 * \details Often top level verbatim blocks use variables with old names.
 * Here we process if we are processing verbatim block at global scope.
 */
std::string CodegenCVisitor::process_verbatim_token(const std::string& token) {
    const std::string& name = token;

    /*
     * If given token is procedure name and if it's defined
     * in the current mod file then it must be replaced
     */
    if (program_symtab->is_method_defined(token)) {
        return method_name(token);
    }

    /*
     * Check if token is commongly used variable name in
     * verbatim block like nt, \c \_threadargs etc. If so, replace
     * it and return.
     */
    auto new_name = replace_if_verbatim_variable(name);
    if (new_name != name) {
        return get_variable_name(new_name, false);
    }

    /*
     * For top level verbatim blocks we shouldn't replace variable
     * names with Instance because arguments are provided from coreneuron
     * and they are missing inst.
     */
    auto use_instance = !printing_top_verbatim_blocks;
    return get_variable_name(token, use_instance);
}


bool CodegenCVisitor::ion_variable_struct_required() const {
    return optimize_ion_variable_copies() && info.ion_has_write_variable();
}


/**
 * \details This can be override in the backend. For example, parameters can be constant
 * except in INITIAL block where they are set to 0. As initial block is/can be
 * executed on c/cpu backend, gpu/cuda backend can mark the parameter as constant.
 */
bool CodegenCVisitor::is_constant_variable(const std::string& name) const {
    auto symbol = program_symtab->lookup_in_scope(name);
    bool is_constant = false;
    if (symbol != nullptr) {
        // per mechanism ion variables needs to be updated from neuron/coreneuron values
        if (info.is_ion_variable(name)) {
            is_constant = false;
        }
        // for parameter variable to be const, make sure it's write count is 0
        // and it's not used in the verbatim block
        else if (symbol->has_any_property(NmodlType::param_assign) &&
                 info.variables_in_verbatim.find(name) == info.variables_in_verbatim.end() &&
                 symbol->get_write_count() == 0) {
            is_constant = true;
        }
    }
    return is_constant;
}


/**
 * \details Once variables are populated, update index semantics to register with coreneuron
 */
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void CodegenCVisitor::update_index_semantics() {
    int index = 0;
    info.semantics.clear();

    if (info.point_process) {
        info.semantics.emplace_back(index++, naming::AREA_SEMANTIC, 1);
        info.semantics.emplace_back(index++, naming::POINT_PROCESS_SEMANTIC, 1);
    }
    for (const auto& ion: info.ions) {
        for (auto i = 0; i < ion.reads.size(); ++i) {
            info.semantics.emplace_back(index++, ion.name + "_ion", 1);
        }
        for (const auto& var: ion.writes) {
            /// add if variable is not present in the read list
            if (std::find(ion.reads.begin(), ion.reads.end(), var) == ion.reads.end()) {
                info.semantics.emplace_back(index++, ion.name + "_ion", 1);
            }
            if (ion.is_ionic_current(var)) {
                info.semantics.emplace_back(index++, ion.name + "_ion", 1);
            }
        }
        if (ion.need_style) {
            info.semantics.emplace_back(index++, fmt::format("#{}_ion", ion.name), 1);
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

    /*
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


std::vector<SymbolType> CodegenCVisitor::get_float_variables() {
    // sort with definition order
    auto comparator = [](const SymbolType& first, const SymbolType& second) -> bool {
        return first->get_definition_order() < second->get_definition_order();
    };

    auto assigned = info.assigned_vars;
    auto states = info.state_vars;

    // each state variable has corresponding Dstate variable
    for (auto& state: states) {
        auto name = "D" + state->get_name();
        auto symbol = make_symbol(name);
        if (state->is_array()) {
            symbol->set_as_array(state->get_length());
        }
        symbol->set_definition_order(state->get_definition_order());
        assigned.push_back(symbol);
    }
    std::sort(assigned.begin(), assigned.end(), comparator);

    auto variables = info.range_parameter_vars;
    variables.insert(variables.end(),
                     info.range_assigned_vars.begin(),
                     info.range_assigned_vars.end());
    variables.insert(variables.end(), info.range_state_vars.begin(), info.range_state_vars.end());
    variables.insert(variables.end(), assigned.begin(), assigned.end());

    if (info.vectorize) {
        variables.push_back(make_symbol(naming::VOLTAGE_UNUSED_VARIABLE));
    }

    if (breakpoint_exist()) {
        std::string name = info.vectorize ? naming::CONDUCTANCE_UNUSED_VARIABLE
                                          : naming::CONDUCTANCE_VARIABLE;

        // make sure conductance variable like `g` is not already defined
        if (auto r = std::find_if(variables.cbegin(),
                                  variables.cend(),
                                  [&](const auto& s) { return name == s->get_name(); });
            r == variables.cend()) {
            variables.push_back(make_symbol(name));
        }
    }

    if (net_receive_exist()) {
        variables.push_back(make_symbol(naming::T_SAVE_VARIABLE));
    }
    return variables;
}


/**
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
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
        std::unordered_map<std::string, int> ion_vars;  // used to keep track of the variables to
                                                        // not have doubles between read/write. Same
                                                        // name variables are allowed
        for (const auto& var: ion.reads) {
            const std::string name = naming::ION_VARNAME_PREFIX + var;
            variables.emplace_back(make_symbol(name));
            variables.back().is_constant = true;
            ion_vars[name] = static_cast<int>(variables.size() - 1);
        }

        /// symbol for di_ion_dv var
        std::shared_ptr<symtab::Symbol> ion_di_dv_var = nullptr;

        for (const auto& var: ion.writes) {
            const std::string name = naming::ION_VARNAME_PREFIX + var;

            const auto ion_vars_it = ion_vars.find(name);
            if (ion_vars_it != ion_vars.end()) {
                variables[ion_vars_it->second].is_constant = false;
            } else {
                variables.emplace_back(make_symbol(naming::ION_VARNAME_PREFIX + var));
            }
            if (ion.is_ionic_current(var)) {
                ion_di_dv_var = make_symbol(std::string(naming::ION_VARNAME_PREFIX) + "di" +
                                            ion.name + "dv");
            }
            if (ion.is_intra_cell_conc(var) || ion.is_extra_cell_conc(var)) {
                need_style = true;
            }
        }

        /// insert after read/write variables but before style ion variable
        if (ion_di_dv_var != nullptr) {
            variables.emplace_back(ion_di_dv_var);
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
        info.tqitem_index = static_cast<int>(variables.size() - 1);
    }

    /**
     * \note Variables for watch statements : there is one extra variable
     * used in coreneuron compared to actual watch statements for compatibility
     * with neuron (which uses one extra Datum variable)
     */
    if (!info.watch_statements.empty()) {
        for (int i = 0; i < info.watch_statements.size() + 1; i++) {
            variables.emplace_back(make_symbol(fmt::format("watch{}", i)), false, false, true);
        }
    }
    return variables;
}


/**
 * \details When we enable fine level parallelism at channel level, we have do updates
 * to ion variables in atomic way. As cpus don't have atomic instructions in
 * simd loop, we have to use shadow vectors for every ion variables. Here
 * we return list of all such variables.
 *
 * \todo If conductances are specified, we don't need all below variables
 */
std::vector<SymbolType> CodegenCVisitor::get_shadow_variables() {
    std::vector<SymbolType> variables;
    for (const auto& ion: info.ions) {
        for (const auto& var: ion.writes) {
            variables.push_back({make_symbol(shadow_varname(naming::ION_VARNAME_PREFIX + var))});
            if (ion.is_ionic_current(var)) {
                variables.push_back({make_symbol(shadow_varname(
                    std::string(naming::ION_VARNAME_PREFIX) + "di" + ion.name + "dv"))});
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

std::string CodegenCVisitor::get_parameter_str(const ParamVector& params) {
    std::string param{};
    for (auto iter = params.begin(); iter != params.end(); iter++) {
        param += fmt::format("{}{} {}{}",
                             std::get<0>(*iter),
                             std::get<1>(*iter),
                             std::get<2>(*iter),
                             std::get<3>(*iter));
        if (!nmodl::utils::is_last(iter, params)) {
            param += ", ";
        }
    }
    return param;
}


void CodegenCVisitor::print_channel_iteration_tiling_block_begin(BlockType /* type */) {
    // no tiling for cpu backend, just get loop bounds
    printer->add_line("int start = 0;");
    printer->add_line("int end = nodecount;");
}


void CodegenCVisitor::print_channel_iteration_tiling_block_end() {
    // backend specific, do nothing
}


void CodegenCVisitor::print_deriv_advance_flag_transfer_to_device() const {
    // backend specific, do nothing
}

void CodegenCVisitor::print_device_atomic_capture_annotation() const {
    // backend specific, do nothing
}

void CodegenCVisitor::print_net_send_buf_count_update_to_host() const {
    // backend specific, do nothing
}

void CodegenCVisitor::print_net_send_buf_update_to_host() const {
    // backend specific, do nothing
}

void CodegenCVisitor::print_net_send_buf_count_update_to_device() const {
    // backend specific, do nothing
}

void CodegenCVisitor::print_dt_update_to_device() const {
    // backend specific, do nothing
}

void CodegenCVisitor::print_device_stream_wait() const {
    // backend specific, do nothing
}

/**
 * \details Each kernel such as \c nrn\_init, \c nrn\_state and \c nrn\_cur could be offloaded
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


void CodegenCVisitor::print_kernel_data_present_annotation_block_end() {
    // backend specific, do nothing
}

void CodegenCVisitor::print_net_init_acc_serial_annotation_block_begin() {
    // backend specific, do nothing
}

void CodegenCVisitor::print_net_init_acc_serial_annotation_block_end() {
    // backend specific, do nothing
}

/**
 * \details Depending programming model and compiler, we print compiler hint
 * for parallelization. For example:
 *
 * \code
 *      #pragma ivdep
 *      for(int id = 0; id < nodecount; id++) {
 *
 *      #pragma acc parallel loop
 *      for(int id = 0; id < nodecount; id++) {
 * \endcode
 */
void CodegenCVisitor::print_channel_iteration_block_parallel_hint(BlockType /* type */) {
    printer->add_line("#pragma ivdep");
    printer->add_line("#pragma omp simd");
}


bool CodegenCVisitor::nrn_cur_reduction_loop_required() {
    return info.point_process;
}


/**
 * \details For CPU backend we iterate over all node counts. For cuda we use thread
 * index to check if block needs to be executed or not.
 */
void CodegenCVisitor::print_channel_iteration_block_begin(BlockType type) {
    print_channel_iteration_block_parallel_hint(type);
    printer->start_block("for (int id = start; id < end; id++)");
}


void CodegenCVisitor::print_channel_iteration_block_end() {
    printer->end_block(1);
}


void CodegenCVisitor::print_rhs_d_shadow_variables() {
    if (info.point_process) {
        printer->fmt_line("double* shadow_rhs = nt->{};", naming::NTHREAD_RHS_SHADOW);
        printer->fmt_line("double* shadow_d = nt->{};", naming::NTHREAD_D_SHADOW);
    }
}


void CodegenCVisitor::print_nrn_cur_matrix_shadow_update() {
    if (info.point_process) {
        printer->add_line("shadow_rhs[id] = rhs;");
        printer->add_line("shadow_d[id] = g;");
    } else {
        auto rhs_op = operator_for_rhs();
        auto d_op = operator_for_d();
        print_atomic_reduction_pragma();
        printer->fmt_line("vec_rhs[node_id] {} rhs;", rhs_op);
        print_atomic_reduction_pragma();
        printer->fmt_line("vec_d[node_id] {} g;", d_op);
    }
}


void CodegenCVisitor::print_nrn_cur_matrix_shadow_reduction() {
    auto rhs_op = operator_for_rhs();
    auto d_op = operator_for_d();
    if (info.point_process) {
        printer->add_line("int node_id = node_index[id];");
        print_atomic_reduction_pragma();
        printer->fmt_line("vec_rhs[node_id] {} shadow_rhs[id];", rhs_op);
        print_atomic_reduction_pragma();
        printer->fmt_line("vec_d[node_id] {} shadow_d[id];", d_op);
    }
}


void CodegenCVisitor::print_atomic_reduction_pragma() {
    // backend specific, do nothing
}


void CodegenCVisitor::print_shadow_reduction_block_begin() {
    printer->start_block("for (int id = start; id < end; id++)");
}


void CodegenCVisitor::print_shadow_reduction_statements() {
    for (const auto& statement: shadow_statements) {
        print_atomic_reduction_pragma();
        auto lhs = get_variable_name(statement.lhs);
        auto rhs = get_variable_name(shadow_varname(statement.lhs));
        printer->fmt_line("{} {} {};", lhs, statement.op, rhs);
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


std::string CodegenCVisitor::backend_name() const {
    return "C (api-compatibility)";
}


bool CodegenCVisitor::optimize_ion_variable_copies() const {
    return optimize_ionvar_copies;
}


void CodegenCVisitor::print_memory_allocation_routine() const {
    printer->add_newline(2);
    auto args = "size_t num, size_t size, size_t alignment = 16";
    printer->fmt_start_block("static inline void* mem_alloc({})", args);
    printer->add_line("void* ptr;");
    printer->add_line("posix_memalign(&ptr, alignment, num*size);");
    printer->add_line("memset(ptr, 0, size);");
    printer->add_line("return ptr;");
    printer->end_block(1);

    printer->add_newline(2);
    printer->start_block("static inline void mem_free(void* ptr)");
    printer->add_line("free(ptr);");
    printer->end_block(1);
}


void CodegenCVisitor::print_abort_routine() const {
    printer->add_newline(2);
    printer->start_block("static inline void coreneuron_abort()");
    printer->add_line("abort();");
    printer->end_block(1);
}


std::string CodegenCVisitor::compute_method_name(BlockType type) const {
    if (type == BlockType::Initial) {
        return method_name(naming::NRN_INIT_METHOD);
    }
    if (type == BlockType::Constructor) {
        return method_name(naming::NRN_CONSTRUCTOR_METHOD);
    }
    if (type == BlockType::Destructor) {
        return method_name(naming::NRN_DESTRUCTOR_METHOD);
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

/// Useful in ispc so that variables in the global struct get "uniform "
std::string CodegenCVisitor::global_var_struct_type_qualifier() {
    return "";
}

void CodegenCVisitor::print_global_var_struct_decl() {
    printer->fmt_line("{} {};", global_struct(), global_struct_instance());
}

/****************************************************************************************/
/*              printing routines for code generation                                   */
/****************************************************************************************/


void CodegenCVisitor::visit_watch_statement(const ast::WatchStatement& /* node */) {
    printer->add_text(fmt::format("nrn_watch_activate(inst, id, pnodecount, {}, v, watch_remove)",
                                  current_watch_statement++));
}


void CodegenCVisitor::print_statement_block(const ast::StatementBlock& node,
                                            bool open_brace,
                                            bool close_brace) {
    if (open_brace) {
        printer->start_block();
    }

    auto statements = node.get_statements();
    for (const auto& statement: statements) {
        if (statement_to_skip(*statement)) {
            continue;
        }
        /// not necessary to add indent for verbatim block (pretty-printing)
        if (!statement->is_verbatim()) {
            printer->add_indent();
        }
        statement->accept(*this);
        if (need_semicolon(statement.get())) {
            printer->add_text(";");
        }
        printer->add_newline();
    }

    if (close_brace) {
        printer->end_block();
    }
}


void CodegenCVisitor::print_function_call(const FunctionCall& node) {
    auto name = node.get_node_name();
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

    auto arguments = node.get_arguments();
    printer->fmt_text("{}(", function_name);

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
            block->accept(*this);
        }
    }

    printing_top_verbatim_blocks = false;
    codegen = false;
    print_namespace_start();
}


/**
 * \todo Issue with verbatim renaming. e.g. pattern.mod has info struct with
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
                function->accept(v);
            }
        }
        for (const auto& function: info.procedures) {
            if (has_parameter_of_name(function, arg)) {
                function->accept(v);
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
        print_function_declaration(*node, node->get_node_name());
        printer->add_text(";");
        printer->add_newline();
    }
    for (const auto& node: info.procedures) {
        print_function_declaration(*node, node->get_node_name());
        printer->add_text(";");
        printer->add_newline();
    }
    codegen = false;
}


static const TableStatement* get_table_statement(const ast::Block& node) {
    // TableStatementVisitor v;

    const auto& table_statements = collect_nodes(node, {AstNodeType::TABLE_STATEMENT});

    if (table_statements.size() != 1) {
        auto message = fmt::format("One table statement expected in {} found {}",
                                   node.get_node_name(),
                                   table_statements.size());
        throw std::runtime_error(message);
    }
    return dynamic_cast<const TableStatement*>(table_statements.front().get());
}


std::tuple<bool, int> CodegenCVisitor::check_if_var_is_array(const std::string& name) {
    auto symbol = program_symtab->lookup_in_scope(name);
    if (!symbol) {
        throw std::runtime_error(
            fmt::format("CodegenCVisitor:: {} not found in symbol table!", name));
    }
    if (symbol->is_array()) {
        return {true, symbol->get_length()};
    } else {
        return {false, 0};
    }
}


void CodegenCVisitor::print_table_check_function(const Block& node) {
    auto statement = get_table_statement(node);
    auto table_variables = statement->get_table_vars();
    auto depend_variables = statement->get_depend_vars();
    const auto& from = statement->get_from();
    const auto& to = statement->get_to();
    auto name = node.get_node_name();
    auto internal_params = internal_method_parameters();
    auto with = statement->get_with()->eval();
    auto use_table_var = get_variable_name(naming::USE_TABLE_VARIABLE);
    auto tmin_name = get_variable_name("tmin_" + name);
    auto mfac_name = get_variable_name("mfac_" + name);
    auto float_type = default_float_data_type();

    printer->add_newline(2);
    print_device_method_annotation();
    printer->fmt_start_block("void check_{}({})",
                             method_name(name),
                             get_parameter_str(internal_params));
    {
        printer->fmt_start_block("if ({} == 0)", use_table_var);
        printer->add_line("return;");
        printer->end_block(1);

        printer->add_line("static bool make_table = true;");
        for (const auto& variable: depend_variables) {
            printer->fmt_line("static {} save_{};", float_type, variable->get_node_name());
        }

        for (const auto& variable: depend_variables) {
            auto name = variable->get_node_name();
            auto instance_name = get_variable_name(name);
            printer->fmt_start_block("if (save_{} != {})", name, instance_name);
            printer->add_line("make_table = true;");
            printer->end_block(1);
        }

        printer->start_block("if (make_table)");
        {
            printer->add_line("make_table = false;");

            printer->add_indent();
            printer->fmt_text("{} = ", tmin_name);
            from->accept(*this);
            printer->add_text(";");
            printer->add_newline();

            printer->add_indent();
            printer->add_text("double tmax = ");
            to->accept(*this);
            printer->add_text(";");
            printer->add_newline();


            printer->fmt_line("double dx = (tmax-{}) / {}.;", tmin_name, with);
            printer->fmt_line("{} = 1./dx;", mfac_name);

            printer->fmt_line("double x = {};", tmin_name);
            printer->fmt_start_block("for (std::size_t i = 0; i < {}; x += dx, i++)", with + 1);
            auto function = method_name("f_" + name);
            if (node.is_procedure_block()) {
                printer->fmt_line("{}({}, x);", function, internal_method_arguments());
                for (const auto& variable: table_variables) {
                    auto name = variable->get_node_name();
                    auto instance_name = get_variable_name(name);
                    auto table_name = get_variable_name("t_" + name);
                    auto [is_array, array_length] = check_if_var_is_array(name);
                    if (is_array) {
                        for (int j = 0; j < array_length; j++) {
                            printer->fmt_line(
                                "{}[{}][i] = {}[{}];", table_name, j, instance_name, j);
                        }
                    } else {
                        printer->fmt_line("{}[i] = {};", table_name, instance_name);
                    }
                }
            } else {
                auto table_name = get_variable_name("t_" + name);
                printer->fmt_line("{}[i] = {}({}, x);",
                                  table_name,
                                  function,
                                  internal_method_arguments());
            }
            printer->end_block(1);

            for (const auto& variable: depend_variables) {
                auto name = variable->get_node_name();
                auto instance_name = get_variable_name(name);
                printer->fmt_line("save_{} = {};", name, instance_name);
            }
        }
        printer->end_block(1);
    }
    printer->end_block(1);
}


void CodegenCVisitor::print_table_replacement_function(const ast::Block& node) {
    auto name = node.get_node_name();
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
        const auto& params = node.get_parameters();
        printer->fmt_start_block("if ({} == 0)", use_table_var);
        if (node.is_procedure_block()) {
            printer->fmt_line("{}({}, {});",
                              function_name,
                              internal_method_arguments(),
                              params[0].get()->get_node_name());
            printer->add_line("return 0;");
        } else {
            printer->fmt_line("return {}({}, {});",
                              function_name,
                              internal_method_arguments(),
                              params[0].get()->get_node_name());
        }
        printer->end_block(1);

        printer->fmt_line("double xi = {} * ({} - {});",
                          mfac_name,
                          params[0].get()->get_node_name(),
                          tmin_name);
        printer->start_block("if (isnan(xi))");
        if (node.is_procedure_block()) {
            for (const auto& var: table_variables) {
                auto name = get_variable_name(var->get_node_name());
                auto [is_array, array_length] = check_if_var_is_array(var->get_node_name());
                if (is_array) {
                    for (int j = 0; j < array_length; j++) {
                        printer->fmt_line("{}[{}] = xi;", name, j);
                    }
                } else {
                    printer->fmt_line("{} = xi;", name);
                }
            }
            printer->add_line("return 0;");
        } else {
            printer->add_line("return xi;");
        }
        printer->end_block(1);

        printer->fmt_start_block("if (xi <= 0. || xi >= {}.)", with);
        printer->fmt_line("int index = (xi <= 0.) ? 0 : {};", with);
        if (node.is_procedure_block()) {
            for (const auto& variable: table_variables) {
                auto name = variable->get_node_name();
                auto instance_name = get_variable_name(name);
                auto table_name = get_variable_name("t_" + name);
                auto [is_array, array_length] = check_if_var_is_array(name);
                if (is_array) {
                    for (int j = 0; j < array_length; j++) {
                        printer->fmt_line(
                            "{}[{}] = {}[{}][index];", instance_name, j, table_name, j);
                    }
                } else {
                    printer->fmt_line("{} = {}[index];", instance_name, table_name);
                }
            }
            printer->add_line("return 0;");
        } else {
            auto table_name = get_variable_name("t_" + name);
            printer->fmt_line("return {}[index];", table_name);
        }
        printer->end_block(1);

        printer->add_line("int i = int(xi);");
        printer->add_line("double theta = xi - double(i);");
        if (node.is_procedure_block()) {
            for (const auto& var: table_variables) {
                auto name = var->get_node_name();
                auto instance_name = get_variable_name(name);
                auto table_name = get_variable_name("t_" + name);
                auto [is_array, array_length] = check_if_var_is_array(var->get_node_name());
                if (is_array) {
                    for (size_t j = 0; j < array_length; j++) {
                        printer->fmt_line(
                            "{0}[{1}] = {2}[{1}][i] + theta*({2}[{1}][i+1]-{2}[{1}][i]);",
                            instance_name,
                            j,
                            table_name);
                    }
                } else {
                    printer->fmt_line("{0} = {1}[i] + theta*({1}[i+1]-{1}[i]);",
                                      instance_name,
                                      table_name);
                }
            }
            printer->add_line("return 0;");
        } else {
            auto table_name = get_variable_name("t_" + name);
            printer->fmt_line("return {0}[i] + theta * ({0}[i+1] - {0}[i]);", table_name);
        }
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

    printer->fmt_start_block("static void {} ({})", name, parameters);
    printer->add_line("setup_instance(nt, ml);");
    printer->fmt_line("auto* const inst = static_cast<{0}*>(ml->instance);", instance_struct());
    printer->add_line("double v = 0;");

    for (const auto& function: info.functions_with_table) {
        auto name = method_name("check_" + function->get_node_name());
        auto arguments = internal_method_arguments();
        printer->fmt_line("{}({});", name, arguments);
    }

    printer->end_block(1);
}


void CodegenCVisitor::print_function_or_procedure(const ast::Block& node, const std::string& name) {
    printer->add_newline(2);
    print_function_declaration(node, name);
    printer->add_text(" ");
    printer->start_block();

    // function requires return variable declaration
    if (node.is_function_block()) {
        auto type = default_float_data_type();
        printer->fmt_line("{} ret_{} = 0.0;", type, name);
    } else {
        printer->fmt_line("int ret_{} = 0;", name);
    }

    print_statement_block(*node.get_statement_block(), false, false);
    printer->fmt_line("return ret_{};", name);
    printer->end_block(1);
}


void CodegenCVisitor::print_function_procedure_helper(const ast::Block& node) {
    codegen = true;
    auto name = node.get_node_name();

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


void CodegenCVisitor::print_procedure(const ast::ProcedureBlock& node) {
    print_function_procedure_helper(node);
}


void CodegenCVisitor::print_function(const ast::FunctionBlock& node) {
    auto name = node.get_node_name();

    // name of return variable
    std::string return_var;
    if (info.function_uses_table(name)) {
        return_var = "ret_f_" + name;
    } else {
        return_var = "ret_" + name;
    }

    // first rename return variable name
    auto block = node.get_statement_block().get();
    RenameVisitor v(name, return_var);
    block->accept(v);

    print_function_procedure_helper(node);
}

/**
 * @brief Checks whether the functor_block generated by sympy solver modifies any variable outside
 * its scope. If it does then return false, so that the operator() of the struct functor of the
 * Eigen Newton solver doesn't have const qualifier.
 *
 * @param variable_block Statement Block of the variables declarations used in the functor struct of
 *                       the solver
 * @param functor_block Actual code being printed in the operator() of the functor struct of the
 *                      solver
 * @return True if operator() is const else False
 */
bool is_functor_const(const ast::StatementBlock& variable_block,
                      const ast::StatementBlock& functor_block) {
    // Save DUChain for every variable in variable_block
    std::unordered_map<std::string, DUChain> chains;

    // Create complete_block with both variable declarations (done in variable_block) and solver
    // part (done in functor_block) to be able to run the SymtabVisitor and DefUseAnalyzeVisitor
    // then and get the proper DUChains for the variables defined in the variable_block
    ast::StatementBlock complete_block(functor_block);
    // Typically variable_block has only one statement, a statement containing the declaration
    // of the local variables
    for (const auto& statement: variable_block.get_statements()) {
        complete_block.insert_statement(complete_block.get_statements().begin(), statement);
    }

    // Create Symbol Table for complete_block
    auto model_symbol_table = std::make_shared<symtab::ModelSymbolTable>();
    SymtabVisitor(model_symbol_table.get()).visit_statement_block(complete_block);
    // Initialize DefUseAnalyzeVisitor to generate the DUChains for the variables defined in the
    // variable_block
    DefUseAnalyzeVisitor v(*complete_block.get_symbol_table());

    // Check the DUChains for all the variables in the variable_block
    // If variable is defined in complete_block don't add const quilifier in operator()
    auto is_functor_const = true;
    const auto& variables = collect_nodes(variable_block, {ast::AstNodeType::LOCAL_VAR});
    for (const auto& variable: variables) {
        const auto& chain = v.analyze(complete_block, variable->get_node_name());
        is_functor_const = !(chain.eval() == DUState::D || chain.eval() == DUState::LD ||
                             chain.eval() == DUState::CD);
        if (!is_functor_const) {
            break;
        }
    }

    return is_functor_const;
}

void CodegenCVisitor::visit_eigen_newton_solver_block(const ast::EigenNewtonSolverBlock& node) {
    // solution vector to store copy of state vars for Newton solver
    printer->add_newline();

    auto float_type = default_float_data_type();
    int N = node.get_n_state_vars()->get_value();
    printer->fmt_line("Eigen::Matrix<{}, {}, 1> nmodl_eigen_xm;", float_type, N);
    printer->fmt_line("{}* nmodl_eigen_x = nmodl_eigen_xm.data();", float_type);

    print_statement_block(*node.get_setup_x_block(), false, false);

    // functor that evaluates F(X) and J(X) for
    // Newton solver
    printer->start_block("struct functor");
    printer->add_line("NrnThread* nt;");
    printer->fmt_line("{0}* inst;", instance_struct());
    printer->add_line("int id, pnodecount;");
    printer->add_line("double v;");
    printer->add_line("Datum* indexes;");
    printer->add_line("double* data;");
    printer->add_line("ThreadDatum* thread;");

    if (ion_variable_struct_required()) {
        print_ion_variable();
    }

    print_statement_block(*node.get_variable_block(), false, false);
    printer->add_newline();

    printer->start_block("void initialize()");
    print_statement_block(*node.get_initialize_block(), false, false);
    printer->end_block(2);

    printer->fmt_line(
        "functor(NrnThread* nt, {}* inst, int id, int pnodecount, double v, Datum* indexes, "
        "double* data, ThreadDatum* thread) : "
        "nt{{nt}}, inst{{inst}}, id{{id}}, pnodecount{{pnodecount}}, v{{v}}, indexes{{indexes}}, "
        "data{{data}}, thread{{thread}} "
        "{{}}",
        instance_struct());

    printer->add_indent();

    const auto& variable_block = *node.get_variable_block();
    const auto& functor_block = *node.get_functor_block();

    printer->fmt_text(
        "void operator()(const Eigen::Matrix<{0}, {1}, 1>& nmodl_eigen_xm, Eigen::Matrix<{0}, {1}, "
        "1>& nmodl_eigen_fm, "
        "Eigen::Matrix<{0}, {1}, {1}>& nmodl_eigen_jm) {2}",
        float_type,
        N,
        is_functor_const(variable_block, functor_block) ? "const " : "");
    printer->start_block();
    printer->fmt_line("const {}* nmodl_eigen_x = nmodl_eigen_xm.data();", float_type);
    printer->fmt_line("{}* nmodl_eigen_j = nmodl_eigen_jm.data();", float_type);
    printer->fmt_line("{}* nmodl_eigen_f = nmodl_eigen_fm.data();", float_type);
    print_statement_block(functor_block, false, false);
    printer->end_block(2);

    // assign newton solver results in matrix X to state vars
    printer->start_block("void finalize()");
    print_statement_block(*node.get_finalize_block(), false, false);
    printer->end_block(1);

    printer->end_block(";");

    // call newton solver with functor and X matrix that contains state vars
    printer->add_line("// call newton solver");
    printer->add_line(
        "functor newton_functor(nt, inst, id, pnodecount, v, indexes, data, thread);");
    printer->add_line("newton_functor.initialize();");
    printer->add_line(
        "int newton_iterations = nmodl::newton::newton_solver(nmodl_eigen_xm, newton_functor);");

    // assign newton solver results in matrix X to state vars
    print_statement_block(*node.get_update_states_block(), false, false);
    printer->add_line("newton_functor.finalize();");
}

void CodegenCVisitor::visit_eigen_linear_solver_block(const ast::EigenLinearSolverBlock& node) {
    printer->add_newline();

    const std::string float_type = default_float_data_type();
    int N = node.get_n_state_vars()->get_value();
    printer->fmt_line("Eigen::Matrix<{0}, {1}, 1> nmodl_eigen_xm, nmodl_eigen_fm;", float_type, N);
    printer->fmt_line("Eigen::Matrix<{0}, {1}, {1}> nmodl_eigen_jm;", float_type, N);
    printer->fmt_line("{}* nmodl_eigen_x = nmodl_eigen_xm.data();", float_type);
    printer->fmt_line("{}* nmodl_eigen_j = nmodl_eigen_jm.data();", float_type);
    printer->fmt_line("{}* nmodl_eigen_f = nmodl_eigen_fm.data();", float_type);
    print_statement_block(*node.get_variable_block(), false, false);
    print_statement_block(*node.get_initialize_block(), false, false);
    print_statement_block(*node.get_setup_x_block(), false, false);

    printer->add_newline();
    print_eigen_linear_solver(float_type, N);
    printer->add_newline();

    print_statement_block(*node.get_update_states_block(), false, false);
    print_statement_block(*node.get_finalize_block(), false, false);
}

void CodegenCVisitor::print_eigen_linear_solver(const std::string& float_type, int N) {
    if (N <= 4) {
        // Faster compared to LU, given the template specialization in Eigen.
        printer->add_line("nmodl_eigen_xm = nmodl_eigen_jm.inverse()*nmodl_eigen_fm;");
    } else {
        printer->fmt_line(
            "nmodl_eigen_xm = Eigen::PartialPivLU<Eigen::Ref<Eigen::Matrix<{0}, {1}, "
            "{1}>>>(nmodl_eigen_jm).solve(nmodl_eigen_fm);",
            float_type,
            N);
    }
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


/**
 * @todo: figure out how to correctly handle qualifiers
 */
CodegenCVisitor::ParamVector CodegenCVisitor::internal_method_parameters() {
    auto params = ParamVector();
    params.emplace_back("", "int", "", "id");
    params.emplace_back(param_type_qualifier(), "int", "", "pnodecount");
    params.emplace_back(param_type_qualifier(),
                        fmt::format("{}*", instance_struct()),
                        param_ptr_qualifier(),
                        "inst");
    if (ion_variable_struct_required()) {
        params.emplace_back("", "IonCurVar&", "", "ionvar");
    }
    params.emplace_back("", "double*", "", "data");
    params.emplace_back("const ", "Datum*", "", "indexes");
    params.emplace_back(param_type_qualifier(), "ThreadDatum*", "", "thread");
    params.emplace_back(param_type_qualifier(), "NrnThread*", param_ptr_qualifier(), "nt");
    params.emplace_back("", "double", "", "v");
    return params;
}


std::string CodegenCVisitor::external_method_arguments() {
    return "id, pnodecount, data, indexes, thread, nt, ml, v";
}


std::string CodegenCVisitor::external_method_parameters(bool table) {
    if (table) {
        return "int id, int pnodecount, double* data, Datum* indexes, "
               "ThreadDatum* thread, NrnThread* nt, Memb_list* ml, int tml_id";
    }
    return "int id, int pnodecount, double* data, Datum* indexes, "
           "ThreadDatum* thread, NrnThread* nt, Memb_list* ml, double v";
}


std::string CodegenCVisitor::nrn_thread_arguments() {
    if (ion_variable_struct_required()) {
        return "id, pnodecount, ionvar, data, indexes, thread, nt, ml, v";
    }
    return "id, pnodecount, data, indexes, thread, nt, ml, v";
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
 * Replace commonly used variables in the verbatim blocks into their corresponding
 * variable name in the new code generation backend.
 */
std::string CodegenCVisitor::replace_if_verbatim_variable(std::string name) {
    if (naming::VERBATIM_VARIABLES_MAPPING.find(name) != naming::VERBATIM_VARIABLES_MAPPING.end()) {
        name = naming::VERBATIM_VARIABLES_MAPPING.at(name);
    }

    /**
     * if function is defined the same mod file then the arguments must
     * contain mechanism instance as well.
     */
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
std::string CodegenCVisitor::process_verbatim_text(std::string const& text) {
    parser::CDriver driver;
    driver.scan_string(text);
    auto tokens = driver.all_tokens();
    std::string result;
    for (size_t i = 0; i < tokens.size(); i++) {
        auto token = tokens[i];

        // check if we have function call in the verbatim block where
        // function is defined in the same mod file
        if (program_symtab->is_method_defined(token) && tokens[i + 1] == "(") {
            internal_method_call_encountered = true;
        }
        auto name = process_verbatim_token(token);

        if (token == (std::string("_") + naming::TQITEM_VARIABLE)) {
            name.insert(0, 1, '&');
        }
        if (token == "_STRIDE") {
            name = "pnodecount+id";
        }
        result += name;
    }
    return result;
}


std::string CodegenCVisitor::register_mechanism_arguments() const {
    auto nrn_cur = nrn_cur_required() ? method_name(naming::NRN_CUR_METHOD) : "nullptr";
    auto nrn_state = nrn_state_required() ? method_name(naming::NRN_STATE_METHOD) : "nullptr";
    auto nrn_alloc = method_name(naming::NRN_ALLOC_METHOD);
    auto nrn_init = method_name(naming::NRN_INIT_METHOD);
    auto const nrn_private_constructor = method_name(naming::NRN_PRIVATE_CONSTRUCTOR_METHOD);
    auto const nrn_private_destructor = method_name(naming::NRN_PRIVATE_DESTRUCTOR_METHOD);
    return fmt::format("mechanism, {}, {}, nullptr, {}, {}, {}, {}, first_pointer_var_index()",
                       nrn_alloc,
                       nrn_cur,
                       nrn_state,
                       nrn_init,
                       nrn_private_constructor,
                       nrn_private_destructor);
}


std::pair<std::string, std::string> CodegenCVisitor::read_ion_variable_name(
    const std::string& name) {
    return {name, naming::ION_VARNAME_PREFIX + name};
}


std::pair<std::string, std::string> CodegenCVisitor::write_ion_variable_name(
    const std::string& name) {
    return {naming::ION_VARNAME_PREFIX + name, name};
}


std::string CodegenCVisitor::conc_write_statement(const std::string& ion_name,
                                                  const std::string& concentration,
                                                  int index) {
    auto conc_var_name = get_variable_name(naming::ION_VARNAME_PREFIX + concentration);
    auto style_var_name = get_variable_name("style_" + ion_name);
    return fmt::format(
        "nrn_wrote_conc({}_type,"
        " &({}),"
        " {},"
        " {},"
        " nrn_ion_global_map,"
        " {},"
        " nt->_ml_list[{}_type]->_nodecount_padded)",
        ion_name,
        conc_var_name,
        index,
        style_var_name,
        get_variable_name(naming::CELSIUS_VARIABLE),
        ion_name);
}


/**
 * If mechanisms dependency level execution is enabled then certain updates
 * like ionic current contributions needs to be atomically updated. In this
 * case we first update current mechanism's shadow vector and then add statement
 * to queue that will be used in reduction queue.
 */
std::string CodegenCVisitor::process_shadow_update_statement(const ShadowUseStatement& statement,
                                                             BlockType /* type */) {
    // when there is no operator or rhs then that statement doesn't need shadow update
    if (statement.op.empty() && statement.rhs.empty()) {
        auto text = statement.lhs + ";";
        return text;
    }

    // return regular statement
    auto lhs = get_variable_name(statement.lhs);
    auto text = fmt::format("{} {} {};", lhs, statement.op, statement.rhs);
    return text;
}


/****************************************************************************************/
/*               Code-specific printing routines for code generation                    */
/****************************************************************************************/


/**
 * NMODL constants from unit database
 *
 */
void CodegenCVisitor::print_nmodl_constants() {
    if (!info.factor_definitions.empty()) {
        printer->add_newline(2);
        printer->add_line("/** constants used in nmodl from UNITS */");
        for (const auto& it: info.factor_definitions) {
#ifdef USE_LEGACY_UNITS
            const std::string format_string = "static const double {} = {:g};";
#else
            const std::string format_string = "static const double {} = {:.18g};";
#endif
            printer->fmt_line(format_string,
                              it->get_node_name(),
                              stod(it->get_value()->get_value()));
        }
    }
}


void CodegenCVisitor::print_first_pointer_var_index_getter() {
    printer->add_newline(2);
    print_device_method_annotation();
    printer->start_block("static inline int first_pointer_var_index()");
    printer->fmt_line("return {};", info.first_pointer_var_index);
    printer->end_block(1);
}


void CodegenCVisitor::print_num_variable_getter() {
    printer->add_newline(2);
    print_device_method_annotation();
    printer->start_block("static inline int float_variables_size()");
    printer->fmt_line("return {};", float_variables_size());
    printer->end_block(1);

    printer->add_newline(2);
    print_device_method_annotation();
    printer->start_block("static inline int int_variables_size()");
    printer->fmt_line("return {};", int_variables_size());
    printer->end_block(1);
}


void CodegenCVisitor::print_net_receive_arg_size_getter() {
    if (!net_receive_exist()) {
        return;
    }
    printer->add_newline(2);
    print_device_method_annotation();
    printer->start_block("static inline int num_net_receive_args()");
    printer->fmt_line("return {};", info.num_net_receive_parameters);
    printer->end_block(1);
}


void CodegenCVisitor::print_mech_type_getter() {
    printer->add_newline(2);
    print_device_method_annotation();
    printer->start_block("static inline int get_mech_type()");
    // false => get it from the host-only global struct, not the instance structure
    printer->fmt_line("return {};", get_variable_name("mech_type", false));
    printer->end_block(1);
}


void CodegenCVisitor::print_memb_list_getter() {
    printer->add_newline(2);
    print_device_method_annotation();
    printer->start_block("static inline Memb_list* get_memb_list(NrnThread* nt)");
    printer->start_block("if (!nt->_ml_list)");
    printer->add_line("return nullptr;");
    printer->end_block(1);
    printer->add_line("return nt->_ml_list[get_mech_type()];");
    printer->end_block(1);
}


void CodegenCVisitor::print_namespace_start() {
    printer->add_newline(2);
    printer->start_block("namespace coreneuron");
}


void CodegenCVisitor::print_namespace_stop() {
    printer->end_block(1);
}


/**
 * \details There are three types of thread variables currently considered:
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
    if (info.vectorize && info.derivimplicit_used()) {
        int tid = info.derivimplicit_var_thread_id;
        int list = info.derivimplicit_list_num;

        // clang-format off
        printer->add_newline(2);
        printer->add_line("/** thread specific helper routines for derivimplicit */");

        printer->add_newline(1);
        printer->fmt_start_block("static inline int* deriv{}_advance(ThreadDatum* thread)", list);
        printer->fmt_line("return &(thread[{}].i);", tid);
        printer->end_block(2);

        printer->fmt_start_block("static inline int dith{}()", list);
        printer->fmt_line("return {};", tid+1);
        printer->end_block(2);

        printer->fmt_start_block("static inline void** newtonspace{}(ThreadDatum* thread)", list);
        printer->fmt_line("return &(thread[{}]._pvoid);", tid+2);
        printer->end_block(1);
    }

    if (info.vectorize && !info.thread_variables.empty()) {
        printer->add_newline(2);
        printer->add_line("/** tid for thread variables */");
        printer->start_block("static inline int thread_var_tid()");
        printer->fmt_line("return {};", info.thread_var_thread_id);
        printer->end_block(1);
    }

    if (info.vectorize && !info.top_local_variables.empty()) {
        printer->add_newline(2);
        printer->add_line("/** tid for top local tread variables */");
        printer->start_block("static inline int top_local_var_tid()");
        printer->fmt_line("return {};", info.top_local_thread_id);
        printer->end_block(1);
    }
    // clang-format on
}


/****************************************************************************************/
/*                         Routines for returning variable name                         */
/****************************************************************************************/


std::string CodegenCVisitor::float_variable_name(const SymbolType& symbol,
                                                 bool use_instance) const {
    auto name = symbol->get_name();
    auto dimension = symbol->get_length();
    auto position = position_of_float_var(name);
    // clang-format off
    if (symbol->is_array()) {
        if (use_instance) {
            return fmt::format("(inst->{}+id*{})", name, dimension);
        }
        return fmt::format("(data + {}*pnodecount + id*{})", position, dimension);
    }
    if (use_instance) {
        return fmt::format("inst->{}[id]", name);
    }
    return fmt::format("data[{}*pnodecount + id]", position);
    // clang-format on
}


std::string CodegenCVisitor::int_variable_name(const IndexVariableInfo& symbol,
                                               const std::string& name,
                                               bool use_instance) const {
    auto position = position_of_int_var(name);
    // clang-format off
    if (symbol.is_index) {
        if (use_instance) {
            return fmt::format("inst->{}[{}]", name, position);
        }
        return fmt::format("indexes[{}]", position);
    }
    if (symbol.is_integer) {
        if (use_instance) {
            return fmt::format("inst->{}[{}*pnodecount+id]", name, position);
        }
        return fmt::format("indexes[{}*pnodecount+id]", position);
    }
    if (use_instance) {
        return fmt::format("inst->{}[indexes[{}*pnodecount + id]]", name, position);
    }
    auto data = symbol.is_vdata ? "_vdata" : "_data";
    return fmt::format("nt->{}[indexes[{}*pnodecount + id]]", data, position);
    // clang-format on
}


std::string CodegenCVisitor::global_variable_name(const SymbolType& symbol,
                                                  bool use_instance) const {
    if (use_instance) {
        return fmt::format("inst->{}->{}", naming::INST_GLOBAL_MEMBER, symbol->get_name());
    } else {
        return fmt::format("{}.{}", global_struct_instance(), symbol->get_name());
    }
}


std::string CodegenCVisitor::ion_shadow_variable_name(const SymbolType& symbol) {
    return fmt::format("inst->{}[id]", symbol->get_name());
}


std::string CodegenCVisitor::update_if_ion_variable_name(const std::string& name) const {
    std::string result(name);
    if (ion_variable_struct_required()) {
        if (info.is_ion_read_variable(name)) {
            result = naming::ION_VARNAME_PREFIX + name;
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


std::string CodegenCVisitor::get_variable_name(const std::string& name, bool use_instance) const {
    std::string varname = update_if_ion_variable_name(name);

    // clang-format off
    auto symbol_comparator = [&varname](const SymbolType& sym) {
                            return varname == sym->get_name();
                         };

    auto index_comparator = [&varname](const IndexVariableInfo& var) {
                            return varname == var.symbol->get_name();
                         };
    // clang-format on

    // float variable
    auto f = std::find_if(codegen_float_variables.begin(),
                          codegen_float_variables.end(),
                          symbol_comparator);
    if (f != codegen_float_variables.end()) {
        return float_variable_name(*f, use_instance);
    }

    // integer variable
    auto i =
        std::find_if(codegen_int_variables.begin(), codegen_int_variables.end(), index_comparator);
    if (i != codegen_int_variables.end()) {
        return int_variable_name(*i, varname, use_instance);
    }

    // global variable
    auto g = std::find_if(codegen_global_variables.begin(),
                          codegen_global_variables.end(),
                          symbol_comparator);
    if (g != codegen_global_variables.end()) {
        return global_variable_name(*g, use_instance);
    }

    // shadow variable
    auto s = std::find_if(codegen_shadow_variables.begin(),
                          codegen_shadow_variables.end(),
                          symbol_comparator);
    if (s != codegen_shadow_variables.end()) {
        return ion_shadow_variable_name(*s);
    }

    if (varname == naming::NTHREAD_DT_VARIABLE) {
        return std::string("nt->_") + naming::NTHREAD_DT_VARIABLE;
    }

    // t in net_receive method is an argument to function and hence it should
    // ne used instead of nt->_t which is current time of thread
    if (varname == naming::NTHREAD_T_VARIABLE && !printing_net_receive) {
        return std::string("nt->_") + naming::NTHREAD_T_VARIABLE;
    }

    auto const iter =
        std::find_if(info.neuron_global_variables.begin(),
                     info.neuron_global_variables.end(),
                     [&varname](auto const& entry) { return entry.first->get_name() == varname; });
    if (iter != info.neuron_global_variables.end()) {
        std::string ret;
        if (use_instance) {
            ret = "*(inst->";
        }
        ret.append(varname);
        if (use_instance) {
            ret.append(")");
        }
        return ret;
    }

    // otherwise return original name
    return varname;
}


/****************************************************************************************/
/*                      Main printing routines for code generation                      */
/****************************************************************************************/


void CodegenCVisitor::print_backend_info() {
    time_t tr{};
    time(&tr);
    auto date = std::string(asctime(localtime(&tr)));
    auto version = nmodl::Version::NMODL_VERSION + " [" + nmodl::Version::GIT_REVISION + "]";

    printer->add_line("/*********************************************************");
    printer->fmt_line("Model Name      : {}", info.mod_suffix);
    printer->fmt_line("Filename        : {}", info.mod_file + ".mod");
    printer->fmt_line("NMODL Version   : {}", nmodl_version());
    printer->fmt_line("Vectorized      : {}", info.vectorize);
    printer->fmt_line("Threadsafe      : {}", info.thread_safe);
    printer->fmt_line("Created         : {}", stringutils::trim(date));
    printer->fmt_line("Backend         : {}", backend_name());
    printer->fmt_line("NMODL Compiler  : {}", version);
    printer->add_line("*********************************************************/");
}


void CodegenCVisitor::print_standard_includes() {
    printer->add_newline();
    printer->add_line("#include <math.h>");
    printer->add_line("#include \"nmodl/fast_math.hpp\" // extend math with some useful functions");
    printer->add_line("#include <stdio.h>");
    printer->add_line("#include <stdlib.h>");
    printer->add_line("#include <string.h>");
}


void CodegenCVisitor::print_coreneuron_includes() {
    printer->add_newline();
    printer->add_line("#include <coreneuron/nrnconf.h>");
    printer->add_line("#include <coreneuron/sim/multicore.hpp>");
    printer->add_line("#include <coreneuron/mechanism/register_mech.hpp>");
    printer->add_line("#include <coreneuron/gpu/nrn_acc_manager.hpp>");
    printer->add_line("#include <coreneuron/utils/randoms/nrnran123.h>");
    printer->add_line("#include <coreneuron/nrniv/nrniv_decl.h>");
    printer->add_line("#include <coreneuron/utils/ivocvect.hpp>");
    printer->add_line("#include <coreneuron/utils/nrnoc_aux.hpp>");
    printer->add_line("#include <coreneuron/mechanism/mech/mod2c_core_thread.hpp>");
    printer->add_line("#include <coreneuron/sim/scopmath/newton_thread.hpp>");
    if (info.eigen_newton_solver_exist) {
        printer->add_line("#include <newton/newton.hpp>");
    }
    if (info.eigen_linear_solver_exist) {
        printer->add_line("#include <Eigen/LU>");
    }
}


/**
 * \details Variables required for type of ion, type of point process etc. are
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
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void CodegenCVisitor::print_mechanism_global_var_structure(bool print_initialisers) {
    const auto value_initialise = print_initialisers ? "{}" : "";
    const auto qualifier = global_var_struct_type_qualifier();

    auto float_type = default_float_data_type();
    printer->add_newline(2);
    printer->add_line("/** all global variables */");
    printer->fmt_start_block("struct {}", global_struct());

    for (const auto& ion: info.ions) {
        auto name = fmt::format("{}_type", ion.name);
        printer->fmt_line("{}int {}{};", qualifier, name, value_initialise);
        codegen_global_variables.push_back(make_symbol(name));
    }

    if (info.point_process) {
        printer->fmt_line("{}int point_type{};", qualifier, value_initialise);
        codegen_global_variables.push_back(make_symbol("point_type"));
    }

    for (const auto& var: info.state_vars) {
        auto name = var->get_name() + "0";
        auto symbol = program_symtab->lookup(name);
        if (symbol == nullptr) {
            printer->fmt_line("{}{} {}{};", qualifier, float_type, name, value_initialise);
            codegen_global_variables.push_back(make_symbol(name));
        }
    }

    // Neuron and Coreneuron adds "v" to global variables when vectorize
    // is false. But as v is always local variable and passed as argument,
    // we don't need to use global variable v

    auto& top_locals = info.top_local_variables;
    if (!info.vectorize && !top_locals.empty()) {
        for (const auto& var: top_locals) {
            auto name = var->get_name();
            auto length = var->get_length();
            if (var->is_array()) {
                printer->fmt_line("{}{} {}[{}] /* TODO init top-local-array */;",
                                  qualifier,
                                  float_type,
                                  name,
                                  length);
            } else {
                printer->fmt_line("{}{} {} /* TODO init top-local */;",
                                  qualifier,
                                  float_type,
                                  name);
            }
            codegen_global_variables.push_back(var);
        }
    }

    if (!info.thread_variables.empty()) {
        printer->fmt_line("{}int thread_data_in_use{};", qualifier, value_initialise);
        printer->fmt_line("{}{} thread_data[{}] /* TODO init thread_data */;",
                          qualifier,
                          float_type,
                          info.thread_var_data_size);
        codegen_global_variables.push_back(make_symbol("thread_data_in_use"));
        auto symbol = make_symbol("thread_data");
        symbol->set_as_array(info.thread_var_data_size);
        codegen_global_variables.push_back(symbol);
    }

    // TODO: remove this entirely?
    printer->fmt_line("{}int reset{};", qualifier, value_initialise);
    codegen_global_variables.push_back(make_symbol("reset"));

    printer->fmt_line("{}int mech_type{};", qualifier, value_initialise);
    codegen_global_variables.push_back(make_symbol("mech_type"));

    for (const auto& var: info.global_variables) {
        auto name = var->get_name();
        auto length = var->get_length();
        if (var->is_array()) {
            printer->fmt_line(
                "{}{} {}[{}] /* TODO init const-array */;", qualifier, float_type, name, length);
        } else {
            double value{};
            if (auto const& value_ptr = var->get_value()) {
                value = *value_ptr;
            }
            printer->fmt_line("{}{} {}{};",
                              qualifier,
                              float_type,
                              name,
                              print_initialisers ? fmt::format("{{{:g}}}", value) : std::string{});
        }
        codegen_global_variables.push_back(var);
    }

    for (const auto& var: info.constant_variables) {
        auto const name = var->get_name();
        auto* const value_ptr = var->get_value().get();
        double const value{value_ptr ? *value_ptr : 0};
        printer->fmt_line("{}{} {}{};",
                          qualifier,
                          float_type,
                          name,
                          print_initialisers ? fmt::format("{{{:g}}}", value) : std::string{});
        codegen_global_variables.push_back(var);
    }

    if (info.primes_size != 0) {
        if (info.primes_size != info.prime_variables_by_order.size()) {
            throw std::runtime_error{
                fmt::format("primes_size = {} differs from prime_variables_by_order.size() = {}, "
                            "this should not happen.",
                            info.primes_size,
                            info.prime_variables_by_order.size())};
        }
        auto const initializer_list = [&](auto const& primes, const char* prefix) -> std::string {
            if (!print_initialisers) {
                return {};
            }
            std::string list{"{"};
            for (auto iter = primes.begin(); iter != primes.end(); ++iter) {
                auto const& prime = *iter;
                list.append(std::to_string(position_of_float_var(prefix + prime->get_name())));
                if (std::next(iter) != primes.end()) {
                    list.append(", ");
                }
            }
            list.append("}");
            return list;
        };
        printer->fmt_line("{}int slist1[{}]{};",
                          qualifier,
                          info.primes_size,
                          initializer_list(info.prime_variables_by_order, ""));
        printer->fmt_line("{}int dlist1[{}]{};",
                          qualifier,
                          info.primes_size,
                          initializer_list(info.prime_variables_by_order, "D"));
        codegen_global_variables.push_back(make_symbol("slist1"));
        codegen_global_variables.push_back(make_symbol("dlist1"));
        // additional list for derivimplicit method
        if (info.derivimplicit_used()) {
            auto primes = program_symtab->get_variables_with_properties(NmodlType::prime_name);
            printer->fmt_line("{}int slist2[{}]{};",
                              qualifier,
                              info.primes_size,
                              initializer_list(primes, ""));
            codegen_global_variables.push_back(make_symbol("slist2"));
        }
    }

    if (info.table_count > 0) {
        printer->fmt_line("{}double usetable{};", qualifier, print_initialisers ? "{1}" : "");
        codegen_global_variables.push_back(make_symbol(naming::USE_TABLE_VARIABLE));

        for (const auto& block: info.functions_with_table) {
            auto name = block->get_node_name();
            printer->fmt_line("{}{} tmin_{}{};", qualifier, float_type, name, value_initialise);
            printer->fmt_line("{}{} mfac_{}{};", qualifier, float_type, name, value_initialise);
            codegen_global_variables.push_back(make_symbol("tmin_" + name));
            codegen_global_variables.push_back(make_symbol("mfac_" + name));
        }

        for (const auto& variable: info.table_statement_variables) {
            auto const name = "t_" + variable->get_name();
            auto const num_values = variable->get_num_values();
            if (variable->is_array()) {
                int array_len = variable->get_length();
                printer->fmt_line("{}{} {}[{}][{}]{};",
                                  qualifier,
                                  float_type,
                                  name,
                                  array_len,
                                  num_values,
                                  value_initialise);
            } else {
                printer->fmt_line(
                    "{}{} {}[{}]{};", qualifier, float_type, name, num_values, value_initialise);
            }
            codegen_global_variables.push_back(make_symbol(name));
        }
    }

    if (info.vectorize && info.thread_data_index) {
        printer->fmt_line("{}ThreadDatum ext_call_thread[{}]{};",
                          qualifier,
                          info.thread_data_index,
                          value_initialise);
        codegen_global_variables.push_back(make_symbol("ext_call_thread"));
    }

    printer->end_block(";");

    print_global_var_struct_assertions();
    print_global_var_struct_decl();
}

void CodegenCVisitor::print_global_var_struct_assertions() const {
    // Assert some things that we assume when copying instances of this struct
    // to the GPU and so on.
    printer->fmt_line("static_assert(std::is_trivially_copy_constructible_v<{}>);",
                      global_struct());
    printer->fmt_line("static_assert(std::is_trivially_move_constructible_v<{}>);",
                      global_struct());
    printer->fmt_line("static_assert(std::is_trivially_copy_assignable_v<{}>);", global_struct());
    printer->fmt_line("static_assert(std::is_trivially_move_assignable_v<{}>);", global_struct());
    printer->fmt_line("static_assert(std::is_trivially_destructible_v<{}>);", global_struct());
}


void CodegenCVisitor::print_prcellstate_macros() const {
    printer->add_line("#ifndef NRN_PRCELLSTATE");
    printer->add_line("#define NRN_PRCELLSTATE 0");
    printer->add_line("#endif");
}


void CodegenCVisitor::print_mechanism_info() {
    auto variable_printer = [&](std::vector<SymbolType>& variables) {
        for (const auto& v: variables) {
            auto name = v->get_name();
            if (!info.point_process) {
                name += "_" + info.mod_suffix;
            }
            if (v->is_array()) {
                name += fmt::format("[{}]", v->get_length());
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
    variable_printer(info.range_assigned_vars);
    printer->add_line("0,");
    variable_printer(info.range_state_vars);
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
                    // false => do not use the instance struct, which is not
                    // defined in the global declaration that we are printing
                    auto name = get_variable_name(variable->get_name(), false);
                    auto ename = add_escape_quote(variable->get_name() + "_" + info.mod_suffix);
                    auto length = variable->get_length();
                    if (if_vector) {
                        printer->fmt_line("{{{}, {}, {}}},", ename, name, length);
                    } else {
                        printer->fmt_line("{{{}, &{}}},", ename, name);
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
    printer->add_line("{nullptr, nullptr}");
    printer->decrease_indent();
    printer->add_line("};");

    printer->add_newline(2);
    printer->add_line("/** connect global (array) variables to hoc -- */");
    printer->add_line("static DoubVec hoc_vector_double[] = {");
    printer->increase_indent();
    variable_printer(globals, true, true);
    variable_printer(thread_vars, true, true);
    printer->add_line("{nullptr, nullptr, 0}");
    printer->decrease_indent();
    printer->add_line("};");
}

/**
 * Return registration type for a given BEFORE/AFTER block
 * /param block A BEFORE/AFTER block being registered
 *
 * Depending on a block type i.e. BEFORE or AFTER and also type
 * of it's associated block i.e. BREAKPOINT, INITIAL, SOLVE and
 * STEP, the registration type (as an integer) is calculated.
 * These values are then interpreted by CoreNEURON internally.
 */
static size_t get_register_type_for_ba_block(const ast::Block* block) {
    size_t register_type = 0;
    BAType ba_type{};
    /// before block have value 10 and after block 20
    if (block->is_before_block()) {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        register_type = 10;
        ba_type =
            dynamic_cast<const ast::BeforeBlock*>(block)->get_bablock()->get_type()->get_value();
    } else {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        register_type = 20;
        ba_type =
            dynamic_cast<const ast::AfterBlock*>(block)->get_bablock()->get_type()->get_value();
    }

    /// associated blocks have different values (1 to 4) based on type.
    /// These values are based on neuron/coreneuron implementation details.
    if (ba_type == BATYPE_BREAKPOINT) {
        register_type += 1;
    } else if (ba_type == BATYPE_SOLVE) {
        register_type += 2;
    } else if (ba_type == BATYPE_INITIAL) {
        register_type += 3;
    } else if (ba_type == BATYPE_STEP) {
        register_type += 4;
    } else {
        throw std::runtime_error("Unhandled Before/After type encountered during code generation");
    }
    return register_type;
}


/**
 * \details Every mod file has register function to connect with the simulator.
 * Various information about mechanism and callbacks get registered with
 * the simulator using suffix_reg() function.
 *
 * Here are details:
 *  - We should exclude that callback based on the solver, watch statements.
 *  - If nrn_get_mechtype is < -1 means that mechanism is not used in the
 *    context of neuron execution and hence could be ignored in coreneuron
 *    execution.
 *  - Ions are internally defined and their types can be queried similar to
 *    other mechanisms.
 *  - hoc_register_var may not be needed in the context of coreneuron
 *  - We assume net receive buffer is on. This is because generated code is
 *    compatible for cpu as well as gpu target.
 */
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void CodegenCVisitor::print_mechanism_register() {
    printer->add_newline(2);
    printer->add_line("/** register channel with the simulator */");
    printer->fmt_start_block("void _{}_reg() ", info.mod_file);

    // type related information
    auto suffix = add_escape_quote(info.mod_suffix);
    printer->add_newline();
    printer->fmt_line("int mech_type = nrn_get_mechtype({});", suffix);
    printer->fmt_line("{} = mech_type;", get_variable_name("mech_type", false));
    printer->start_block("if (mech_type == -1)");
    printer->add_line("return;");
    printer->end_block(1);

    printer->add_newline();
    printer->add_line("_nrn_layout_reg(mech_type, 0);");  // 0 for SoA

    // register mechanism
    auto args = register_mechanism_arguments();
    auto nobjects = num_thread_objects();
    if (info.point_process) {
        printer->fmt_line("point_register_mech({}, {}, {}, {});",
                          args,
                          info.constructor_node ? method_name(naming::NRN_CONSTRUCTOR_METHOD)
                                                : "nullptr",
                          info.destructor_node ? method_name(naming::NRN_DESTRUCTOR_METHOD)
                                               : "nullptr",
                          nobjects);
    } else {
        printer->fmt_line("register_mech({}, {});", args, nobjects);
        if (info.constructor_node) {
            printer->fmt_line("register_constructor({});",
                              method_name(naming::NRN_CONSTRUCTOR_METHOD));
        }
    }

    // types for ion
    for (const auto& ion: info.ions) {
        printer->fmt_line("{} = nrn_get_mechtype({});",
                          get_variable_name(ion.name + "_type", false),
                          add_escape_quote(ion.name + "_ion"));
    }
    printer->add_newline();

    /*
     *  Register callbacks for thread allocation and cleanup. Note that thread_data_index
     *  represent total number of thread used minus 1 (i.e. index of last thread).
     */
    if (info.vectorize && (info.thread_data_index != 0)) {
        // false to avoid getting the copy from the instance structure
        printer->fmt_line("thread_mem_init({});", get_variable_name("ext_call_thread", false));
    }

    if (!info.thread_variables.empty()) {
        printer->fmt_line("{} = 0;", get_variable_name("thread_data_in_use"));
    }

    if (info.thread_callback_register) {
        printer->add_line("_nrn_thread_reg0(mech_type, thread_mem_cleanup);");
        printer->add_line("_nrn_thread_reg1(mech_type, thread_mem_init);");
    }

    if (info.emit_table_thread()) {
        auto name = method_name("check_table_thread");
        printer->fmt_line("_nrn_thread_table_reg(mech_type, {});", name);
    }

    // register read/write callbacks for pointers
    if (info.bbcore_pointer_used) {
        printer->add_line("hoc_reg_bbcore_read(mech_type, bbcore_read);");
        printer->add_line("hoc_reg_bbcore_write(mech_type, bbcore_write);");
    }

    // register size of double and int elements
    // clang-format off
    printer->add_line("hoc_register_prop_size(mech_type, float_variables_size(), int_variables_size());");
    // clang-format on

    // register semantics for index variables
    for (auto& semantic: info.semantics) {
        auto args =
            fmt::format("mech_type, {}, {}", semantic.index, add_escape_quote(semantic.name));
        printer->fmt_line("hoc_register_dparam_semantics({});", args);
    }

    if (info.is_watch_used()) {
        auto watch_fun = compute_method_name(BlockType::Watch);
        printer->fmt_line("hoc_register_watch_check({}, mech_type);", watch_fun);
    }

    if (info.write_concentration) {
        printer->add_line("nrn_writes_conc(mech_type, 0);");
    }

    // register various information for point process type
    if (info.net_event_used) {
        printer->add_line("add_nrn_has_net_event(mech_type);");
    }
    if (info.artificial_cell) {
        printer->fmt_line("add_nrn_artcell(mech_type, {});", info.tqitem_index);
    }
    if (net_receive_buffering_required()) {
        printer->fmt_line("hoc_register_net_receive_buffering({}, mech_type);",
                          method_name("net_buf_receive"));
    }
    if (info.num_net_receive_parameters != 0) {
        auto net_recv_init_arg = "nullptr";
        if (info.net_receive_initial_node != nullptr) {
            net_recv_init_arg = "net_init";
        }
        printer->fmt_line("set_pnt_receive(mech_type, {}, {}, num_net_receive_args());",
                          method_name("net_receive"),
                          net_recv_init_arg);
    }
    if (info.for_netcon_used) {
        // index where information about FOR_NETCON is stored in the integer array
        const auto index =
            std::find_if(info.semantics.begin(), info.semantics.end(), [](const IndexSemantics& a) {
                return a.name == naming::FOR_NETCON_SEMANTIC;
            })->index;
        printer->fmt_line("add_nrn_fornetcons(mech_type, {});", index);
    }

    if (info.net_event_used || info.net_send_used) {
        printer->add_line("hoc_register_net_send_buffering(mech_type);");
    }

    /// register all before/after blocks
    for (size_t i = 0; i < info.before_after_blocks.size(); i++) {
        // register type and associated function name for the block
        const auto& block = info.before_after_blocks[i];
        size_t register_type = get_register_type_for_ba_block(block);
        std::string function_name = method_name(fmt::format("nrn_before_after_{}", i));
        printer->fmt_line("hoc_reg_ba(mech_type, {}, {});", function_name, register_type);
    }

    // register variables for hoc
    printer->add_line("hoc_register_var(hoc_scalar_double, hoc_vector_double, NULL);");
    printer->end_block(1);
}


void CodegenCVisitor::print_thread_memory_callbacks() {
    if (!info.thread_callback_register) {
        return;
    }

    // thread_mem_init callback
    printer->add_newline(2);
    printer->add_line("/** thread memory allocation callback */");
    printer->start_block("static void thread_mem_init(ThreadDatum* thread) ");

    if (info.vectorize && info.derivimplicit_used()) {
        printer->fmt_line("thread[dith{}()].pval = nullptr;", info.derivimplicit_list_num);
    }
    if (info.vectorize && (info.top_local_thread_size != 0)) {
        auto length = info.top_local_thread_size;
        auto allocation = fmt::format("(double*)mem_alloc({}, sizeof(double))", length);
        printer->fmt_line("thread[top_local_var_tid()].pval = {};", allocation);
    }
    if (info.thread_var_data_size != 0) {
        auto length = info.thread_var_data_size;
        auto thread_data = get_variable_name("thread_data");
        auto thread_data_in_use = get_variable_name("thread_data_in_use");
        auto allocation = fmt::format("(double*)mem_alloc({}, sizeof(double))", length);
        printer->fmt_start_block("if ({})", thread_data_in_use);
        printer->fmt_line("thread[thread_var_tid()].pval = {};", allocation);
        printer->restart_block("else");
        printer->fmt_line("thread[thread_var_tid()].pval = {};", thread_data);
        printer->fmt_line("{} = 1;", thread_data_in_use);
        printer->end_block(1);
    }
    printer->end_block(3);


    // thread_mem_cleanup callback
    printer->add_line("/** thread memory cleanup callback */");
    printer->start_block("static void thread_mem_cleanup(ThreadDatum* thread) ");

    // clang-format off
    if (info.vectorize && info.derivimplicit_used()) {
        int n = info.derivimplicit_list_num;
        printer->fmt_line("free(thread[dith{}()].pval);", n);
        printer->fmt_line("nrn_destroy_newtonspace(static_cast<NewtonSpace*>(*newtonspace{}(thread)));", n);
    }
    // clang-format on

    if (info.top_local_thread_size != 0) {
        auto line = "free(thread[top_local_var_tid()].pval);";
        printer->add_line(line);
    }
    if (info.thread_var_data_size != 0) {
        auto thread_data = get_variable_name("thread_data");
        auto thread_data_in_use = get_variable_name("thread_data_in_use");
        printer->fmt_start_block("if (thread[thread_var_tid()].pval == {})", thread_data);
        printer->fmt_line("{} = 0;", thread_data_in_use);
        printer->restart_block("else");
        printer->add_line("free(thread[thread_var_tid()].pval);");
        printer->end_block(1);
    }
    printer->end_block(1);
}


void CodegenCVisitor::print_mechanism_range_var_structure(bool print_initialisers) {
    auto const value_initialise = print_initialisers ? "{}" : "";
    auto float_type = default_float_data_type();
    auto int_type = default_int_data_type();
    printer->add_newline(2);
    printer->add_line("/** all mechanism instance variables and global variables */");
    printer->fmt_start_block("struct {} ", instance_struct());

    for (auto const& [var, type]: info.neuron_global_variables) {
        auto const name = var->get_name();
        printer->fmt_line("{}* {}{}{};",
                          type,
                          ptr_type_qualifier(),
                          name,
                          print_initialisers ? fmt::format("{{&coreneuron::{}}}", name)
                                             : std::string{});
    }
    for (auto& var: codegen_float_variables) {
        auto name = var->get_name();
        auto type = get_range_var_float_type(var);
        auto qualifier = is_constant_variable(name) ? "const " : "";
        printer->fmt_line(
            "{}{}* {}{}{};", qualifier, type, ptr_type_qualifier(), name, value_initialise);
    }
    for (auto& var: codegen_int_variables) {
        auto name = var.symbol->get_name();
        if (var.is_index || var.is_integer) {
            auto qualifier = var.is_constant ? "const " : "";
            printer->fmt_line(
                "{}{}* {}{}{};", qualifier, int_type, ptr_type_qualifier(), name, value_initialise);
        } else {
            auto qualifier = var.is_constant ? "const " : "";
            auto type = var.is_vdata ? "void*" : default_float_data_type();
            printer->fmt_line(
                "{}{}* {}{}{};", qualifier, type, ptr_type_qualifier(), name, value_initialise);
        }
    }

    printer->fmt_line("{}* {}{};",
                      global_struct(),
                      naming::INST_GLOBAL_MEMBER,
                      print_initialisers ? fmt::format("{{&{}}}", global_struct_instance())
                                         : std::string{});
    printer->end_block(";");
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
            printer->fmt_line("{} {};", float_type, var);
            members.push_back(var);
        }
    }
    for (auto& var: info.currents) {
        if (!info.is_ion_variable(var)) {
            printer->fmt_line("{} {};", float_type, var);
            members.push_back(var);
        }
    }

    print_ion_var_constructor(members);

    printer->end_block(";");
}


void CodegenCVisitor::print_ion_var_constructor(const std::vector<std::string>& members) {
    // constructor
    printer->add_newline();
    printer->add_line("IonCurVar() : ", 0);
    for (int i = 0; i < members.size(); i++) {
        printer->fmt_text("{}(0)", members[i]);
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


void CodegenCVisitor::print_global_variable_device_update_annotation() {
    // nothing for cpu
}


void CodegenCVisitor::print_setup_range_variable() {
    auto type = float_data_type();
    printer->add_newline(2);
    printer->add_line("/** allocate and setup array for range variable */");
    printer->fmt_start_block("static inline {}* setup_range_variable(double* variable, int n)",
                             type);
    printer->fmt_line("{0}* data = ({0}*) mem_alloc(n, sizeof({0}));", type);
    printer->start_block("for(size_t i = 0; i < n; i++)");
    printer->add_line("data[i] = variable[i];");
    printer->end_block(1);
    printer->add_line("return data;");
    printer->end_block(1);
}


/**
 * \details If floating point type like "float" is specified on command line then
 * we can't turn all variables to new type. This is because certain variables
 * are pointers to internal variables (e.g. ions). Hence, we check if given
 * variable can be safely converted to new type. If so, return new type.
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

    printer->add_newline();
    printer->add_line("// Allocate instance structure");
    printer->fmt_start_block("static void {}(NrnThread* nt, Memb_list* ml, int type)",
                             method_name(naming::NRN_PRIVATE_CONSTRUCTOR_METHOD));
    printer->add_line("assert(!ml->instance);");
    printer->add_line("assert(!ml->global_variables);");
    printer->add_line("assert(ml->global_variables_size == 0);");
    printer->fmt_line("auto* const inst = new {}{{}};", instance_struct());
    printer->fmt_line("assert(inst->{} == &{});",
                      naming::INST_GLOBAL_MEMBER,
                      global_struct_instance());
    printer->add_line("ml->instance = inst;");
    printer->fmt_line("ml->global_variables = inst->{};", naming::INST_GLOBAL_MEMBER);
    printer->fmt_line("ml->global_variables_size = sizeof({});", global_struct());
    printer->end_block(2);

    auto const cast_inst_and_assert_validity = [&]() {
        printer->fmt_line("auto* const inst = static_cast<{}*>(ml->instance);", instance_struct());
        printer->add_line("assert(inst);");
        printer->fmt_line("assert(inst->{});", naming::INST_GLOBAL_MEMBER);
        printer->fmt_line("assert(inst->{} == &{});",
                          naming::INST_GLOBAL_MEMBER,
                          global_struct_instance());
        printer->fmt_line("assert(inst->{} == ml->global_variables);", naming::INST_GLOBAL_MEMBER);
        printer->fmt_line("assert(ml->global_variables_size == sizeof({}));", global_struct());
    };

    // Must come before print_instance_struct_copy_to_device and
    // print_instance_struct_delete_from_device
    print_instance_struct_transfer_routine_declarations();

    printer->add_line("// Deallocate the instance structure");
    printer->fmt_start_block("static void {}(NrnThread* nt, Memb_list* ml, int type)",
                             method_name(naming::NRN_PRIVATE_DESTRUCTOR_METHOD));
    cast_inst_and_assert_validity();
    print_instance_struct_delete_from_device();
    printer->add_line("delete inst;");
    printer->add_line("ml->instance = nullptr;");
    printer->add_line("ml->global_variables = nullptr;");
    printer->add_line("ml->global_variables_size = 0;");
    printer->end_block(2);


    printer->add_line("/** initialize mechanism instance variables */");
    printer->start_block("static inline void setup_instance(NrnThread* nt, Memb_list* ml)");
    cast_inst_and_assert_validity();

    std::string stride;
    printer->add_line("int pnodecount = ml->_nodecount_padded;");
    stride = "*pnodecount";

    printer->add_line("Datum* indexes = ml->pdata;");

    auto const float_type = default_float_data_type();

    int id = 0;
    std::vector<std::string> ptr_members{naming::INST_GLOBAL_MEMBER};
    for (auto const& [var, type]: info.neuron_global_variables) {
        ptr_members.push_back(var->get_name());
    }
    ptr_members.reserve(ptr_members.size() + codegen_float_variables.size() +
                        codegen_int_variables.size());
    for (auto& var: codegen_float_variables) {
        auto name = var->get_name();
        auto range_var_type = get_range_var_float_type(var);
        if (float_type == range_var_type) {
            auto const variable = fmt::format("ml->data+{}{}", id, stride);
            printer->fmt_line("inst->{} = {};", name, variable);
        } else {
            // TODO what MOD file exercises this?
            printer->fmt_line("inst->{} = setup_range_variable(ml->data+{}{}, pnodecount);",
                              name,
                              id,
                              stride);
        }
        ptr_members.push_back(std::move(name));
        id += var->get_length();
    }

    for (auto& var: codegen_int_variables) {
        auto name = var.symbol->get_name();
        auto const variable = [&var]() {
            if (var.is_index || var.is_integer) {
                return "ml->pdata";
            } else if (var.is_vdata) {
                return "nt->_vdata";
            } else {
                return "nt->_data";
            }
        }();
        printer->fmt_line("inst->{} = {};", name, variable);
        ptr_members.push_back(std::move(name));
    }
    print_instance_struct_copy_to_device();
    printer->end_block(2);  // setup_instance

    print_instance_struct_transfer_routines(ptr_members);
}


void CodegenCVisitor::print_initial_block(const InitialBlock* node) {
    if (info.artificial_cell) {
        printer->add_line("double v = 0.0;");
    } else {
        printer->add_line("int node_id = node_index[id];");
        printer->add_line("double v = voltage[node_id];");
        print_v_unused();
    }

    if (ion_variable_struct_required()) {
        printer->add_line("IonCurVar ionvar;");
    }

    // read ion statements
    auto read_statements = ion_read_statements(BlockType::Initial);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    // initialize state variables (excluding ion state)
    for (auto& var: info.state_vars) {
        auto name = var->get_name();
        if (!info.is_ionic_conc(name)) {
            auto lhs = get_variable_name(name);
            auto rhs = get_variable_name(name + "0");
            printer->fmt_line("{} = {};", lhs, rhs);
        }
    }

    // initial block
    if (node != nullptr) {
        const auto& block = node->get_statement_block();
        print_statement_block(*block, false, false);
    }

    // write ion statements
    auto write_statements = ion_write_statements(BlockType::Initial);
    for (auto& statement: write_statements) {
        auto text = process_shadow_update_statement(statement, BlockType::Initial);
        printer->add_line(text);
    }
}


void CodegenCVisitor::print_global_function_common_code(BlockType type,
                                                        const std::string& function_name) {
    std::string method;
    if (function_name.empty()) {
        method = compute_method_name(type);
    } else {
        method = function_name;
    }
    auto args = "NrnThread* nt, Memb_list* ml, int type";

    // watch statement function doesn't have type argument
    if (type == BlockType::Watch) {
        args = "NrnThread* nt, Memb_list* ml";
    }

    print_global_method_annotation();
    printer->fmt_start_block("void {}({})", method, args);
    if (type != BlockType::Destructor && type != BlockType::Constructor) {
        // We do not (currently) support DESTRUCTOR and CONSTRUCTOR blocks
        // running anything on the GPU.
        print_kernel_data_present_annotation_block_begin();
    } else {
        /// TODO: Remove this when the code generation is propery done
        /// Related to https://github.com/BlueBrain/nmodl/issues/692
        printer->add_line("#ifndef CORENEURON_BUILD");
    }
    printer->add_line("int nodecount = ml->nodecount;");
    printer->add_line("int pnodecount = ml->_nodecount_padded;");
    printer->fmt_line("const int* {}node_index = ml->nodeindices;", ptr_type_qualifier());
    printer->fmt_line("double* {}data = ml->data;", ptr_type_qualifier());
    printer->fmt_line("const double* {}voltage = nt->_actual_v;", ptr_type_qualifier());

    if (type == BlockType::Equation) {
        printer->fmt_line("double* {} vec_rhs = nt->_actual_rhs;", ptr_type_qualifier());
        printer->fmt_line("double* {} vec_d = nt->_actual_d;", ptr_type_qualifier());
        print_rhs_d_shadow_variables();
    }
    printer->fmt_line("Datum* {}indexes = ml->pdata;", ptr_type_qualifier());
    printer->fmt_line("ThreadDatum* {}thread = ml->_thread;", ptr_type_qualifier());

    if (type == BlockType::Initial) {
        printer->add_newline();
        printer->add_line("setup_instance(nt, ml);");
    }
    printer->fmt_line("auto* const {1}inst = static_cast<{0}*>(ml->instance);",
                      instance_struct(),
                      ptr_type_qualifier());
    printer->add_newline(1);
}

void CodegenCVisitor::print_nrn_init(bool skip_init_check) {
    codegen = true;
    printer->add_newline(2);
    printer->add_line("/** initialize channel */");

    print_global_function_common_code(BlockType::Initial);
    if (info.derivimplicit_used()) {
        printer->add_newline();
        int nequation = info.num_equations;
        int list_num = info.derivimplicit_list_num;
        // clang-format off
        printer->fmt_line("int& deriv_advance_flag = *deriv{}_advance(thread);", list_num);
        printer->add_line("deriv_advance_flag = 0;");
        print_deriv_advance_flag_transfer_to_device();
        printer->fmt_line("auto ns = newtonspace{}(thread);", list_num);
        printer->fmt_line("auto& th = thread[dith{}()];", list_num);
        printer->start_block("if (*ns == nullptr)");
        printer->fmt_line("int vec_size = 2*{}*pnodecount*sizeof(double);", nequation);
        printer->fmt_line("double* vec = makevector(vec_size);", nequation);
        printer->fmt_line("th.pval = vec;", list_num);
        printer->fmt_line("*ns = nrn_cons_newtonspace({}, pnodecount);", nequation);
        print_newtonspace_transfer_to_device();
        printer->end_block(1);
        // clang-format on
    }

    // update global variable as those might be updated via python/hoc API
    // NOTE: CoreNEURON has enough information to do this on its own, which
    // would be neater.
    print_global_variable_device_update_annotation();

    if (skip_init_check) {
        printer->start_block("if (_nrn_skip_initmodel == 0)");
    }

    if (!info.changed_dt.empty()) {
        printer->fmt_line("double _save_prev_dt = {};",
                          get_variable_name(naming::NTHREAD_DT_VARIABLE));
        printer->fmt_line("{} = {};",
                          get_variable_name(naming::NTHREAD_DT_VARIABLE),
                          info.changed_dt);
        print_dt_update_to_device();
    }

    print_channel_iteration_tiling_block_begin(BlockType::Initial);
    print_channel_iteration_block_begin(BlockType::Initial);

    if (info.net_receive_node != nullptr) {
        printer->fmt_line("{} = -1e20;", get_variable_name("tsave"));
    }

    print_initial_block(info.initial_node);
    print_channel_iteration_block_end();
    print_shadow_reduction_statements();
    print_channel_iteration_tiling_block_end();

    if (!info.changed_dt.empty()) {
        printer->fmt_line("{} = _save_prev_dt;", get_variable_name(naming::NTHREAD_DT_VARIABLE));
        print_dt_update_to_device();
    }

    printer->end_block(1);

    if (info.derivimplicit_used()) {
        printer->add_line("deriv_advance_flag = 1;");
        print_deriv_advance_flag_transfer_to_device();
    }

    if (info.net_send_used && !info.artificial_cell) {
        print_send_event_move();
    }

    print_kernel_data_present_annotation_block_end();
    if (skip_init_check) {
        printer->end_block(1);
    }
    codegen = false;
}

void CodegenCVisitor::print_before_after_block(const ast::Block* node, size_t block_id) {
    codegen = true;

    std::string ba_type;
    std::shared_ptr<ast::BABlock> ba_block;

    if (node->is_before_block()) {
        ba_block = dynamic_cast<const ast::BeforeBlock*>(node)->get_bablock();
        ba_type = "BEFORE";
    } else {
        ba_block = dynamic_cast<const ast::AfterBlock*>(node)->get_bablock();
        ba_type = "AFTER";
    }

    std::string ba_block_type = ba_block->get_type()->eval();

    /// name of the before/after function
    std::string function_name = method_name(fmt::format("nrn_before_after_{}", block_id));

    /// print common function code like init/state/current
    printer->add_newline(2);
    printer->fmt_line("/** {} of block type {} # {} */", ba_type, ba_block_type, block_id);
    print_global_function_common_code(BlockType::BeforeAfter, function_name);

    print_channel_iteration_tiling_block_begin(BlockType::BeforeAfter);
    print_channel_iteration_block_begin(BlockType::BeforeAfter);

    printer->add_line("int node_id = node_index[id];");
    printer->add_line("double v = voltage[node_id];");
    print_v_unused();

    // read ion statements
    const auto& read_statements = ion_read_statements(BlockType::Equation);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    /// print main body
    printer->add_indent();
    print_statement_block(*ba_block->get_statement_block());
    printer->add_newline();

    // write ion statements
    const auto& write_statements = ion_write_statements(BlockType::Equation);
    for (auto& statement: write_statements) {
        auto text = process_shadow_update_statement(statement, BlockType::Equation);
        printer->add_line(text);
    }

    /// loop end including data annotation block
    print_channel_iteration_block_end();
    print_channel_iteration_tiling_block_end();
    printer->end_block(1);
    print_kernel_data_present_annotation_block_end();

    codegen = false;
}

void CodegenCVisitor::print_nrn_constructor() {
    printer->add_newline(2);
    print_global_function_common_code(BlockType::Constructor);
    if (info.constructor_node != nullptr) {
        const auto& block = info.constructor_node->get_statement_block();
        print_statement_block(*block, false, false);
    }
    printer->add_line("#endif");
    printer->end_block(1);
}


void CodegenCVisitor::print_nrn_destructor() {
    printer->add_newline(2);
    print_global_function_common_code(BlockType::Destructor);
    if (info.destructor_node != nullptr) {
        const auto& block = info.destructor_node->get_statement_block();
        print_statement_block(*block, false, false);
    }
    printer->add_line("#endif");
    printer->end_block(1);
}


void CodegenCVisitor::print_nrn_alloc() {
    printer->add_newline(2);
    auto method = method_name(naming::NRN_ALLOC_METHOD);
    printer->fmt_start_block("static void {}(double* data, Datum* indexes, int type) ", method);
    printer->add_line("// do nothing");
    printer->end_block(1);
}

/**
 * \todo Number of watch could be more than number of statements
 * according to grammar. Check if this is correctly handled in neuron
 * and coreneuron.
 */
void CodegenCVisitor::print_watch_activate() {
    if (info.watch_statements.empty()) {
        return;
    }
    codegen = true;
    printer->add_newline(2);
    auto inst = fmt::format("{}* inst", instance_struct());

    printer->fmt_start_block(
        "static void nrn_watch_activate({}, int id, int pnodecount, int watch_id, "
        "double v, bool &watch_remove)",
        inst);

    // initialize all variables only during first watch statement
    printer->start_block("if (watch_remove == false)");
    for (int i = 0; i < info.watch_count; i++) {
        auto name = get_variable_name(fmt::format("watch{}", i + 1));
        printer->fmt_line("{} = 0;", name);
    }
    printer->add_line("watch_remove = true;");
    printer->end_block(1);

    /**
     * \todo Similar to neuron/coreneuron we are using
     * first watch and ignoring rest.
     */
    for (int i = 0; i < info.watch_statements.size(); i++) {
        auto statement = info.watch_statements[i];
        printer->fmt_start_block("if (watch_id == {})", i);

        auto varname = get_variable_name(fmt::format("watch{}", i + 1));
        printer->add_indent();
        printer->fmt_text("{} = 2 + (", varname);
        auto watch = statement->get_statements().front();
        watch->get_expression()->visit_children(*this);
        printer->add_text(");");
        printer->add_newline();

        printer->end_block(1);
    }
    printer->end_block(1);
    codegen = false;
}


/**
 * \todo Similar to print_watch_activate, we are using only
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

    if (info.is_voltage_used_by_watch_statements()) {
        printer->add_line("int node_id = node_index[id];");
        printer->add_line("double v = voltage[node_id];");
        print_v_unused();
    }

    // flat to make sure only one WATCH statement can be triggered at a time
    printer->add_line("bool watch_untriggered = true;");

    for (int i = 0; i < info.watch_statements.size(); i++) {
        auto statement = info.watch_statements[i];
        auto watch = statement->get_statements().front();
        auto varname = get_variable_name(fmt::format("watch{}", i + 1));

        // start block 1
        printer->fmt_start_block("if ({}&2 && watch_untriggered)", varname);

        // start block 2
        printer->add_indent();
        printer->add_text("if (");
        watch->get_expression()->accept(*this);
        printer->add_text(") {");
        printer->add_newline();
        printer->increase_indent();

        // start block 3
        printer->fmt_start_block("if (({}&1) == 0)", varname);

        printer->add_line("watch_untriggered = false;");

        auto tqitem = get_variable_name("tqitem");
        auto point_process = get_variable_name("point_process");
        printer->add_indent();
        printer->add_text("net_send_buffering(");
        auto t = get_variable_name("t");
        printer->add_text(
            fmt::format("ml->_net_send_buffer, 0, {}, -1, {}, {}+0.0, ", tqitem, point_process, t));
        watch->get_value()->accept(*this);
        printer->add_text(");");
        printer->add_newline();
        printer->end_block(1);

        printer->fmt_line("{} = 3;", varname);
        // end block 3

        // start block 3
        printer->decrease_indent();
        printer->start_block("} else");
        printer->fmt_line("{} = 2;", varname);
        printer->end_block(1);
        // end block 3

        printer->end_block(1);
        // end block 1
    }

    print_channel_iteration_block_end();
    print_send_event_move();
    print_channel_iteration_tiling_block_end();
    print_kernel_data_present_annotation_block_end();
    printer->end_block(1);
    codegen = false;
}


void CodegenCVisitor::print_net_receive_common_code(const Block& node, bool need_mech_inst) {
    printer->add_line("int tid = pnt->_tid;");
    printer->add_line("int id = pnt->_i_instance;");
    printer->add_line("double v = 0;");
    if (info.artificial_cell || node.is_initial_block()) {
        printer->add_line("NrnThread* nt = nrn_threads + tid;");
        printer->add_line("Memb_list* ml = nt->_ml_list[pnt->_type];");
    }
    if (node.is_initial_block()) {
        print_kernel_data_present_annotation_block_begin();
    }

    printer->fmt_line("{}int nodecount = ml->nodecount;", param_type_qualifier());
    printer->fmt_line("{}int pnodecount = ml->_nodecount_padded;", param_type_qualifier());
    printer->add_line("double* data = ml->data;");
    printer->add_line("double* weights = nt->weights;");
    printer->add_line("Datum* indexes = ml->pdata;");
    printer->add_line("ThreadDatum* thread = ml->_thread;");
    if (need_mech_inst) {
        printer->fmt_line("auto* const inst = static_cast<{0}*>(ml->instance);", instance_struct());
    }

    if (node.is_initial_block()) {
        print_net_init_acc_serial_annotation_block_begin();
    }

    // rename variables but need to see if they are actually used
    auto parameters = info.net_receive_node->get_parameters();
    if (!parameters.empty()) {
        int i = 0;
        printer->add_newline();
        for (auto& parameter: parameters) {
            auto name = parameter->get_node_name();
            bool var_used = VarUsageVisitor().variable_used(node, "(*" + name + ")");
            if (var_used) {
                printer->fmt_line("double* {} = weights + weight_index + {};", name, i);
                RenameVisitor vr(name, "*" + name);
                node.visit_children(vr);
            }
            i++;
        }
    }
}


void CodegenCVisitor::print_net_send_call(const FunctionCall& node) {
    auto const& arguments = node.get_arguments();
    auto tqitem = get_variable_name("tqitem");
    std::string weight_index = "weight_index";
    std::string pnt = "pnt";

    // for non-net_receieve functions i.e. initial block, the weight_index argument is 0.
    if (!printing_net_receive) {
        weight_index = "0";
        auto var = get_variable_name("point_process");
        if (info.artificial_cell) {
            pnt = "(Point_process*)" + var;
        }
    }

    // artificial cells don't use spike buffering
    // clang-format off
    if (info.artificial_cell) {
        printer->fmt_text("artcell_net_send(&{}, {}, {}, nt->_t+", tqitem, weight_index, pnt);
    } else {
        auto point_process = get_variable_name("point_process");
        std::string t = get_variable_name("t");
        printer->add_text("net_send_buffering(");
        printer->fmt_text("ml->_net_send_buffer, 0, {}, {}, {}, {}+", tqitem, weight_index, point_process, t);
    }
    // clang-format off
    print_vector_elements(arguments, ", ");
    printer->add_text(")");
}


void CodegenCVisitor::print_net_move_call(const FunctionCall& node) {
    if (!printing_net_receive) {
        std::cout << "Error : net_move only allowed in NET_RECEIVE block" << std::endl;
        abort();
    }

    auto const& arguments = node.get_arguments();
    auto tqitem = get_variable_name("tqitem");
    std::string weight_index = "-1";
    std::string pnt = "pnt";

    // artificial cells don't use spike buffering
    // clang-format off
    if (info.artificial_cell) {
        printer->fmt_text("artcell_net_move(&{}, {}, ", tqitem, pnt);
        print_vector_elements(arguments, ", ");
        printer->add_text(")");
    } else {
        auto point_process = get_variable_name("point_process");
        std::string t = get_variable_name("t");
        printer->add_text("net_send_buffering(");
        printer->fmt_text("ml->_net_send_buffer, 2, {}, {}, {}, ", tqitem, weight_index, point_process);
        print_vector_elements(arguments, ", ");
        printer->add_text(", 0.0");
        printer->add_text(")");
    }
}


void CodegenCVisitor::print_net_event_call(const FunctionCall& node) {
    const auto& arguments = node.get_arguments();
    if (info.artificial_cell) {
        printer->add_text("net_event(pnt, ");
        print_vector_elements(arguments, ", ");
    } else {
        auto point_process = get_variable_name("point_process");
        printer->add_text("net_send_buffering(");
        printer->fmt_text("ml->_net_send_buffer, 1, -1, -1, {}, ", point_process);
        print_vector_elements(arguments, ", ");
        printer->add_text(", 0.0");
    }
    printer->add_text(")");
}

/**
 * Rename arguments to NET_RECEIVE block with corresponding pointer variable
 *
 * Arguments to NET_RECEIVE block are packed and passed via weight vector. These
 * variables need to be replaced with corresponding pointer variable. For example,
 * if mod file is like
 *
 * \code{.mod}
 *      NET_RECEIVE (weight, R){
 *          INITIAL {
 *              R=1
 *          }
 *      }
 * \endcode
 *
 * then generated code for initial block should be:
 *
 * \code{.cpp}
 *      double* R = weights + weight_index + 0;
 *      (*R) = 1.0;
 * \endcode
 *
 * So, the `R` in AST needs to be renamed with `(*R)`.
 */
static void rename_net_receive_arguments(const ast::NetReceiveBlock& net_receive_node, const ast::Node& node) {
    auto parameters = net_receive_node.get_parameters();
    for (auto& parameter: parameters) {
        auto name = parameter->get_node_name();
        auto var_used = VarUsageVisitor().variable_used(node, name);
        if (var_used) {
            RenameVisitor vr(name, "(*" + name + ")");
            node.get_statement_block()->visit_children(vr);
        }
    }
}


void CodegenCVisitor::print_net_init() {
    const auto node = info.net_receive_initial_node;
    if (node == nullptr) {
        return;
    }

    // rename net_receive arguments used in the initial block of net_receive
    rename_net_receive_arguments(*info.net_receive_node, *node);

    codegen = true;
    auto args = "Point_process* pnt, int weight_index, double flag";
    printer->add_newline(2);
    printer->add_line("/** initialize block for net receive */");
    printer->fmt_start_block("static void net_init({}) ", args);
    auto block = node->get_statement_block().get();
    if (block->get_statements().empty()) {
        printer->add_line("// do nothing");
    } else {
        print_net_receive_common_code(*node);
        print_statement_block(*block, false, false);
        if (node->is_initial_block()) {
            print_net_init_acc_serial_annotation_block_end();
            print_kernel_data_present_annotation_block_end();
            printer->add_line("auto& nsb = ml->_net_send_buffer;");
            print_net_send_buf_update_to_host();
        }
    }
    printer->end_block(1);
    codegen = false;
}


void CodegenCVisitor::print_send_event_move() {
    printer->add_newline();
    printer->add_line("NetSendBuffer_t* nsb = ml->_net_send_buffer;");
    print_net_send_buf_update_to_host();
    printer->start_block("for (int i=0; i < nsb->_cnt; i++)");
    printer->add_line("int type = nsb->_sendtype[i];");
    printer->add_line("int tid = nt->id;");
    printer->add_line("double t = nsb->_nsb_t[i];");
    printer->add_line("double flag = nsb->_nsb_flag[i];");
    printer->add_line("int vdata_index = nsb->_vdata_index[i];");
    printer->add_line("int weight_index = nsb->_weight_index[i];");
    printer->add_line("int point_index = nsb->_pnt_index[i];");
    printer->add_line("net_sem_from_gpu(type, vdata_index, weight_index, tid, point_index, t, flag);");
    printer->end_block(1);
    printer->add_line("nsb->_cnt = 0;");
    print_net_send_buf_count_update_to_device();
}


std::string CodegenCVisitor::net_receive_buffering_declaration() {
    return fmt::format("void {}(NrnThread* nt)", method_name("net_buf_receive"));
}


void CodegenCVisitor::print_get_memb_list() {
    printer->add_line("Memb_list* ml = get_memb_list(nt);");
    printer->start_block("if (!ml)");
    printer->add_line("return;");
    printer->end_block(2);
}


void CodegenCVisitor::print_net_receive_loop_begin() {
    printer->add_line("int count = nrb->_displ_cnt;");
    print_channel_iteration_block_parallel_hint(BlockType::NetReceive);
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

    print_kernel_data_present_annotation_block_begin();

    printer->fmt_line("NetReceiveBuffer_t* {}nrb = ml->_net_receive_buffer;", ptr_type_qualifier());
    if (need_mech_inst) {
        printer->fmt_line("auto* const inst = static_cast<{0}*>(ml->instance);", instance_struct());
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
    printer->fmt_line("{}(t, point_process, inst, nt, ml, weight_index, flag);", net_receive);
    printer->end_block(1);
    print_net_receive_loop_end();

    print_device_stream_wait();
    printer->add_line("nrb->_displ_cnt = 0;");
    printer->add_line("nrb->_cnt = 0;");

    if (info.net_send_used || info.net_event_used) {
        print_send_event_move();
    }

    printer->add_newline();
    print_kernel_data_present_annotation_block_end();
    printer->end_block(1);
}

void CodegenCVisitor::print_net_send_buffering_grow() {
    printer->start_block("if(i >= nsb->_size)");
    printer->add_line("nsb->grow();");
    printer->end_block(1);
}

void CodegenCVisitor::print_net_send_buffering() {
    if (!net_send_buffer_required()) {
        return;
    }

    printer->add_newline(2);
    print_device_method_annotation();
    auto args =
        "NetSendBuffer_t* nsb, int type, int vdata_index, "
        "int weight_index, int point_index, double t, double flag";
    printer->fmt_start_block("static inline void net_send_buffering({}) ", args);
    printer->add_line("int i = 0;");
    print_device_atomic_capture_annotation();
    printer->add_line("i = nsb->_cnt++;");
    print_net_send_buffering_grow();
    printer->start_block("if(i < nsb->_size)");
    printer->add_line("nsb->_sendtype[i] = type;");
    printer->add_line("nsb->_vdata_index[i] = vdata_index;");
    printer->add_line("nsb->_weight_index[i] = weight_index;");
    printer->add_line("nsb->_pnt_index[i] = point_index;");
    printer->add_line("nsb->_nsb_t[i] = t;");
    printer->add_line("nsb->_nsb_flag[i] = flag;");
    printer->end_block(1);
    printer->end_block(1);
}


void CodegenCVisitor::visit_for_netcon(const ast::ForNetcon& node) {
    // For_netcon should take the same arguments as net_receive and apply the operations
    // in the block to the weights of the netcons. Since all the weights are on the same vector,
    // weights, we have a mask of operations that we apply iteratively, advancing the offset
    // to the next netcon.
    const auto& args = node.get_parameters();
    RenameVisitor v;
    auto& statement_block = node.get_statement_block();
    for (size_t i_arg = 0; i_arg < args.size(); ++i_arg) {
        // sanitize node_name since we want to substitute names like (*w) as they are
        auto old_name =
            std::regex_replace(args[i_arg]->get_node_name(), regex_special_chars, R"(\$&)");
        auto new_name = fmt::format("weights[{} + nt->_fornetcon_weight_perm[i]]", i_arg);
        v.set(old_name, new_name);
        statement_block->accept(v);
    }

    const auto index =
        std::find_if(info.semantics.begin(), info.semantics.end(), [](const IndexSemantics& a) {
            return a.name == naming::FOR_NETCON_SEMANTIC;
        })->index;

    printer->fmt_text("const size_t offset = {}*pnodecount + id;", index);
    printer->add_newline();
    printer->add_line(
        "const size_t for_netcon_start = nt->_fornetcon_perm_indices[indexes[offset]];");
    printer->add_line(
        "const size_t for_netcon_end = nt->_fornetcon_perm_indices[indexes[offset] + 1];");

    printer->add_line("for (auto i = for_netcon_start; i < for_netcon_end; ++i) {");
    printer->increase_indent();
    print_statement_block(*statement_block, false, false);
    printer->decrease_indent();

    printer->add_line("}");
}

void CodegenCVisitor::print_net_receive_kernel() {
    if (!net_receive_required()) {
        return;
    }
    codegen = true;
    printing_net_receive = true;
    const auto node = info.net_receive_node;

    // rename net_receive arguments used in the block itself
    rename_net_receive_arguments(*info.net_receive_node, *node);

    std::string name;
    auto params = ParamVector();
    if (!info.artificial_cell) {
        name = method_name("net_receive_kernel");
        params.emplace_back("", "double", "", "t");
        params.emplace_back("", "Point_process*", "", "pnt");
        params.emplace_back(param_type_qualifier(),
                            fmt::format("{}*", instance_struct()),
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
    printer->fmt_start_block("static inline void {}({}) ", name, get_parameter_str(params));
    print_net_receive_common_code(*node, info.artificial_cell);
    if (info.artificial_cell) {
        printer->add_line("double t = nt->_t;");
    }

    // set voltage variable if it is used in the block (e.g. for WATCH statement)
    auto v_used = VarUsageVisitor().variable_used(*node->get_statement_block(), "v");
    if (v_used) {
        printer->add_line("int node_id = ml->nodeindices[id];");
        printer->add_line("v = nt->_actual_v[node_id];");
    }

    printer->fmt_line("{} = t;", get_variable_name("tsave"));

    if (info.is_watch_used()) {
        printer->add_line("bool watch_remove = false;");
    }

    printer->add_indent();
    node->get_statement_block()->accept(*this);
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
        printer->fmt_start_block("static void {}({}) ", name, get_parameter_str(params));
        printer->add_line("NrnThread* nt = nrn_threads + pnt->_tid;");
        printer->add_line("Memb_list* ml = get_memb_list(nt);");
        printer->add_line("NetReceiveBuffer_t* nrb = ml->_net_receive_buffer;");
        printer->start_block("if (nrb->_cnt >= nrb->_size)");
        printer->add_line("realloc_net_receive_buffer(nt, ml);");
        printer->end_block(1);
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
 * \todo Data is not derived. Need to add instance into instance struct?
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
    auto stride = "*pnodecount+id";

    printer->add_newline(2);

    printer->start_block("namespace");
    printer->fmt_start_block("struct _newton_{}_{}", block_name, info.mod_suffix);
    printer->fmt_start_block("int operator()({}) const", external_method_parameters());
    auto const instance = fmt::format("auto* const inst = static_cast<{0}*>(ml->instance);",
                                      instance_struct());
    auto const slist1 = fmt::format("auto const& slist{} = {};",
                                    list_num,
                                    get_variable_name(fmt::format("slist{}", list_num)));
    auto const slist2 = fmt::format("auto& slist{} = {};",
                                    list_num + 1,
                                    get_variable_name(fmt::format("slist{}", list_num + 1)));
    auto const dlist1 = fmt::format("auto const& dlist{} = {};",
                                    list_num,
                                    get_variable_name(fmt::format("dlist{}", list_num)));
    auto const dlist2 = fmt::format(
        "double* dlist{} = static_cast<double*>(thread[dith{}()].pval) + ({}*pnodecount);",
        list_num + 1,
        list_num,
        info.primes_size);
    printer->add_line(instance);
    if (ion_variable_struct_required()) {
        print_ion_variable();
    }
    printer->fmt_line("double* savstate{} = static_cast<double*>(thread[dith{}()].pval);",
                      list_num,
                      list_num);
    printer->add_line(slist1);
    printer->add_line(dlist1);
    printer->add_line(dlist2);
    codegen = true;
    print_statement_block(*block->get_statement_block(), false, false);
    codegen = false;
    printer->add_line("int counter = -1;");
    printer->fmt_start_block("for (int i=0; i<{}; i++)", info.num_primes);
    printer->fmt_start_block("if (*deriv{}_advance(thread))", list_num);
    printer->fmt_line(
        "dlist{0}[(++counter){1}] = "
        "data[dlist{2}[i]{1}]-(data[slist{2}[i]{1}]-savstate{2}[i{1}])/nt->_dt;",
        list_num + 1,
        stride,
        list_num);
    printer->restart_block("else");
    printer->fmt_line("dlist{0}[(++counter){1}] = data[slist{2}[i]{1}]-savstate{2}[i{1}];",
                      list_num + 1,
                      stride,
                      list_num);
    printer->end_block(1);
    printer->end_block(1);
    printer->add_line("return 0;");
    printer->end_block(1);  // operator()
    printer->end_block(";");   // struct
    printer->end_block(2);  // namespace
    printer->fmt_start_block("int {}_{}({})", block_name, suffix, ext_params);
    printer->add_line(instance);
    printer->fmt_line("double* savstate{} = (double*) thread[dith{}()].pval;", list_num, list_num);
    printer->add_line(slist1);
    printer->add_line(slist2);
    printer->add_line(dlist2);
    printer->fmt_start_block("for (int i=0; i<{}; i++)", info.num_primes);
    printer->fmt_line("savstate{}[i{}] = data[slist{}[i]{}];", list_num, stride, list_num, stride);
    printer->end_block(1);
    printer->fmt_line(
        "int reset = nrn_newton_thread(static_cast<NewtonSpace*>(*newtonspace{}(thread)), {}, "
        "slist{}, _newton_{}_{}{{}}, dlist{}, {});",
        list_num,
        primes_size,
        list_num + 1,
        block_name,
        suffix,
        list_num + 1,
        ext_args);
    printer->add_line("return reset;");
    printer->end_block(3);
}


void CodegenCVisitor::print_newtonspace_transfer_to_device() const {
    // nothing to do on cpu
}


void CodegenCVisitor::visit_derivimplicit_callback(const ast::DerivimplicitCallback& node) {
    if (!codegen) {
        return;
    }
    printer->fmt_line("{}_{}({});",
                      node.get_node_to_solve()->get_node_name(),
                      info.mod_suffix,
                      external_method_arguments());
}

void CodegenCVisitor::visit_solution_expression(const SolutionExpression& node) {
    auto block = node.get_node_to_solve().get();
    if (block->is_statement_block()) {
        auto statement_block = dynamic_cast<ast::StatementBlock*>(block);
        print_statement_block(*statement_block, false, false);
    } else {
        block->accept(*this);
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

    printer->add_line("int node_id = node_index[id];");
    printer->add_line("double v = voltage[node_id];");
    print_v_unused();

    /**
     * \todo Eigen solver node also emits IonCurVar variable in the functor
     * but that shouldn't update ions in derivative block
     */
    if (ion_variable_struct_required()) {
        print_ion_variable();
    }

    auto read_statements = ion_read_statements(BlockType::State);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    if (info.nrn_state_block) {
        info.nrn_state_block->visit_children(*this);
    }

    if (info.currents.empty() && info.breakpoint_node != nullptr) {
        auto block = info.breakpoint_node->get_statement_block();
        print_statement_block(*block, false, false);
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


void CodegenCVisitor::print_nrn_current(const BreakpointBlock& node) {
    auto args = internal_method_parameters();
    const auto& block = node.get_statement_block();
    printer->add_newline(2);
    print_device_method_annotation();
    printer->fmt_start_block("inline double nrn_current_{}({})",
                             info.mod_suffix,
                             get_parameter_str(args));
    printer->add_line("double current = 0.0;");
    print_statement_block(*block, false, false);
    for (auto& current: info.currents) {
        auto name = get_variable_name(current);
        printer->fmt_line("current += {};", name);
    }
    printer->add_line("return current;");
    printer->end_block(1);
}


void CodegenCVisitor::print_nrn_cur_conductance_kernel(const BreakpointBlock& node) {
    const auto& block = node.get_statement_block();
    print_statement_block(*block, false, false);
    if (!info.currents.empty()) {
        std::string sum;
        for (const auto& current: info.currents) {
            auto var = breakpoint_current(current);
            sum += get_variable_name(var);
            if (&current != &info.currents.back()) {
                sum += "+";
            }
        }
        printer->fmt_line("double rhs = {};", sum);
    }

    std::string sum;
    for (const auto& conductance: info.conductances) {
        auto var = breakpoint_current(conductance.variable);
        sum += get_variable_name(var);
        if (&conductance != &info.conductances.back()) {
            sum += "+";
        }
    }
    printer->fmt_line("double g = {};", sum);

    for (const auto& conductance: info.conductances) {
        if (!conductance.ion.empty()) {
            auto lhs = std::string(naming::ION_VARNAME_PREFIX) + "di" + conductance.ion + "dv";
            auto rhs = get_variable_name(conductance.variable);
            ShadowUseStatement statement{lhs, "+=", rhs};
            auto text = process_shadow_update_statement(statement, BlockType::Equation);
            printer->add_line(text);
        }
    }
}


void CodegenCVisitor::print_nrn_cur_non_conductance_kernel() {
    printer->fmt_line("double g = nrn_current_{}({}+0.001);",
                                  info.mod_suffix,
                                  internal_method_arguments());
    for (auto& ion: info.ions) {
        for (auto& var: ion.writes) {
            if (ion.is_ionic_current(var)) {
                auto name = get_variable_name(var);
                printer->fmt_line("double di{} = {};", ion.name, name);
            }
        }
    }
    printer->fmt_line("double rhs = nrn_current_{}({});",
                                  info.mod_suffix,
                                  internal_method_arguments());
    printer->add_line("g = (g-rhs)/0.001;");
    for (auto& ion: info.ions) {
        for (auto& var: ion.writes) {
            if (ion.is_ionic_current(var)) {
                auto lhs = std::string(naming::ION_VARNAME_PREFIX) + "di" + ion.name + "dv";
                auto rhs = fmt::format("(di{}-{})/0.001", ion.name, get_variable_name(var));
                if (info.point_process) {
                    auto area = get_variable_name(naming::NODE_AREA_VARIABLE);
                    rhs += fmt::format("*1.e2/{}", area);
                }
                ShadowUseStatement statement{lhs, "+=", rhs};
                auto text = process_shadow_update_statement(statement, BlockType::Equation);
                printer->add_line(text);
            }
        }
    }
}


void CodegenCVisitor::print_nrn_cur_kernel(const BreakpointBlock& node) {
    printer->add_line("int node_id = node_index[id];");
    printer->add_line("double v = voltage[node_id];");
    print_v_unused();
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
        printer->fmt_line("double mfactor = 1.e2/{};", area);
        printer->add_line("g = g*mfactor;");
        printer->add_line("rhs = rhs*mfactor;");
    }

    print_g_unused();
}

void CodegenCVisitor::print_fast_imem_calculation() {
    if (!info.electrode_current) {
        return;
    }
    std::string rhs, d;
    auto rhs_op = operator_for_rhs();
    auto d_op = operator_for_d();
    if (info.point_process) {
        rhs = "shadow_rhs[id]";
        d = "shadow_d[id]";
    } else {
        rhs = "rhs";
        d = "g";
    }

    printer->start_block("if (nt->nrn_fast_imem)");
    if (nrn_cur_reduction_loop_required()) {
        print_shadow_reduction_block_begin();
        printer->add_line("int node_id = node_index[id];");
    }
    print_atomic_reduction_pragma();
    printer->fmt_line("nt->nrn_fast_imem->nrn_sav_rhs[node_id] {} {};", rhs_op, rhs);
    print_atomic_reduction_pragma();
    printer->fmt_line("nt->nrn_fast_imem->nrn_sav_d[node_id] {} {};", d_op, d);
    if (nrn_cur_reduction_loop_required()) {
        print_shadow_reduction_block_end();
    }
    printer->end_block(1);
}

void CodegenCVisitor::print_nrn_cur() {
    if (!nrn_cur_required()) {
        return;
    }

    codegen = true;
    if (info.conductances.empty()) {
        print_nrn_current(*info.breakpoint_node);
    }

    printer->add_newline(2);
    printer->add_line("/** update current */");
    print_global_function_common_code(BlockType::Equation);
    print_channel_iteration_tiling_block_begin(BlockType::Equation);
    print_channel_iteration_block_begin(BlockType::Equation);
    print_nrn_cur_kernel(*info.breakpoint_node);
    print_nrn_cur_matrix_shadow_update();
    if (!nrn_cur_reduction_loop_required()) {
        print_fast_imem_calculation();
    }
    print_channel_iteration_block_end();

    if (nrn_cur_reduction_loop_required()) {
        print_shadow_reduction_block_begin();
        print_nrn_cur_matrix_shadow_reduction();
        print_shadow_reduction_statements();
        print_shadow_reduction_block_end();
        print_fast_imem_calculation();
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
    print_net_receive_arg_size_getter();
    print_thread_getters();
    print_num_variable_getter();
    print_mech_type_getter();
    print_memb_list_getter();
}


void CodegenCVisitor::print_data_structures(bool print_initialisers) {
    print_mechanism_global_var_structure(print_initialisers);
    print_mechanism_range_var_structure(print_initialisers);
    print_ion_var_structure();
}

void CodegenCVisitor::print_v_unused() const {
    if (!info.vectorize) {
        return;
    }
    printer->add_line("#if NRN_PRCELLSTATE");
    printer->add_line("inst->v_unused[id] = v;");
    printer->add_line("#endif");
}

void CodegenCVisitor::print_g_unused() const {
    printer->add_line("#if NRN_PRCELLSTATE");
    printer->add_line("inst->g_unused[id] = g;");
    printer->add_line("#endif");
}

void CodegenCVisitor::print_compute_functions() {
    print_top_verbatim_blocks();
    print_function_prototypes();
    for (const auto& procedure: info.procedures) {
        print_procedure(*procedure);
    }
    for (const auto& function: info.functions) {
        print_function(*function);
    }
    for (size_t i = 0; i < info.before_after_blocks.size(); i++) {
        print_before_after_block(info.before_after_blocks[i], i);
    }
    for (const auto& callback: info.derivimplicit_callbacks) {
        auto block = callback->get_node_to_solve().get();
        print_derivimplicit_kernel(block);
    }
    print_net_send_buffering();
    print_net_init();
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
    print_nmodl_constants();
    print_prcellstate_macros();
    print_mechanism_info();
    print_data_structures(true);
    print_global_variables_for_hoc();
    print_common_getters();
    print_memory_allocation_routine();
    print_abort_routine();
    print_thread_memory_callbacks();
    print_instance_variable_setup();
    print_nrn_alloc();
    print_nrn_constructor();
    print_nrn_destructor();
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


void CodegenCVisitor::setup(const Program& node) {
    program_symtab = node.get_symbol_table();

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


void CodegenCVisitor::visit_program(const Program& node) {
    setup(node);
    print_codegen_routines();
    print_wrapper_routines();
}

}  // namespace codegen
}  // namespace nmodl
