#include <algorithm>
#include <utility>
#include <cmath>
#include <ctime>

#include "visitors/rename_visitor.hpp"
#include "codegen/base/codegen_helper_visitor.hpp"
#include "codegen/c/codegen_c_visitor.hpp"
#include "visitors/var_usage_visitor.hpp"
#include "utils/string_utils.hpp"
#include "version/version.h"
#include "parser/c11_driver.hpp"


using namespace symtab;
using namespace fmt::literals;
using SymbolType = std::shared_ptr<symtab::Symbol>;


/****************************************************************************************/
/*                                All visitor routines                                  */
/****************************************************************************************/


void CodegenBaseVisitor::visit_string(String* node) {
    if (!codegen) {
        return;
    }
    std::string name = node->eval();
    if (enable_variable_name_lookup) {
        name = get_variable_name(name);
    }
    printer->add_text(name);
}


void CodegenBaseVisitor::visit_integer(Integer* node) {
    if (!codegen) {
        return;
    }
    auto macro = node->get_macro_name();
    auto value = node->get_value();
    if (macro) {
        macro->accept(this);
    } else {
        printer->add_text(std::to_string(value));
    }
}


void CodegenBaseVisitor::visit_float(Float* node) {
    if (!codegen) {
        return;
    }
    printer->add_text(std::to_string(node->eval()));
}


void CodegenBaseVisitor::visit_double(Double* node) {
    if (!codegen) {
        return;
    }
    auto value = node->eval();
    printer->add_text(double_to_string(value));
}


void CodegenBaseVisitor::visit_boolean(Boolean* node) {
    if (!codegen) {
        return;
    }
    printer->add_text(std::to_string(static_cast<int>(node->eval())));
}


void CodegenBaseVisitor::visit_name(Name* node) {
    if (!codegen) {
        return;
    }
    node->visit_children(this);
}


void CodegenBaseVisitor::visit_unit(ast::Unit* node) {
    // do not print units
}


void CodegenBaseVisitor::visit_prime_name(PrimeName* node) {
    throw std::runtime_error("Prime encountered, ODEs not solved?");
}


/// \todo : validate how @ is being handled in neuron implementation
void CodegenBaseVisitor::visit_var_name(VarName* node) {
    if (!codegen) {
        return;
    }
    node->name->accept(this);
    if (node->at_index) {
        printer->add_text("@");
        node->at_index->accept(this);
    }
    if (node->index) {
        printer->add_text("[");
        node->index->accept(this);
        printer->add_text("]");
    }
}


void CodegenBaseVisitor::visit_indexed_name(IndexedName* node) {
    if (!codegen) {
        return;
    }
    node->name->accept(this);
    printer->add_text("[");
    node->length->accept(this);
    printer->add_text("]");
}


void CodegenBaseVisitor::visit_local_list_statement(LocalListStatement* node) {
    if (!codegen) {
        return;
    }
    auto type = local_var_type() + " ";
    printer->add_text(type);
    print_vector_elements(node->variables, ", ");
}


void CodegenBaseVisitor::visit_if_statement(IfStatement* node) {
    if (!codegen) {
        return;
    }
    printer->add_text("if (");
    node->condition->accept(this);
    printer->add_text(") ");
    node->statementblock->accept(this);
    print_vector_elements(node->elseifs, "");
    if (node->elses) {
        node->elses->accept(this);
    }
}


void CodegenBaseVisitor::visit_else_if_statement(ElseIfStatement* node) {
    if (!codegen) {
        return;
    }
    printer->add_text(" else if (");
    node->condition->accept(this);
    printer->add_text(") ");
    node->statementblock->accept(this);
}


void CodegenBaseVisitor::visit_else_statement(ElseStatement* node) {
    if (!codegen) {
        return;
    }
    printer->add_text(" else ");
    node->visit_children(this);
}


