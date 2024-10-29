/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "codegen/codegen_cpp_visitor.hpp"

#include <filesystem>

#include "config/config.h"

#include "ast/all.hpp"
#include "codegen/codegen_helper_visitor.hpp"
#include "codegen/codegen_utils.hpp"
#include "visitors/defuse_analyze_visitor.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace codegen {

using namespace ast;

using visitor::DefUseAnalyzeVisitor;
using visitor::DUState;
using visitor::RenameVisitor;
using visitor::SymtabVisitor;

using symtab::syminfo::NmodlType;


/****************************************************************************************/
/*                     Common helper routines accross codegen functions                 */
/****************************************************************************************/

bool CodegenCppVisitor::ion_variable_struct_required() const {
    return optimize_ion_variable_copies() && info.ion_has_write_variable();
}

std::string CodegenCppVisitor::get_arg_str(const ParamVector& params) {
    std::vector<std::string> variables;
    for (const auto& param: params) {
        variables.push_back(std::get<3>(param));
    }
    return fmt::format("{}", fmt::join(variables, ", "));
}


std::string CodegenCppVisitor::get_parameter_str(const ParamVector& params) {
    std::vector<std::string> variables;
    for (const auto& param: params) {
        variables.push_back(fmt::format("{}{} {}{}",
                                        std::get<0>(param),
                                        std::get<1>(param),
                                        std::get<2>(param),
                                        std::get<3>(param)));
    }
    return fmt::format("{}", fmt::join(variables, ", "));
}


template <typename T>
bool CodegenCppVisitor::has_parameter_of_name(const T& node, const std::string& name) {
    auto parameters = node->get_parameters();
    return std::any_of(parameters.begin(),
                       parameters.end(),
                       [&name](const decltype(*parameters.begin()) arg) {
                           return arg->get_node_name() == name;
                       });
}


