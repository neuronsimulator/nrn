/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "codegen/codegen_cpp_visitor.hpp"

#include "ast/all.hpp"
#include "codegen/codegen_helper_visitor.hpp"
#include "codegen/codegen_utils.hpp"
#include "visitors/rename_visitor.hpp"

namespace nmodl {
namespace codegen {

using namespace ast;

using visitor::RenameVisitor;

using symtab::syminfo::NmodlType;


/****************************************************************************************/
/*                     Common helper routines accross codegen functions                 */
/****************************************************************************************/


template <typename T>
bool CodegenCppVisitor::has_parameter_of_name(const T& node, const std::string& name) {
    auto parameters = node->get_parameters();
    return std::any_of(parameters.begin(),
                       parameters.end(),
                       [&name](const decltype(*parameters.begin()) arg) {
                           return arg->get_node_name() == name;
                       });
}


/**
 * \details Certain statements like unit, comment, solve can/need to be skipped
 * during code generation. Note that solve block is wrapped in expression
 * statement and hence we have to check inner expression. It's also true
 * for the initial block defined inside net receive block.
 */
bool CodegenCppVisitor::statement_to_skip(const Statement& node) {
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


bool CodegenCppVisitor::net_send_buffer_required() const noexcept {
    if (net_receive_required() && !info.artificial_cell) {
        if (info.net_event_used || info.net_send_used || info.is_watch_used()) {
            return true;
        }
    }
    return false;
}


bool CodegenCppVisitor::net_receive_buffering_required() const noexcept {
    return info.point_process && !info.artificial_cell && info.net_receive_node != nullptr;
}


bool CodegenCppVisitor::nrn_state_required() const noexcept {
    if (info.artificial_cell) {
        return false;
    }
    return info.nrn_state_block != nullptr || breakpoint_exist();
}


bool CodegenCppVisitor::nrn_cur_required() const noexcept {
    return info.breakpoint_node != nullptr && !info.currents.empty();
}


bool CodegenCppVisitor::net_receive_exist() const noexcept {
    return info.net_receive_node != nullptr;
}


bool CodegenCppVisitor::breakpoint_exist() const noexcept {
    return info.breakpoint_node != nullptr;
}


bool CodegenCppVisitor::net_receive_required() const noexcept {
    return net_receive_exist();
}


/**
 * \details When floating point data type is not default (i.e. double) then we
 * have to copy old array to new type (for range variables).
 */
bool CodegenCppVisitor::range_variable_setup_required() const noexcept {
    return codegen::naming::DEFAULT_FLOAT_TYPE != float_data_type();
}


// check if there is a function or procedure defined with given name
bool CodegenCppVisitor::defined_method(const std::string& name) const {
    const auto& function = program_symtab->lookup(name);
    auto properties = NmodlType::function_block | NmodlType::procedure_block;
    return function && function->has_any_property(properties);
}


int CodegenCppVisitor::float_variables_size() const {
    return codegen_float_variables.size();
}


int CodegenCppVisitor::int_variables_size() const {
    const auto count_semantics = [](int sum, const IndexSemantics& sem) { return sum += sem.size; };
    return std::accumulate(info.semantics.begin(), info.semantics.end(), 0, count_semantics);
}


/**
 * \details We can directly print value but if user specify value as integer then
 * then it gets printed as an integer. To avoid this, we use below wrapper.
 * If user has provided integer then it gets printed as 1.0 (similar to mod2c
 * and neuron where ".0" is appended). Otherwise we print double variables as
 * they are represented in the mod file by user. If the value is in scientific
 * representation (1e+20, 1E-15) then keep it as it is.
 */
std::string CodegenCppVisitor::format_double_string(const std::string& s_value) {
    return utils::format_double_string<CodegenCppVisitor>(s_value);
}


std::string CodegenCppVisitor::format_float_string(const std::string& s_value) {
    return utils::format_float_string<CodegenCppVisitor>(s_value);
}


/**
 * \details Statements like if, else etc. don't need semicolon at the end.
 * (Note that it's valid to have "extraneous" semicolon). Also, statement
 * block can appear as statement using expression statement which need to
 * be inspected.
 */
bool CodegenCppVisitor::need_semicolon(const Statement& node) {
    // clang-format off
    if (node.is_if_statement()
        || node.is_else_if_statement()
        || node.is_else_statement()
        || node.is_from_statement()
        || node.is_verbatim()
        || node.is_conductance_hint()
        || node.is_while_statement()
        || node.is_protect_statement()
        || node.is_mutex_lock()
        || node.is_mutex_unlock()) {
        return false;
    }
    if (node.is_expression_statement()) {
        auto expression = dynamic_cast<const ExpressionStatement&>(node).get_expression();
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


/****************************************************************************************/
/*                      Main printing routines for code generation                      */
/****************************************************************************************/


void CodegenCppVisitor::print_prcellstate_macros() const {
    printer->add_line("#ifndef NRN_PRCELLSTATE");
    printer->add_line("#define NRN_PRCELLSTATE 0");
    printer->add_line("#endif");
}


void CodegenCppVisitor::print_mechanism_info() {
    auto variable_printer = [&](const std::vector<SymbolType>& variables) {
        for (const auto& v: variables) {
            auto name = v->get_name();
            if (!info.point_process) {
                name += "_" + info.mod_suffix;
            }
            if (v->is_array()) {
                name += fmt::format("[{}]", v->get_length());
            }
            printer->add_line(add_escape_quote(name), ",");
        }
    };

    printer->add_newline(2);
    printer->add_line("/** channel information */");
    printer->fmt_line("static const char *{}[] = {{", get_channel_info_var_name());
    printer->increase_indent();
    printer->add_line(add_escape_quote(nmodl_version()), ",");
    printer->add_line(add_escape_quote(info.mod_suffix), ",");
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


/****************************************************************************************/
/*                         Printing routines for code generation                        */
/****************************************************************************************/


void CodegenCppVisitor::print_statement_block(const ast::StatementBlock& node,
                                              bool open_brace,
                                              bool close_brace) {
    if (open_brace) {
        printer->push_block();
    }

    const auto& statements = node.get_statements();
    for (const auto& statement: statements) {
        if (statement_to_skip(*statement)) {
            continue;
        }
        /// not necessary to add indent for verbatim block (pretty-printing)
        if (!statement->is_verbatim() && !statement->is_mutex_lock() &&
            !statement->is_mutex_unlock() && !statement->is_protect_statement()) {
            printer->add_indent();
        }
        statement->accept(*this);
        if (need_semicolon(*statement)) {
            printer->add_text(';');
        }
        if (!statement->is_mutex_lock() && !statement->is_mutex_unlock()) {
            printer->add_newline();
        }
    }

    if (close_brace) {
        printer->pop_block_nl(0);
    }
}


/**
 * \todo Issue with verbatim renaming. e.g. pattern.mod has info struct with
 * index variable. If we use "index" instead of "indexes" as default argument
 * then during verbatim replacement we don't know the index is which one. This
 * is because verbatim renaming pass has already stripped out prefixes from
 * the text.
 */
void CodegenCppVisitor::rename_function_arguments() {
    const auto& default_arguments = stringutils::split_string(nrn_thread_arguments(), ',');
    for (const auto& dirty_arg: default_arguments) {
        const auto& arg = stringutils::trim(dirty_arg);
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


/****************************************************************************************/
/*                              Main code printing entry points                         */
/****************************************************************************************/


/**
 * NMODL constants from unit database
 *
 */
void CodegenCppVisitor::print_nmodl_constants() {
    if (!info.factor_definitions.empty()) {
        printer->add_newline(2);
        printer->add_line("/** constants used in nmodl from UNITS */");
        for (const auto& it: info.factor_definitions) {
            const std::string format_string = "static const double {} = {};";
            printer->fmt_line(format_string, it->get_node_name(), it->get_value()->get_value());
        }
    }
}


/****************************************************************************************/
/*                            Overloaded visitor routines                               */
/****************************************************************************************/


extern const std::regex regex_special_chars{R"([-[\]{}()*+?.,\^$|#\s])"};


void CodegenCppVisitor::visit_string(const String& node) {
    if (!codegen) {
        return;
    }
    std::string name = node.eval();
    if (enable_variable_name_lookup) {
        name = get_variable_name(name);
    }
    printer->add_text(name);
}


void CodegenCppVisitor::visit_integer(const Integer& node) {
    if (!codegen) {
        return;
    }
    const auto& value = node.get_value();
    printer->add_text(std::to_string(value));
}


void CodegenCppVisitor::visit_float(const Float& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(format_float_string(node.get_value()));
}


void CodegenCppVisitor::visit_double(const Double& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(format_double_string(node.get_value()));
}


void CodegenCppVisitor::visit_boolean(const Boolean& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(std::to_string(static_cast<int>(node.eval())));
}


void CodegenCppVisitor::visit_name(const Name& node) {
    if (!codegen) {
        return;
    }
    node.visit_children(*this);
}


void CodegenCppVisitor::visit_unit(const ast::Unit& node) {
    // do not print units
}


void CodegenCppVisitor::visit_prime_name(const PrimeName& /* node */) {
    throw std::runtime_error("PRIME encountered during code generation, ODEs not solved?");
}


/**
 * \todo : Validate how @ is being handled in neuron implementation
 */
void CodegenCppVisitor::visit_var_name(const VarName& node) {
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


void CodegenCppVisitor::visit_indexed_name(const IndexedName& node) {
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


void CodegenCppVisitor::visit_local_list_statement(const LocalListStatement& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(local_var_type(), ' ');
    print_vector_elements(node.get_variables(), ", ");
}


void CodegenCppVisitor::visit_if_statement(const IfStatement& node) {
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


void CodegenCppVisitor::visit_else_if_statement(const ElseIfStatement& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(" else if (");
    node.get_condition()->accept(*this);
    printer->add_text(") ");
    node.get_statement_block()->accept(*this);
}


void CodegenCppVisitor::visit_else_statement(const ElseStatement& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(" else ");
    node.visit_children(*this);
}


void CodegenCppVisitor::visit_while_statement(const WhileStatement& node) {
    printer->add_text("while (");
    node.get_condition()->accept(*this);
    printer->add_text(") ");
    node.get_statement_block()->accept(*this);
}


void CodegenCppVisitor::visit_from_statement(const ast::FromStatement& node) {
    if (!codegen) {
        return;
    }
    auto name = node.get_node_name();
    const auto& from = node.get_from();
    const auto& to = node.get_to();
    const auto& inc = node.get_increment();
    const auto& block = node.get_statement_block();
    printer->fmt_text("for (int {} = ", name);
    from->accept(*this);
    printer->fmt_text("; {} <= ", name);
    to->accept(*this);
    if (inc) {
        printer->fmt_text("; {} += ", name);
        inc->accept(*this);
    } else {
        printer->fmt_text("; {}++", name);
    }
    printer->add_text(") ");
    block->accept(*this);
}


void CodegenCppVisitor::visit_paren_expression(const ParenExpression& node) {
    if (!codegen) {
        return;
    }
    printer->add_text("(");
    node.get_expression()->accept(*this);
    printer->add_text(")");
}


void CodegenCppVisitor::visit_binary_expression(const BinaryExpression& node) {
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


void CodegenCppVisitor::visit_binary_operator(const BinaryOperator& node) {
    if (!codegen) {
        return;
    }
    printer->add_text(node.eval());
}


void CodegenCppVisitor::visit_unary_operator(const UnaryOperator& node) {
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
void CodegenCppVisitor::visit_statement_block(const StatementBlock& node) {
    if (!codegen) {
        node.visit_children(*this);
        return;
    }
    print_statement_block(node);
}


void CodegenCppVisitor::visit_function_call(const FunctionCall& node) {
    if (!codegen) {
        return;
    }
    print_function_call(node);
}


void CodegenCppVisitor::visit_verbatim(const Verbatim& node) {
    if (!codegen) {
        return;
    }
    const auto& text = node.get_statement()->eval();
    const auto& result = process_verbatim_text(text);

    const auto& statements = stringutils::split_string(result, '\n');
    for (const auto& statement: statements) {
        const auto& trimed_stmt = stringutils::trim_newline(statement);
        if (trimed_stmt.find_first_not_of(' ') != std::string::npos) {
            printer->add_line(trimed_stmt);
        }
    }
}


void CodegenCppVisitor::visit_update_dt(const ast::UpdateDt& node) {
    // dt change statement should be pulled outside already
}


void CodegenCppVisitor::visit_protect_statement(const ast::ProtectStatement& node) {
    print_atomic_reduction_pragma();
    printer->add_indent();
    node.get_expression()->accept(*this);
    printer->add_text(";");
}


void CodegenCppVisitor::visit_mutex_lock(const ast::MutexLock& node) {
    printer->fmt_line("#pragma omp critical ({})", info.mod_suffix);
    printer->add_indent();
    printer->push_block();
}


void CodegenCppVisitor::visit_mutex_unlock(const ast::MutexUnlock& node) {
    printer->pop_block();
}


/**
 * \details Once variables are populated, update index semantics to register with coreneuron
 */
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void CodegenCppVisitor::update_index_semantics() {
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
            info.semantics.emplace_back(index++, fmt::format("{}_ion", ion.name), 1);
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


std::vector<CodegenCppVisitor::SymbolType> CodegenCppVisitor::get_float_variables() const {
    // sort with definition order
    auto comparator = [](const SymbolType& first, const SymbolType& second) -> bool {
        return first->get_definition_order() < second->get_definition_order();
    };

    auto assigned = info.assigned_vars;
    auto states = info.state_vars;

    // each state variable has corresponding Dstate variable
    for (const auto& state: states) {
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
std::vector<IndexVariableInfo> CodegenCppVisitor::get_int_variables() {
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

    for (auto& ion: info.ions) {
        bool need_style = false;
        std::unordered_map<std::string, int> ion_vars;  // used to keep track of the variables to
                                                        // not have doubles between read/write. Same
                                                        // name variables are allowed
        // See if we need to add extra readion statements to match NEURON with SoA data
        auto const has_var = [&ion](const char* suffix) -> bool {
            auto const pred = [name = ion.name + suffix](auto const& x) { return x == name; };
            return std::any_of(ion.reads.begin(), ion.reads.end(), pred) ||
                   std::any_of(ion.writes.begin(), ion.writes.end(), pred);
        };
        auto const add_implicit_read = [&ion](const char* suffix) {
            auto name = ion.name + suffix;
            ion.reads.push_back(name);
            ion.implicit_reads.push_back(std::move(name));
        };
        bool const have_ionin{has_var("i")}, have_ionout{has_var("o")};
        if (have_ionin && !have_ionout) {
            add_implicit_read("o");
        } else if (have_ionout && !have_ionin) {
            add_implicit_read("i");
        }
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
            variables.emplace_back(make_symbol(naming::ION_VARNAME_PREFIX + ion.name + "_erev"));
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


void CodegenCppVisitor::setup(const Program& node) {
    program_symtab = node.get_symbol_table();

    CodegenHelperVisitor v;
    info = v.analyze(node);
    info.mod_file = mod_filename;

    if (!info.vectorize) {
        logger->warn(
            "CodegenCoreneuronCppVisitor : MOD file uses non-thread safe constructs of NMODL");
    }

    codegen_float_variables = get_float_variables();
    codegen_int_variables = get_int_variables();

    update_index_semantics();
    rename_function_arguments();
}


void CodegenCppVisitor::visit_program(const Program& node) {
    setup(node);
    print_codegen_routines();
}

}  // namespace codegen
}  // namespace nmodl