void CodegenBaseVisitor::visit_from_statement(ast::FromStatement* node) {
    if (!codegen) {
        return;
    }
    auto name = node->get_from_name()->get_name();
    auto from = node->get_from_expression();
    auto to = node->get_to_expression();
    auto inc = node->get_inc_expression();
    auto block = node->get_block();
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


void CodegenBaseVisitor::visit_paren_expression(ParenExpression* node) {
    if (!codegen) {
        return;
    }
    printer->add_text("(");
    node->expr->accept(this);
    printer->add_text(")");
}


void CodegenBaseVisitor::visit_binary_expression(BinaryExpression* node) {
    if (!codegen) {
        return;
    }
    auto op = node->op.eval();
    auto lhs = node->get_lhs();
    auto rhs = node->get_rhs();
    if (op == "^") {
        printer->add_text("pow(");
        lhs->accept(this);
        printer->add_text(",");
        rhs->accept(this);
        printer->add_text(")");
    } else {
        if (op == "=" || op == "&&" || op == "||" || op == "==") {
            op = " " + op + " ";
        }
        lhs->accept(this);
        printer->add_text(op);
        rhs->accept(this);
    }
}


void CodegenBaseVisitor::visit_binary_operator(BinaryOperator* node) {
    if (!codegen) {
        return;
    }
    printer->add_text(node->eval());
}


void CodegenBaseVisitor::visit_unary_operator(UnaryOperator* node) {
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
void CodegenBaseVisitor::visit_statement_block(StatementBlock* node) {
    if (!codegen) {
        node->visit_children(this);
        return;
    }
    print_statement_block(node);
}


void CodegenBaseVisitor::visit_program(Program* node) {
    program_symtab = node->get_symbol_table();

    CodegenHelperVisitor v;
    info = v.get_code_info(node);
    info.mod_file = mod_file_suffix;

    float_variables = get_float_variables();
    int_variables = get_int_variables();
    shadow_variables = get_shadow_variables();

    update_index_semantics();
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
bool CodegenBaseVisitor::skip_statement(Statement* node) {
    // clang-format off
    if (node->is_unit_state()
        || node->is_comment()
        || node->is_solve_block()
        || node->is_conductance_hint()) {
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


bool CodegenBaseVisitor::net_send_buffer_required() {
    if (net_receive_required() && !info.artificial_cell) {
        if (info.net_event_used || info.net_send_used) {
            return true;
        }
    }
    return false;
}


bool CodegenBaseVisitor::net_receive_buffering_required() {
    return info.point_process && !info.artificial_cell && info.net_receive_node != nullptr;
}


bool CodegenBaseVisitor::nrn_state_required() {
    if (info.artificial_cell) {
        return false;
    }
    return info.solve_node != nullptr || info.currents.empty();
}


bool CodegenBaseVisitor::nrn_cur_required() {
    return info.breakpoint_node != nullptr && !info.currents.empty();
}


bool CodegenBaseVisitor::net_receive_exist() {
    return info.net_receive_node != nullptr;
}


bool CodegenBaseVisitor::breakpoint_exist() {
    return info.breakpoint_node != nullptr;
}


bool CodegenBaseVisitor::net_receive_required() {
    return net_receive_exist();
}


bool CodegenBaseVisitor::state_variable(std::string name) {
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


int CodegenBaseVisitor::position_of_float_var(const std::string& name) {
    int index = 0;
    for (auto& var : float_variables) {
        if (var->get_name() == name) {
            return index;
        }
        index += var->get_length();
    }
    throw std::logic_error(name + " variable not found");
}


int CodegenBaseVisitor::position_of_int_var(const std::string& name) {
    int index = 0;
    for (auto& var : int_variables) {
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
std::string CodegenBaseVisitor::double_to_string(double value) {
    if (ceilf(value) == value) {
        return "{:.1f}"_format(value);
    }
    return "{:.16g}"_format(value);
}


/**
 * Check if given statement needs semicolon at the end of statement
 *
 * Statements like if, else etc. don't need semicolon at the end.
 * (Note that it's valid to have "extraneous" semicolon). Also, statement
 * block can appear as statement using expression statement which need to
 * be inspected.
 */
bool CodegenBaseVisitor::need_semicolon(Statement* node) {
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
    // clang-format on
    if (node->is_expression_statement()) {
        auto statement = dynamic_cast<ExpressionStatement*>(node);
        if (statement->get_expression()->is_statement_block()) {
            return false;
        }
    }
    return true;
}


// check if there is a function or procedure defined with given name
bool CodegenBaseVisitor::defined_method(std::string name) {
    auto function = program_symtab->lookup(std::move(name));
    auto properties = NmodlInfo::function_block | NmodlInfo::procedure_block;
    return function && function->has_properties(properties);
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
std::string CodegenBaseVisitor::breakpoint_current(std::string current) {
    auto breakpoint = info.breakpoint_node;
    if (breakpoint == nullptr) {
        return current;
    }
    auto symtab = breakpoint->get_statement_block()->get_symbol_table();
    auto variables = symtab->get_variables_with_properties(NmodlInfo::local_var);
    for (auto& var : variables) {
        auto renamed_name = var->get_name();
        auto original_name = var->get_original_name();
        if (current == original_name) {
            current = renamed_name;
            break;
        }
    }
    return current;
}


int CodegenBaseVisitor::num_float_variable() {
    auto count_length = [](std::vector<SymbolType>& variables) -> int {
        int length = 0;
        for (const auto& variable : variables) {
            length += variable->get_length();
        }
        return length;
    };

    int num_variables = count_length(info.range_parameter_vars);
    num_variables += count_length(info.range_dependent_vars);
    num_variables += count_length(info.state_vars);
    num_variables += count_length(info.dependent_vars);

    /// for state variables we add Dstate variables
    num_variables += info.state_vars.size();
    num_variables += info.ion_state_vars.size();

    /// for v_unused variable
    if (info.vectorize) {
        num_variables++;
    }
    /// for g_unused variable
    if (breakpoint_exist()) {
        num_variables++;
    }
    /// for tsave variable
    if (net_receive_exist()) {
        num_variables++;
    }
    return num_variables;
}


int CodegenBaseVisitor::num_int_variable() {
    int num_variables = 0;
    for (auto& semantic : info.semantics) {
        num_variables += semantic.size;
    }
    return num_variables;
}


/****************************************************************************************/
/*               Non-code-specific printing routines for code generation                */
/****************************************************************************************/


void CodegenBaseVisitor::print_statement_block(ast::StatementBlock* node,
                                               bool open_brace,
                                               bool close_brace) {
    if (open_brace) {
        printer->start_block();
    }

    auto statements = node->get_statements();
    for (auto& statement : statements) {
        if (skip_statement(statement.get())) {
            continue;
        }
        /// not necessary to add indent for vetbatim block (pretty-printing)
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



/**
 * Once variables are populated, update index semantics to register with coreneuron
 */
void CodegenBaseVisitor::update_index_semantics() {
    int index = 0;
    info.semantics.clear();
    if (info.point_process) {
        info.semantics.emplace_back(index++, "area", 1);
        info.semantics.emplace_back(index++, "pntproc", 1);
    }
    for (const auto& ion : info.ions) {
        for (auto& var : ion.reads) {
            info.semantics.emplace_back(index++, ion.name + "_ion", 1);
        }
        for (auto& var : ion.writes) {
            info.semantics.emplace_back(index++, ion.name + "_ion", 1);
            if (ion.is_ionic_current(var)) {
                info.semantics.emplace_back(index++, ion.name + "_ion", 1);
            }
        }
        if (ion.need_style) {
            info.semantics.emplace_back(index++, "#{}_ion"_format(ion.name), 1);
        }
    }
    for (auto& var : info.pointer_variables) {
        if (info.first_pointer_var_index == -1) {
            info.first_pointer_var_index = index;
        }
        int size = var->get_length();
        if (var->has_properties(NmodlInfo::pointer_var)) {
            info.semantics.emplace_back(index, "pointer", size);
        } else {
            info.semantics.emplace_back(index, "bbcorepointer", size);
        }
        index += size;
    }
    if (info.net_send_used) {
        info.semantics.emplace_back(index++, "netsend", 1);
    }
}


/**
 * Return all floating point variables required for code generation
 */
std::vector<SymbolType> CodegenBaseVisitor::get_float_variables() {
    /// sort with definition order
    auto comparator = [](const SymbolType& first, const SymbolType& second) -> bool {
        return first->get_definition_order() < second->get_definition_order();
    };

    auto dependents = info.dependent_vars;
    auto states = info.state_vars;
    states.insert(states.end(), info.ion_state_vars.begin(), info.ion_state_vars.end());

    /// each state variable has corresponding Dstate variable
    for (auto& variable : states) {
        auto name = "D" + variable->get_name();
        auto symbol = make_symbol(name);
        symbol->set_definition_order(variable->get_definition_order());
        dependents.push_back(symbol);
    }
    std::sort(dependents.begin(), dependents.end(), comparator);

    auto variables = info.range_parameter_vars;
    variables.insert(variables.end(), info.range_dependent_vars.begin(),
                     info.range_dependent_vars.end());
    variables.insert(variables.end(), info.state_vars.begin(), info.state_vars.end());
    variables.insert(variables.end(), dependents.begin(), dependents.end());

    if (info.vectorize) {
        variables.push_back(make_symbol("v_unused"));
    }
    if (breakpoint_exist()) {
        std::string name = info.vectorize ? "g_unused" : "_g";
        variables.push_back(make_symbol(name));
    }
    if (net_receive_exist()) {
        variables.push_back(make_symbol("tsave"));
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
 */
std::vector<IndexVariableInfo> CodegenBaseVisitor::get_int_variables() {
    std::vector<IndexVariableInfo> variables;
    if (info.point_process) {
        variables.emplace_back(make_symbol(node_area));
        if (info.artificial_cell) {
            variables.emplace_back(make_symbol("point_process"), true);
        } else {
            variables.emplace_back(make_symbol("point_process"), false, false, true);
        }
    }

    for (auto& ion : info.ions) {
        bool need_style = false;
        for (auto& var : ion.reads) {
            variables.emplace_back(make_symbol("ion_" + var));
        }
        for (auto& var : ion.writes) {
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
        }
    }

    for (auto& var : info.pointer_variables) {
        auto name = var->get_name();
        if (var->has_properties(NmodlInfo::pointer_var)) {
            variables.emplace_back(make_symbol(name));
        } else {
            variables.emplace_back(make_symbol(name), true);
        }
    }

    // for non-artificial cell, when net_receive buffering is enabled
    // then tqitem is an offset
    if (info.net_send_used) {
        if (info.artificial_cell) {
            variables.emplace_back(make_symbol("tqitem"), true);
        } else {
            variables.emplace_back(make_symbol("tqitem"), false, false, true);
        }
        info.tqitem_index = variables.size() - 1;
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
std::vector<SymbolType> CodegenBaseVisitor::get_shadow_variables() {
    std::vector<SymbolType> variables;
    for (auto& ion : info.ions) {
        for (auto& var : ion.writes) {
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