std::string CodegenCppVisitor::table_update_function_name(const std::string& block_name) const {
    return "update_table_" + method_name(block_name);
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

bool CodegenCppVisitor::is_function_table_call(const std::string& name) const {
    auto it = std::find_if(info.function_tables.begin(),
                           info.function_tables.end(),
                           [name](const auto& node) { return node->get_node_name() == name; });
    return it != info.function_tables.end();
}

int CodegenCppVisitor::float_variables_size() const {
    int n_floats = 0;
    for (const auto& var: codegen_float_variables) {
        n_floats += var->get_length();
    }

    return n_floats;
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
    return utils::format_double_string(s_value);
}


std::string CodegenCppVisitor::format_float_string(const std::string& s_value) {
    return utils::format_float_string(s_value);
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

/**
 * \details Depending upon the block type, we have to print read/write ion variables
 * during code generation. Depending on block/procedure being printed, this
 * method return statements as vector. As different code backends could have
 * different variable names, we rely on backend-specific read_ion_variable_name
 * and write_ion_variable_name method which will be overloaded.
 */
std::vector<std::string> CodegenCppVisitor::ion_read_statements(BlockType type) const {
    if (optimize_ion_variable_copies()) {
        return ion_read_statements_optimized(type);
    }
    std::vector<std::string> statements;
    for (const auto& ion: info.ions) {
        auto name = ion.name;
        for (const auto& var: ion.reads) {
            auto const iter = std::find(ion.implicit_reads.begin(), ion.implicit_reads.end(), var);
            if (iter != ion.implicit_reads.end()) {
                continue;
            }
            auto variable_names = read_ion_variable_name(var);
            auto first = get_variable_name(variable_names.first);
            auto second = get_variable_name(variable_names.second);
            statements.push_back(fmt::format("{} = {};", first, second));
        }
        for (const auto& var: ion.writes) {
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


std::vector<std::string> CodegenCppVisitor::ion_read_statements_optimized(BlockType type) const {
    std::vector<std::string> statements;
    for (const auto& ion: info.ions) {
        for (const auto& var: ion.writes) {
            if (ion.is_ionic_conc(var)) {
                auto variables = read_ion_variable_name(var);
                auto first = "ionvar." + variables.first;
                const auto& second = get_variable_name(variables.second);
                statements.push_back(fmt::format("{} = {};", first, second));
            }
        }
    }
    return statements;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
std::vector<ShadowUseStatement> CodegenCppVisitor::ion_write_statements(BlockType type) {
    std::vector<ShadowUseStatement> statements;
    for (const auto& ion: info.ions) {
        std::string concentration;
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
            append_conc_write_statements(statements, ion, concentration);
        }
    }
    return statements;
}

/**
 * If mechanisms dependency level execution is enabled then certain updates
 * like ionic current contributions needs to be atomically updated. In this
 * case we first update current mechanism's shadow vector and then add statement
 * to queue that will be used in reduction queue.
 */
std::string CodegenCppVisitor::process_shadow_update_statement(const ShadowUseStatement& statement,
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


/**
 * \details Current variable used in breakpoint block could be local variable.
 * In this case, neuron has already renamed the variable name by prepending
 * "_l". In our implementation, the variable could have been renamed by
 * one of the pass. And hence, we search all local variables and check if
 * the variable is renamed. Note that we have to look into the symbol table
 * of statement block and not breakpoint.
 */
std::string CodegenCppVisitor::breakpoint_current(std::string current) const {
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

/****************************************************************************************/
/*                         Routines for returning variable name                         */
/****************************************************************************************/

std::string CodegenCppVisitor::update_if_ion_variable_name(const std::string& name) const {
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


std::pair<std::string, std::string> CodegenCppVisitor::read_ion_variable_name(
    const std::string& name) {
    return {name, naming::ION_VARNAME_PREFIX + name};
}


std::pair<std::string, std::string> CodegenCppVisitor::write_ion_variable_name(
    const std::string& name) {
    return {naming::ION_VARNAME_PREFIX + name, name};
}


int CodegenCppVisitor::get_int_variable_index(const std::string& var_name) {
    return get_index_from_name(codegen_int_variables, var_name);
}


/****************************************************************************************/
/*                      Main printing routines for code generation                      */
/****************************************************************************************/


void CodegenCppVisitor::print_backend_info() {
    time_t current_time{};
    time(&current_time);
    std::string data_time_str{std::ctime(&current_time)};
    auto version = nmodl::Version::NMODL_VERSION + " [" + nmodl::Version::GIT_REVISION + "]";

    printer->add_line("/*********************************************************");
    printer->add_line("Model Name      : ", info.mod_suffix);
    printer->add_line("Filename        : ", info.mod_file, ".mod");
    printer->add_line("NMODL Version   : ", nmodl_version());
    printer->fmt_line("Vectorized      : {}", info.vectorize);
    printer->fmt_line("Threadsafe      : {}", info.thread_safe);
    printer->add_line("Created         : ", stringutils::trim(data_time_str));
    printer->add_line("Simulator       : ", simulator_name());
    printer->add_line("Backend         : ", backend_name());
    printer->add_line("NMODL Compiler  : ", version);
    printer->add_line("*********************************************************/");
}


void CodegenCppVisitor::print_global_struct_function_table_ptrs() {
    for (const auto& f: info.function_tables) {
        printer->fmt_line("void* _ptable_{}{{}};", f->get_node_name());
        codegen_global_variables.push_back(make_symbol("_ptable_" + f->get_node_name()));
    }
}


void CodegenCppVisitor::print_global_var_struct_assertions() const {
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


void CodegenCppVisitor::print_global_var_struct_decl() {
    printer->fmt_line("static {} {};", global_struct(), global_struct_instance());
}


void CodegenCppVisitor::print_function_call(const FunctionCall& node) {
    const auto& name = node.get_node_name();

    // return C++ function name for RANDOM construct function
    // e.g. nrnran123_negexp for random_negexp
    auto get_renamed_random_function =
        [&](const std::string& name) -> std::pair<std::string, bool> {
        if (codegen::naming::RANDOM_FUNCTIONS_MAPPING.count(name)) {
            return {codegen::naming::RANDOM_FUNCTIONS_MAPPING[name], true};
        }
        return {name, false};
    };
    auto [function_name, is_random_function] = get_renamed_random_function(name);

    if (defined_method(name)) {
        function_name = method_name(name);
    }

    if (is_nrn_pointing(name)) {
        print_nrn_pointing(node);
        return;
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

    if (is_function_table_call(name)) {
        print_function_table_call(node);
        return;
    }

    const auto& arguments = node.get_arguments();
    printer->add_text(function_name, '(');

    if (defined_method(name)) {
        auto internal_args = internal_method_arguments();
        printer->add_text(internal_args);
        if (!arguments.empty() && !internal_args.empty()) {
            printer->add_text(", ");
        }
    }

    // first argument to random functions need to be type casted
    // from void* to nrnran123_State*.
    if (is_random_function && !arguments.empty()) {
        printer->add_text("(nrnran123_State*)");
    }

    print_vector_elements(arguments, ", ");
    printer->add_text(')');
}

void CodegenCppVisitor::print_nrn_pointing(const ast::FunctionCall& node) {
    printer->add_text("nrn_pointing(&");
    print_vector_elements(node.get_arguments(), ", ");
    printer->add_text(")");
}

void CodegenCppVisitor::print_procedure(const ast::ProcedureBlock& node) {
    print_function_procedure_helper(node);
}


void CodegenCppVisitor::print_function(const ast::FunctionBlock& node) {
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


void CodegenCppVisitor::print_function_tables(const ast::FunctionTableBlock& node) {
    auto name = node.get_node_name();
    const auto& p = node.get_parameters();
    auto [params, table_params] = function_table_parameters(node);
    printer->fmt_push_block("double {}({})", method_name(name), get_parameter_str(params));
    printer->fmt_line("double _arg[{}];", p.size());
    for (size_t i = 0; i < p.size(); ++i) {
        printer->fmt_line("_arg[{}] = {};", i, p[i]->get_node_name());
    }
    printer->fmt_line("return hoc_func_table({}, {}, _arg);",
                      get_variable_name(std::string("_ptable_" + name), true),
                      p.size());
    printer->pop_block();

    printer->fmt_push_block("double table_{}({})",
                            method_name(name),
                            get_parameter_str(table_params));
    printer->fmt_line("hoc_spec_table(&{}, {});",
                      get_variable_name(std::string("_ptable_" + name)),
                      p.size());
    printer->add_line("return 0.;");
    printer->pop_block();
}


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

void CodegenCppVisitor::print_using_namespace() {
    printer->fmt_line("using namespace {};", namespace_name());
}

void CodegenCppVisitor::print_namespace_start() {
    printer->add_newline(2);
    printer->fmt_push_block("namespace {}", namespace_name());
}


void CodegenCppVisitor::print_namespace_stop() {
    printer->pop_block();
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


bool CodegenCppVisitor::is_functor_const(const ast::StatementBlock& variable_block,
                                         const ast::StatementBlock& functor_block) {
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


void CodegenCppVisitor::print_functors_definitions() {
    for (const auto& functor_name: info.functor_names) {
        printer->add_newline(2);
        print_functor_definition(*functor_name.first);
    }
}

void CodegenCppVisitor::print_functor_definition(const ast::EigenNewtonSolverBlock& node) {
    // functor that evaluates F(X) and J(X) for
    // Newton solver
    auto float_type = default_float_data_type();
    int N = node.get_n_state_vars()->get_value();

    const auto functor_name = info.functor_names[&node];
    printer->fmt_push_block("struct {}", functor_name);

    auto params = functor_params();
    for (const auto& param: params) {
        printer->fmt_line("{}{} {};", std::get<0>(param), std::get<1>(param), std::get<3>(param));
    }

    if (ion_variable_struct_required()) {
        print_ion_variable();
    }

    print_statement_block(*node.get_variable_block(), false, false);
    printer->add_newline();

    printer->push_block("void initialize()");
    print_statement_block(*node.get_initialize_block(), false, false);
    printer->pop_block();
    printer->add_newline();

    printer->fmt_line("{}({})", functor_name, get_parameter_str(params));
    printer->increase_indent();
    auto initializers = std::vector<std::string>();
    for (const auto& param: params) {
        initializers.push_back(fmt::format("{0}({0})", std::get<3>(param)));
    }

    printer->add_multi_line(": " + fmt::format("{}", fmt::join(initializers, ", ")));
    printer->decrease_indent();
    printer->add_line("{}");

    printer->add_indent();

    const auto& variable_block = *node.get_variable_block();
    const auto& functor_block = *node.get_functor_block();

    printer->fmt_text(
        "void operator()(const Eigen::Matrix<{0}, {1}, 1>& nmodl_eigen_xm, Eigen::Matrix<{0}, {1}, "
        "1>& nmodl_eigen_dxm, Eigen::Matrix<{0}, {1}, "
        "1>& nmodl_eigen_fm, "
        "Eigen::Matrix<{0}, {1}, {1}>& nmodl_eigen_jm) {2}",
        float_type,
        N,
        is_functor_const(variable_block, functor_block) ? "const " : "");
    printer->push_block();
    printer->fmt_line("const {}* nmodl_eigen_x = nmodl_eigen_xm.data();", float_type);
    printer->fmt_line("{}* nmodl_eigen_dx = nmodl_eigen_dxm.data();", float_type);
    printer->fmt_line("{}* nmodl_eigen_j = nmodl_eigen_jm.data();", float_type);
    printer->fmt_line("{}* nmodl_eigen_f = nmodl_eigen_fm.data();", float_type);

    for (size_t i = 0; i < N; ++i) {
        printer->fmt_line(
            "nmodl_eigen_dx[{0}] = std::max(1e-6, 0.02*std::fabs(nmodl_eigen_x[{0}]));", i);
    }

    print_statement_block(functor_block, false, false);
    printer->pop_block();
    printer->add_newline();

    // assign newton solver results in matrix X to state vars
    printer->push_block("void finalize()");
    print_statement_block(*node.get_finalize_block(), false, false);
    printer->pop_block();

    printer->pop_block(";");
}


void CodegenCppVisitor::print_eigen_linear_solver(const std::string& float_type, int N) {
    if (N <= 4) {
        // Faster compared to LU, given the template specialization in Eigen.
        printer->add_multi_line(R"CODE(
            bool invertible;
            nmodl_eigen_jm.computeInverseWithCheck(nmodl_eigen_jm_inv,invertible);
            nmodl_eigen_xm = nmodl_eigen_jm_inv*nmodl_eigen_fm;
            if (!invertible) assert(false && "Singular or ill-conditioned matrix (Eigen::inverse)!");
        )CODE");
    } else {
        // In Eigen the default storage order is ColMajor.
        // Crout's implementation requires matrices stored in RowMajor order (C++-style arrays).
        // Therefore, the transposeInPlace is critical such that the data() method to give the rows
        // instead of the columns.
        printer->add_line("if (!nmodl_eigen_jm.IsRowMajor) nmodl_eigen_jm.transposeInPlace();");

        // pivot vector
        printer->fmt_line("Eigen::Matrix<int, {}, 1> pivot;", N);
        printer->fmt_line("Eigen::Matrix<{0}, {1}, 1> rowmax;", float_type, N);

        // In-place LU-Decomposition (Crout Algo) : Jm is replaced by its LU-decomposition
        printer->fmt_line(
            "if (nmodl::crout::Crout<{0}>({1}, nmodl_eigen_jm.data(), pivot.data(), rowmax.data()) "
            "< 0) assert(false && \"Singular or ill-conditioned matrix (nmodl::crout)!\");",
            float_type,
            N);

        // Solve the linear system : Forward/Backward substitution part
        printer->fmt_line(
            "nmodl::crout::solveCrout<{0}>({1}, nmodl_eigen_jm.data(), nmodl_eigen_fm.data(), "
            "nmodl_eigen_xm.data(), pivot.data());",
            float_type,
            N);
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
    std::string name = node.eval();
    if (enable_variable_name_lookup) {
        name = get_variable_name(name);
    }
    printer->add_text(name);
}


void CodegenCppVisitor::visit_integer(const Integer& node) {
    const auto& value = node.get_value();
    printer->add_text(std::to_string(value));
}


void CodegenCppVisitor::visit_float(const Float& node) {
    printer->add_text(format_float_string(node.get_value()));
}


void CodegenCppVisitor::visit_double(const Double& node) {
    printer->add_text(format_double_string(node.get_value()));
}


void CodegenCppVisitor::visit_boolean(const Boolean& node) {
    printer->add_text(std::to_string(static_cast<int>(node.eval())));
}


void CodegenCppVisitor::visit_name(const Name& node) {
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
    node.get_name()->accept(*this);
    printer->add_text("[");
    printer->add_text("static_cast<int>(");
    node.get_length()->accept(*this);
    printer->add_text(")");
    printer->add_text("]");
}


void CodegenCppVisitor::visit_local_list_statement(const LocalListStatement& node) {
    printer->add_text(local_var_type(), ' ');
    print_vector_elements(node.get_variables(), ", ");
}


void CodegenCppVisitor::visit_if_statement(const IfStatement& node) {
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
    printer->add_text(" else if (");
    node.get_condition()->accept(*this);
    printer->add_text(") ");
    node.get_statement_block()->accept(*this);
}


void CodegenCppVisitor::visit_else_statement(const ElseStatement& node) {
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
    printer->add_text("(");
    node.get_expression()->accept(*this);
    printer->add_text(")");
}


void CodegenCppVisitor::visit_binary_expression(const BinaryExpression& node) {
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
    printer->add_text(node.eval());
}


void CodegenCppVisitor::visit_unary_operator(const UnaryOperator& node) {
    printer->add_text(" " + node.eval());
}


/**
 * \details Statement block is top level construct (for every nmodl block).
 * Sometime we want to analyse ast nodes even if code generation is
 * false. Hence we visit children even if code generation is false.
 */
void CodegenCppVisitor::visit_statement_block(const StatementBlock& node) {
    print_statement_block(node);
}


void CodegenCppVisitor::visit_function_call(const FunctionCall& node) {
    print_function_call(node);
}


void CodegenCppVisitor::visit_verbatim(const Verbatim& node) {
    const auto& text = node.get_statement()->eval();
    printer->add_line("// VERBATIM");
    const auto& result = process_verbatim_text(text);

    const auto& statements = stringutils::split_string(result, '\n');
    for (const auto& statement: statements) {
        const auto& trimed_stmt = stringutils::trim_newline(statement);
        if (trimed_stmt.find_first_not_of(' ') != std::string::npos) {
            printer->add_line(trimed_stmt);
        }
    }
    printer->add_line("// ENDVERBATIM");
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


void CodegenCppVisitor::visit_solution_expression(const SolutionExpression& node) {
    auto block = node.get_node_to_solve().get();
    if (block->is_statement_block()) {
        auto statement_block = dynamic_cast<ast::StatementBlock*>(block);
        print_statement_block(*statement_block, false, false);
    } else {
        block->accept(*this);
    }
}


void CodegenCppVisitor::visit_eigen_newton_solver_block(const ast::EigenNewtonSolverBlock& node) {
    // solution vector to store copy of state vars for Newton solver
    printer->add_newline();

    auto float_type = default_float_data_type();
    int N = node.get_n_state_vars()->get_value();
    printer->fmt_line("Eigen::Matrix<{}, {}, 1> nmodl_eigen_xm;", float_type, N);
    printer->fmt_line("{}* nmodl_eigen_x = nmodl_eigen_xm.data();", float_type);

    print_statement_block(*node.get_setup_x_block(), false, false);

    // call newton solver with functor and X matrix that contains state vars
    printer->add_line("// call newton solver");
    printer->fmt_line("{} newton_functor({});",
                      info.functor_names[&node],
                      get_arg_str(functor_params()));
    printer->add_line("newton_functor.initialize();");
    printer->add_line(
        "int newton_iterations = nmodl::newton::newton_solver(nmodl_eigen_xm, newton_functor);");
    printer->add_line(
        "if (newton_iterations < 0) assert(false && \"Newton solver did not converge!\");");

    // assign newton solver results in matrix X to state vars
    print_statement_block(*node.get_update_states_block(), false, false);
    printer->add_line("newton_functor.initialize(); // TODO mimic calling F again.");
    printer->add_line("newton_functor.finalize();");
}


void CodegenCppVisitor::visit_eigen_linear_solver_block(const ast::EigenLinearSolverBlock& node) {
    printer->add_newline();

    const std::string float_type = default_float_data_type();
    int N = node.get_n_state_vars()->get_value();
    printer->fmt_line("Eigen::Matrix<{0}, {1}, 1> nmodl_eigen_xm, nmodl_eigen_fm;", float_type, N);
    printer->fmt_line("Eigen::Matrix<{0}, {1}, {1}> nmodl_eigen_jm;", float_type, N);
    if (N <= 4) {
        printer->fmt_line("Eigen::Matrix<{0}, {1}, {1}> nmodl_eigen_jm_inv;", float_type, N);
    }
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

    for (auto& var: info.random_variables) {
        if (info.first_random_var_index == -1) {
            info.first_random_var_index = index;
        }
        int size = var->get_length();
        info.semantics.emplace_back(index, naming::RANDOM_SEMANTIC, size);
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

    for (const auto& v: assigned) {
        auto it = std::find_if(info.external_variables.begin(),
                               info.external_variables.end(),
                               [&v](auto it) { return it->get_name() == get_name(v); });

        if (it == info.external_variables.end()) {
            variables.push_back(v);
        }
    }

    if (needs_v_unused()) {
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

        add_variable_point_process(variables);
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

    for (const auto& var: info.random_variables) {
        auto name = var->get_name();
        variables.emplace_back(make_symbol(name), true);
        variables.back().symbol->add_properties(NmodlType::random_var);
    }

    if (info.diam_used) {
        variables.emplace_back(make_symbol(naming::DIAM_VARIABLE));
    }

    if (info.area_used) {
        variables.emplace_back(make_symbol(naming::AREA_VARIABLE));
    }

    add_variable_tqitem(variables);

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

    if (info.for_netcon_used) {
        variables.emplace_back(make_symbol(naming::FOR_NETCON_VARIABLE), false, false, true);
    }
    return variables;
}


void CodegenCppVisitor::setup(const Program& node) {
    program_symtab = node.get_symbol_table();

    CodegenHelperVisitor v;
    info = v.analyze(node);
    info.mod_file = mod_filename;

    if (info.mod_suffix == "") {
        info.mod_suffix = std::filesystem::path(mod_filename).stem().string();
    }
    info.rsuffix = info.point_process ? "" : "_" + info.mod_suffix;
    if (info.mod_suffix == "nothing") {
        info.rsuffix = "";
    }

    if (!info.vectorize) {
        logger->warn(
            "CodegenCoreneuronCppVisitor : MOD file uses non-thread safe constructs of NMODL");
    }

    codegen_float_variables = get_float_variables();
    codegen_int_variables = get_int_variables();

    update_index_semantics();

    info.semantic_variable_count = int_variables_size();
}


std::string CodegenCppVisitor::compute_method_name(BlockType type) const {
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


void CodegenCppVisitor::visit_program(const Program& node) {
    setup(node);
    print_codegen_routines();
}


void CodegenCppVisitor::print_table_replacement_function(const ast::Block& node) {
    auto name = node.get_node_name();
    auto statement = get_table_statement(node);
    auto table_variables = statement->get_table_vars();
    auto with = statement->get_with()->eval();
    auto use_table_var = get_variable_name(naming::USE_TABLE_VARIABLE);
    auto tmin_name = get_variable_name("tmin_" + name);
    auto mfac_name = get_variable_name("mfac_" + name);
    auto function_name = method_name("f_" + name);

    printer->add_newline(2);
    print_function_declaration(node, name);
    printer->push_block();
    {
        const auto& params = node.get_parameters();
        printer->fmt_push_block("if ({} == 0)", use_table_var);
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
        printer->pop_block();

        printer->fmt_line("double xi = {} * ({} - {});",
                          mfac_name,
                          params[0].get()->get_node_name(),
                          tmin_name);
        printer->push_block("if (isnan(xi))");
        if (node.is_procedure_block()) {
            for (const auto& var: table_variables) {
                auto var_name = get_variable_name(var->get_node_name());
                auto [is_array, array_length] = check_if_var_is_array(var->get_node_name());
                if (is_array) {
                    for (int j = 0; j < array_length; j++) {
                        printer->fmt_line("{}[{}] = xi;", var_name, j);
                    }
                } else {
                    printer->fmt_line("{} = xi;", var_name);
                }
            }
            printer->add_line("return 0;");
        } else {
            printer->add_line("return xi;");
        }
        printer->pop_block();

        printer->fmt_push_block("if (xi <= 0. || xi >= {}.)", with);
        printer->fmt_line("int index = (xi <= 0.) ? 0 : {};", with);
        if (node.is_procedure_block()) {
            for (const auto& variable: table_variables) {
                auto var_name = variable->get_node_name();
                auto instance_name = get_variable_name(var_name);
                auto table_name = get_variable_name("t_" + var_name);
                auto [is_array, array_length] = check_if_var_is_array(var_name);
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
        printer->pop_block();

        printer->add_line("int i = int(xi);");
        printer->add_line("double theta = xi - double(i);");
        if (node.is_procedure_block()) {
            for (const auto& var: table_variables) {
                auto var_name = var->get_node_name();
                auto instance_name = get_variable_name(var_name);
                auto table_name = get_variable_name("t_" + var_name);
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
    printer->pop_block();
}


void CodegenCppVisitor::print_table_check_function(const Block& node) {
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
    printer->fmt_push_block("void {}({})",
                            table_update_function_name(name),
                            get_parameter_str(internal_params));
    {
        printer->fmt_push_block("if ({} == 0)", use_table_var);
        printer->add_line("return;");
        printer->pop_block();

        printer->add_line("static bool make_table = true;");
        for (const auto& variable: depend_variables) {
            printer->fmt_line("static {} save_{};", float_type, variable->get_node_name());
        }

        for (const auto& variable: depend_variables) {
            const auto& var_name = variable->get_node_name();
            const auto& instance_name = get_variable_name(var_name);
            printer->fmt_push_block("if (save_{} != {})", var_name, instance_name);
            printer->add_line("make_table = true;");
            printer->pop_block();
        }

        printer->push_block("if (make_table)");
        {
            printer->add_line("make_table = false;");

            printer->add_indent();
            printer->add_text(tmin_name, " = ");
            from->accept(*this);
            printer->add_text(';');
            printer->add_newline();

            printer->add_indent();
            printer->add_text("double tmax = ");
            to->accept(*this);
            printer->add_text(';');
            printer->add_newline();


            printer->fmt_line("double dx = (tmax-{}) / {}.;", tmin_name, with);
            printer->fmt_line("{} = 1./dx;", mfac_name);

            printer->fmt_line("double x = {};", tmin_name);
            printer->fmt_push_block("for (std::size_t i = 0; i < {}; x += dx, i++)", with + 1);
            auto function = method_name("f_" + name);
            if (node.is_procedure_block()) {
                printer->fmt_line("{}({}, x);", function, internal_method_arguments());
                for (const auto& variable: table_variables) {
                    auto var_name = variable->get_node_name();
                    auto instance_name = get_variable_name(var_name);
                    auto table_name = get_variable_name("t_" + var_name);
                    auto [is_array, array_length] = check_if_var_is_array(var_name);
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
            printer->pop_block();

            for (const auto& variable: depend_variables) {
                auto var_name = variable->get_node_name();
                auto instance_name = get_variable_name(var_name);
                printer->fmt_line("save_{} = {};", var_name, instance_name);
            }
        }
        printer->pop_block();
    }
    printer->pop_block();
}


std::string CodegenCppVisitor::get_object_specifiers(
    const std::unordered_set<CppObjectSpecifier>& specifiers) {
    std::string result;
    for (const auto& specifier: specifiers) {
        if (!result.empty()) {
            result += " ";
        }
        result += object_specifier_map[specifier];
    }
    return result;
}


const ast::TableStatement* CodegenCppVisitor::get_table_statement(const ast::Block& node) {
    const auto& table_statements = collect_nodes(node, {AstNodeType::TABLE_STATEMENT});

    if (table_statements.size() != 1) {
        auto message = fmt::format("One table statement expected in {} found {}",
                                   node.get_node_name(),
                                   table_statements.size());
        throw std::runtime_error(message);
    }
    return dynamic_cast<const ast::TableStatement*>(table_statements.front().get());
}


std::tuple<bool, int> CodegenCppVisitor::check_if_var_is_array(const std::string& name) {
    auto symbol = program_symtab->lookup_in_scope(name);
    if (!symbol) {
        throw std::runtime_error(
            fmt::format("CodegenCppVisitor:: {} not found in symbol table!", name));
    }
    if (symbol->is_array()) {
        return {true, symbol->get_length()};
    } else {
        return {false, 0};
    }
}


void CodegenCppVisitor::print_rename_state_vars() const {
    for (const auto& state: info.state_vars) {
        auto state_name = state->get_name();
        auto lhs = get_variable_name(state_name);
        auto rhs = get_variable_name(state_name + "0");

        if (state->is_array()) {
            for (int i = 0; i < state->get_length(); ++i) {
                printer->fmt_line("{}[{}] = {};", lhs, i, rhs);
            }
        } else {
            printer->fmt_line("{} = {};", lhs, rhs);
        }
    }
}
}  // namespace codegen
}  // namespace nmodl
