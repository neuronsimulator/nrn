#include <algorithm>
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
/*                            Overloaded visitor routines                               */
/****************************************************************************************/



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
    for (auto& statement : statements) {
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
        return ion_read_statements_optimal(type);
    }
    std::vector<std::string> statements;
    for (auto& ion : info.ions) {
        auto name = ion.name;
        for (auto& var : ion.reads) {
            if (type == BlockType::Ode && ion.is_ionic_conc(var) && state_variable(var)) {
                continue;
            }
            auto variable_names = read_ion_variable_name(var);
            auto first = get_variable_name(variable_names.first);
            auto second = get_variable_name(variable_names.second);
            statements.push_back("{} = {};"_format(first, second));
        }
        for (auto& var : ion.writes) {
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


std::vector<std::string> CodegenCVisitor::ion_read_statements_optimal(BlockType type) {
    std::vector<std::string> statements;
    for (auto& ion : info.ions) {
        for (auto& var : ion.writes) {
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
    for (auto& ion : info.ions) {
        std::string concentration;
        auto name = ion.name;
        for (auto& var : ion.writes) {
            auto variable_names = write_ion_variable_name(var);
            if (ion.is_ionic_current(var)) {
                if (type == BlockType::Equation) {
                    auto current = breakpoint_current(var);
                    auto lhs = variable_names.first;
                    auto op = "+=";
                    auto rhs = get_variable_name(current);
                    if (info.point_process) {
                        auto area = get_variable_name(node_area);
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
    if (printing_top_verbatim_blocks) {
        std::string name = token;
        if (verbatim_variables_mapping.find(token) != verbatim_variables_mapping.end()) {
            name = verbatim_variables_mapping[token];
        }
        return get_variable_name(name, false);
    }
    return get_variable_name(token);
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
        if (symbol->has_properties(NmodlInfo::read_ion_var)) {
            is_constant = true;
        }
        if (symbol->has_properties(NmodlInfo::param_assign) && symbol->get_write_count() == 0) {
            is_constant = true;
        }
    }
    return is_constant;
}



/****************************************************************************************/
/*                      Routines must be overloaded in backend                          */
/****************************************************************************************/


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
 *  void nrn_state(...) {
 *      #pragma acc data present (nt, ml...)
 *      {
 *
 *      }
 *  }
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
 *      for(int id=0; id<nodecount; id++) {
 *
 *      #pragma acc parallel loop
 *      for(int id=0; id<nodecount; id++) {
 *
 */
void CodegenCVisitor::print_channel_iteration_block_parallel_hint() {
    printer->add_line("#pragma ivdep");
}



/// if reduction block in nrn_cur required
bool CodegenCVisitor::nrn_cur_reduction_loop_required() {
    return channel_task_dependency_enabled() || info.point_process;
}



/*
 * Depending on the backend, print condition/loop for iterating over channels
 *
 * For CPU backend we iterate over all node counts. For cuda we use thread
 * index to check if block needs to be executed or not.
 */
void CodegenCVisitor::print_channel_iteration_block_begin() {
    print_channel_iteration_block_parallel_hint();
    printer->start_block("for (int id = start; id < end; id++) ");
}


void CodegenCVisitor::print_channel_iteration_block_end() {
    printer->end_block();
    printer->add_newline();
}


void CodegenCVisitor::print_rhs_d_shadow_variables() {
    if (info.point_process) {
        printer->add_line("double* shadow_rhs = nt->_shadow_rhs;");
        printer->add_line("double* shadow_d = nt->_shadow_d;");
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
    for (auto& statement : shadow_statements) {
        print_atomic_reduction_pragma();
        auto lhs = get_variable_name(statement.lhs);
        auto rhs = get_variable_name(shadow_varname(statement.lhs));
        auto text = "{} {} {};"_format(lhs, statement.op, rhs);
        printer->add_line(text);
    }
    shadow_statements.clear();
}


void CodegenCVisitor::print_shadow_reduction_block_end() {
    printer->end_block();
    printer->add_newline();
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


void CodegenCVisitor::print_backend_namespace_end() {
    // no separate namespace for C (cpu) backend
}


void CodegenCVisitor::print_backend_includes() {
    // backend specific, do nothing
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
        return method_name("nrn_init");
    }
    if (type == BlockType::State) {
        return method_name("nrn_state");
    }
    if (type == BlockType::Equation) {
        return method_name("nrn_cur");
    }
    throw std::logic_error("compute_method_name not implemented");
}


// note extra empty space for pretty-printing if we skip the symbol
std::string CodegenCVisitor::k_restrict() {
    return "__restrict__ ";
}


std::string CodegenCVisitor::k_const() {
    return "const ";
}


/****************************************************************************************/
/*               Non-code-specific printing routines for code generation                */
/****************************************************************************************/



void CodegenCVisitor::print_statement_block(ast::StatementBlock* node,
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
        /// not necessary to add indent for vebatim block (pretty-printing)
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
    auto name = node->get_function_name()->get_name();
    auto function_name = name;
    if (defined_method(name)) {
        function_name = method_name(name);
    }

    if (is_net_send(name)) {
        print_net_send_call(node);
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
    print_namespace_end();

    printer->add_newline(2);
    printer->add_line("using namespace coreneuron;");
    codegen = true;
    printing_top_verbatim_blocks = true;

    for (auto& block : info.top_blocks) {
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
    for (auto& arg : default_arguments) {
        stringutils::trim(arg);
        RenameVisitor v(arg, "arg_" + arg);
        for (const auto& function : info.functions) {
            function->accept(&v);
        }
        for (const auto& function : info.procedures) {
            function->accept(&v);
        }
    }
}


void CodegenCVisitor::print_function_prototypes() {
    if (info.functions.empty() && info.procedures.empty()) {
        return;
    }
    codegen = true;
    printer->add_newline(2);
    for (const auto& function : info.functions) {
        print_function_declaration(function);
        printer->add_text(";");
        printer->add_newline();
    }
    for (const auto& function : info.procedures) {
        print_function_declaration(function);
        printer->add_text(";");
        printer->add_newline();
    }
    codegen = false;
}


void CodegenCVisitor::print_procedure(ast::ProcedureBlock* node) {
    codegen = true;
    auto parameters = node->get_arguments();

    printer->add_newline(2);
    print_function_declaration(node);
    printer->add_text(" ");
    printer->start_block();
    print_statement_block(node->get_statement_block().get(), false, false);
    printer->add_line("return 0;");
    printer->end_block();
    printer->add_newline();
    codegen = false;
}


void CodegenCVisitor::print_function(ast::FunctionBlock* node) {
    codegen = true;
    auto name = node->get_name();
    auto return_var = "ret_" + name;
    auto type = float_data_type() + " ";

    /// first rename return variable name
    auto block = node->get_statement_block().get();
    RenameVisitor v(name, return_var);
    block->accept(&v);

    printer->add_newline(2);
    print_function_declaration(node);
    printer->add_text(" ");
    printer->start_block();
    printer->add_line("{}{} = 0.0;"_format(type, return_var));
    print_statement_block(block, false, false);
    printer->add_line("return {};"_format(return_var));
    printer->end_block();
    printer->add_newline();
    codegen = false;
}



/****************************************************************************************/
/*                           Code-specific helper routines                              */
/****************************************************************************************/



std::string CodegenCVisitor::process_verbatim_text(std::string text) {
    c11::Driver driver;
    driver.scan_string(text);
    auto tokens = driver.all_tokens();
    std::string result;
    for (auto& token : tokens) {
        auto name = process_verbatim_token(token);
        if (token == "_tqitem") {
            name = "&" + name;
        }
        if (token == "_STRIDE") {
            name = (layout == LayoutType::soa) ? "pnodecount+id" : "1";
        }
        result += name;
    }
    return result;
}


std::string CodegenCVisitor::internal_method_arguments() {
    if (ion_variable_struct_required()) {
        return "id, pnodecount, inst, ionvar, data, indexes, thread, nt, v";
    }
    return "id, pnodecount, inst, data, indexes, thread, nt, v";
}


std::string CodegenCVisitor::internal_method_parameters() {
    std::string ion_var_arg;
    if (ion_variable_struct_required()) {
        ion_var_arg = " IonCurVar& ionvar,";
    }
    return "int id, int pnodecount, {}* inst,{} double* data, "
           "{}Datum* indexes, ThreadDatum* thread, "
           "NrnThread* nt, double v"_format(instance_struct(), ion_var_arg, k_const());
}


std::string CodegenCVisitor::external_method_arguments() {
    return "id, pnodecount, data, indexes, thread, nt, v";
}


std::string CodegenCVisitor::external_method_parameters() {
    return "int id, int pnodecount, double* data, Datum* indexes, "
           "ThreadDatum* thread, NrnThread* nt, double v";
}


std::string CodegenCVisitor::nrn_thread_arguments() {
    return "id, pnodecount, data, indexes, thread, nt, v";
}


std::string CodegenCVisitor::register_mechanism_arguments() {
    auto nrn_cur = nrn_cur_required() ? method_name("nrn_cur") : "NULL";
    auto nrn_state = nrn_state_required() ? method_name("nrn_state") : "NULL";
    auto nrn_alloc = method_name("nrn_alloc");
    auto nrn_init = method_name("nrn_init");
    return "mechanism, {}, {}, NULL, {}, {}, first_pointer_var_index()"
           ""_format(nrn_alloc, nrn_cur, nrn_state, nrn_init);
}


std::pair<std::string, std::string> CodegenCVisitor::read_ion_variable_name(std::string name) {
    return {name, "ion_" + name};
}


std::pair<std::string, std::string> CodegenCVisitor::write_ion_variable_name(std::string name) {
    return {"ion_" + name, name};
}


std::string CodegenCVisitor::conc_write_statement(std::string ion_name,
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
    printer->add_line("static inline int num_float_variable() {");
    printer->add_line("    return {};"_format(num_float_variable()));
    printer->add_line("}");

    printer->add_newline(2);
    print_device_method_annotation();
    printer->add_line("static inline int num_int_variable() {");
    printer->add_line("    return {};"_format(num_int_variable()));
    printer->add_line("}");
}


void CodegenCVisitor::print_net_receive_arg_size_getter() {
    if (!net_receive_exist()) {
        return;
    }
    printer->add_newline(2);
    print_device_method_annotation();
    printer->add_line("static inline int num_net_receive_args() {");
    printer->add_line("    return {};"_format(info.num_net_receive_arguments));
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
        printer->add_line("data = ml->data + id*{};"_format(num_float_variable()));
        printer->add_line("indexes = ml->pdata + id*{};"_format(num_int_variable()));
    }
}


void CodegenCVisitor::print_namespace_start() {
    printer->add_newline(2);
    printer->start_block("namespace coreneuron");
}


void CodegenCVisitor::print_namespace_end() {
    printer->end_block();
    printer->add_newline();
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
    if (info.vectorize && info.derivimplicit_used) {
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
    auto num_float = num_float_variable();
    auto position = position_of_float_var(name);
    // clang-format off
    if (symbol->is_array()) {
        if (use_instance) {
            auto stride = (layout == LayoutType::soa) ? dimension : num_float;
            return "(inst->{}+id*{})"_format(name, stride);
        }
        auto stride = (layout == LayoutType::soa) ? "{}*pnodecount+id*{}"_format(position, dimension) : "{}"_format(position);
        return "(data+{})"_format(stride);
    } else {
        if (use_instance) {
            auto stride = (layout == LayoutType::soa) ? "id" : "id*{}"_format(num_float);
            return "inst->{}[{}]"_format(name, stride);
        }
        auto stride = (layout == LayoutType::soa) ? "{}*pnodecount+id"_format(position) : "{}"_format(position);
        return "data[{}]"_format(stride);
    }
    // clang-format on
}


std::string CodegenCVisitor::int_variable_name(IndexVariableInfo& symbol,
                                               std::string name,
                                               bool use_instance) {
    auto position = position_of_int_var(name);
    auto num_int = num_int_variable();
    std::string offset;
    // clang-format off
    if (symbol.is_index) {
        offset = std::to_string(position);
        if (use_instance) {
            return "inst->{}[{}]"_format(name, offset);
        }
        return "indexes[{}]"_format(offset);
    } else if (symbol.is_integer) {
        if (use_instance) {
            offset = (layout == LayoutType::soa) ? "{}*pnodecount+id"_format(position) : "id*{}+{}"_format(num_int, position);
            return "inst->{}[{}]"_format(name, offset);
        }
        offset = (layout == LayoutType::soa) ? "{}*pnodecount+id"_format(position) : "id";
        return "indexes[{}]"_format(offset);
    } else {
        offset = (layout == LayoutType::soa) ? "{}*pnodecount+id"_format(position) : "{}"_format(position);
        if (use_instance) {
            return "inst->{}[indexes[{}]]"_format(name, offset);
        }
        auto data = symbol.is_vdata ? "_vdata" : "_data";
        return "nt->{}[indexes[{}]]"_format(data, offset);
    }
    // clang-format on
}


std::string CodegenCVisitor::global_variable_name(SymbolType& symbol) {
    return "{}_global->{}"_format(info.mod_suffix, symbol->get_name());
}


std::string CodegenCVisitor::ion_shadow_variable_name(SymbolType& symbol) {
    return "inst->{}[id]"_format(symbol->get_name());
}


std::string CodegenCVisitor::update_if_ion_variable_name(std::string name) {
    if (ion_variable_struct_required()) {
        if (info.is_ion_read_variable(name)) {
            return "ion_" + name;
        }
        if (info.is_ion_write_variable(name)) {
            return "ionvar." + name;
        }
        if (info.is_current(name)) {
            return "ionvar." + name;
        }
    }
    return name;
}


/**
 * Return variable name in the structure of mechanism properties
 *
 * @param name variable name that is being printed
 * @return use_instance whether print name using Instance structure (or data array if false)
 */
std::string CodegenCVisitor::get_variable_name(std::string name, bool use_instance) {
    name = update_if_ion_variable_name(name);

    // clang-format off
    auto l_symbol = [&name](const SymbolType& sym) {
                            return name == sym->get_name();
                         };

    auto i_symbol = [&name](const IndexVariableInfo& var) {
                            return name == var.symbol->get_name();
                         };
    // clang-format on

    /// float variable
    auto f = std::find_if(float_variables.begin(), float_variables.end(), l_symbol);
    if (f != float_variables.end()) {
        return float_variable_name(*f, use_instance);
    }

    /// integer variable
    auto i = std::find_if(int_variables.begin(), int_variables.end(), i_symbol);
    if (i != int_variables.end()) {
        return int_variable_name(*i, name, use_instance);
    }

    /// global variable
    auto g = std::find_if(global_variables.begin(), global_variables.end(), l_symbol);
    if (g != global_variables.end()) {
        return global_variable_name(*g);
    }

    /// shadow variable
    auto s = std::find_if(shadow_variables.begin(), shadow_variables.end(), l_symbol);
    if (s != shadow_variables.end()) {
        return ion_shadow_variable_name(*s);
    }

    /// otherwise return original name
    return name;
}


/****************************************************************************************/
/*                      Main printing routines for code generation                      */
/****************************************************************************************/


void CodegenCVisitor::print_backend_info() {
    time_t tr;
    time(&tr);
    auto date = std::string(asctime(localtime(&tr)));
    auto version = nocmodl::version::NOCMODL_VERSION + " [" + nocmodl::version::GIT_REVISION + "]";

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
    printer->add_line("#include <stdio.h>");
    printer->add_line("#include <stdlib.h>");
    printer->add_line("#include <math.h>");
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
void CodegenCVisitor::print_mechanism_global_structure() {
    auto float_type = float_data_type();
    printer->add_newline(2);
    printer->add_line("/** all global variables */");
    printer->add_line("struct {} {}"_format(global_struct(), "{"));
    printer->increase_indent();

    if (!info.ions.empty()) {
        for (const auto& ion : info.ions) {
            auto name = "{}_type"_format(ion.name);
            printer->add_line("int {};"_format(name));
            global_variables.push_back(make_symbol(name));
        }
    }

    if (info.point_process) {
        printer->add_line("int point_type;");
        global_variables.push_back(make_symbol("point_type"));
    }

    if (!info.state_vars.empty()) {
        for (auto& var : info.state_vars) {
            auto name = var->get_name() + "0";
            auto symbol = program_symtab->lookup(name);
            if (symbol == nullptr) {
                printer->add_line("{} {};"_format(float_type, name));
                global_variables.push_back(make_symbol(name));
            }
        }
    }

    if (!info.vectorize) {
        printer->add_line("{} v;"_format(float_type));
        global_variables.push_back(make_symbol("v"));
    }

    auto& top_locals = info.top_local_variables;
    if (!info.vectorize && !top_locals.empty()) {
        for (auto& var : top_locals) {
            auto name = var->get_name();
            auto length = var->get_length();
            if (var->is_array()) {
                printer->add_line("{} {}[{}];"_format(float_type, name, length));
            } else {
                printer->add_line("{} {};"_format(float_type, name));
            }
            global_variables.push_back(var);
        }
    }

    if (!info.thread_variables.empty()) {
        printer->add_line("int thread_data_in_use;");
        printer->add_line("{} thread_data[{}];"_format(float_type, info.thread_var_data_size));
        global_variables.push_back(make_symbol("thread_data_in_use"));
        auto symbol = make_symbol("thread_data");
        symbol->set_as_array(info.thread_var_data_size);
        global_variables.push_back(symbol);
    }

    printer->add_line("int reset;");
    global_variables.push_back(make_symbol("reset"));

    printer->add_line("int mech_type;");
    global_variables.push_back(make_symbol("mech_type"));

    auto& globals = info.global_variables;
    auto& constants = info.constant_variables;

    if (!globals.empty()) {
        for (auto& var : globals) {
            auto name = var->get_name();
            auto length = var->get_length();
            if (var->is_array()) {
                printer->add_line("{} {}[{}];"_format(float_type, name, length));
            } else {
                printer->add_line("{} {};"_format(float_type, name));
            }
            global_variables.push_back(var);
        }
    }

    if (!constants.empty()) {
        for (auto& var : constants) {
            auto name = var->get_name();
            auto value_ptr = var->get_value();
            printer->add_line("{} {};"_format(float_type, name));
            global_variables.push_back(var);
        }
    }

    if (info.primes_size != 0) {
        printer->add_line("int* slist1;");
        printer->add_line("int* dlist1;");
        global_variables.push_back(make_symbol("slist1"));
        global_variables.push_back(make_symbol("dlist1"));
        if (info.derivimplicit_used) {
            printer->add_line("int* slist2;");
            global_variables.push_back(make_symbol("slist2"));
        }
    }

    if (info.vectorize) {
        printer->add_line("ThreadDatum* {}ext_call_thread;"_format(k_restrict()));
        global_variables.push_back(make_symbol("ext_call_thread"));
    }

    printer->decrease_indent();
    printer->add_line("};");

    printer->add_newline(1);
    printer->add_line("/** holds object of global variable */");
    printer->add_line("{}* {}{}_global;"_format(global_struct(), k_restrict(), info.mod_suffix));
}


void CodegenCVisitor::print_mechanism_info_array() {
    auto variable_printer = [this](std::vector<SymbolType>& variables) {
        for (auto& v : variables) {
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
    auto variable_printer = [this](std::vector<SymbolType>& variables, bool if_array,
                                   bool if_vector) {
        for (auto& variable : variables) {
            if (variable->is_array() == if_array) {
                auto name = get_variable_name(variable->get_name());
                auto ename = add_escape_quote(variable->get_name());
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
    for (const auto& ion : info.ions) {
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
        printer->add_line("{} = 0;"_format(get_variable_name("thread_data_in_use")));
    }
    if (info.thread_callback_register) {
        printer->add_line("_nrn_thread_reg0(mech_type, thread_mem_cleanup);");
        printer->add_line("_nrn_thread_reg1(mech_type, thread_mem_init);");
    }
    if (info.emit_table_thread) {
        printer->add_line("nrn_thread_table_reg(mech_type, check_table_thread);");
    }

    /// register read/write callbacks for pointers
    if (info.bbcore_pointer_used) {
        printer->add_line("hoc_reg_bbcore_read(mech_type, bbcore_read);");
        printer->add_line("hoc_reg_bbcore_write(mech_type, bbcore_write);");
    }

    /// register size of double and int elements
    // clang-format off
    printer->add_line("hoc_register_prop_size(mech_type, num_float_variable(), num_int_variable());");
    // clang-format on

    /// register semantics for index variables
    for (auto& semantic : info.semantics) {
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
        printer->add_line("hoc_register_net_receive_buffering(net_buf_receive, mech_type);");
    }
    if (info.num_net_receive_arguments != 0) {
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
    printer->end_block();
    printer->add_newline();
}


void CodegenCVisitor::print_thread_memory_callbacks() {
    if (!info.thread_callback_register) {
        return;
    }

    /// thread_mem_init callback
    printer->add_newline(2);
    printer->add_line("/** thread memory allocation callback */");
    printer->start_block("static void thread_mem_init(ThreadDatum* thread) ");

    if (info.vectorize && info.derivimplicit_used) {
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
    printer->end_block();
    printer->add_newline();


    /// thread_mem_cleanup callback
    printer->add_newline(2);
    printer->add_line("/** thread memory cleanup callback */");
    printer->start_block("static void thread_mem_cleanup(ThreadDatum* thread) ");

    if (info.vectorize && info.derivimplicit_used) {
        int n = info.derivimplicit_list_num;
        printer->add_line("free(thread[dith{}()].pval);"_format(n));
        printer->add_line("nrn_destroy_newtonspace(*newtonspace{}(thread));"_format(n));
    }
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
    printer->end_block();
    printer->add_newline();
}


void CodegenCVisitor::print_mechanism_var_structure() {
    auto float_type = float_data_type();
    auto int_type = int_data_type();
    printer->add_newline(2);
    printer->add_line("/** all mechanism instance variables */");
    printer->start_block("struct {} "_format(instance_struct()));
    for (auto& var : float_variables) {
        auto name = var->get_name();
        auto qualifier = is_constant_variable(name) ? k_const() : "";
        printer->add_line("{}{}* {}{};"_format(qualifier, float_type, k_restrict(), name));
    }
    for (auto& var : int_variables) {
        auto name = var.symbol->get_name();
        if (var.is_index || var.is_integer) {
            printer->add_line("{}{}* {}{};"_format(k_const(), int_type, k_restrict(), name));
        } else {
            auto qualifier = var.is_constant ? k_const() : "";
            auto type = var.is_vdata ? "void*" : float_data_type();
            printer->add_line("{}{}* {}{};"_format(qualifier, type, k_restrict(), name));
        }
    }
    if (channel_task_dependency_enabled()) {
        for (auto& var : shadow_variables) {
            auto name = var->get_name();
            printer->add_line("{}* {}{};"_format(float_type, k_restrict(), name));
        }
    }
    printer->end_block();
    printer->add_text(";");
    printer->add_newline();
}


void CodegenCVisitor::print_ion_variables_structure() {
    if (!ion_variable_struct_required()) {
        return;
    }
    printer->add_newline(2);
    printer->add_line("/** ion write variables */");
    printer->start_block("struct IonCurVar");
    for (auto& ion : info.ions) {
        for (auto& var : ion.writes) {
            printer->add_line("{} {};"_format(float_data_type(), var));
        }
    }
    for (auto& var : info.currents) {
        if (!info.is_ion_variable(var)) {
            printer->add_line("{} {};"_format(float_data_type(), var));
        }
    }
    printer->end_block();
    printer->add_text(";");
    printer->add_newline();
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

    printer->add_newline(1);
    auto allocation = "({0}*) mem_alloc(1, sizeof({0}))"_format(global_struct());
    printer->add_line("{0}_global = {1};"_format(info.mod_suffix, allocation));

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
        for (auto& prime : info.prime_variables_by_order) {
            auto name = prime->get_name();
            printer->add_line("{}[{}] = {};"_format(slist1, id, position_of_float_var(name)));
            printer->add_line("{}[{}] = {};"_format(dlist1, id, position_of_float_var("D" + name)));
            id++;
        }
    }

    /// additional list for derivimplicit method
    if (info.derivimplicit_used) {
        auto primes = program_symtab->get_variables_with_properties(NmodlInfo::prime_name);
        auto slist2 = get_variable_name("slist2");
        auto nprimes = info.primes_size;
        printer->add_line("{} = (int*) mem_alloc({}, sizeof(int));"_format(slist2, nprimes));
        int id = 0;
        for (auto& variable : primes) {
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
    for (auto& var : info.state_vars) {
        auto name = var->get_name() + "0";
        auto symbol = program_symtab->lookup(name);
        if (symbol == nullptr) {
            auto global_name = get_variable_name(name);
            printer->add_line("{} = 0.0;"_format(global_name));
        }
    }
    if (!info.vectorize) {
        printer->add_line("{} = 0.0;"_format(get_variable_name("v")));
    }
    if (!info.thread_variables.empty()) {
        printer->add_line("{} = 0;"_format(get_variable_name("thread_data_in_use")));
    }

    /// initialize global variables
    for (auto& var : info.global_variables) {
        if (!var->is_array()) {
            auto name = get_variable_name(var->get_name());
            double value = 0;
            auto value_ptr = var->get_value();
            if (value_ptr != nullptr) {
                value = *value_ptr;
            }
            printer->add_line("{} = {};"_format(name, double_to_string(value)));
        }
    }

    /// initialize constant variables
    for (auto& var : info.constant_variables) {
        auto name = get_variable_name(var->get_name());
        auto value_ptr = var->get_value();
        double value = 0;
        if (value_ptr != nullptr) {
            value = *value_ptr;
        }
        printer->add_line("{} = {};"_format(name, double_to_string(value)));
    }

    printer->add_newline();
    printer->add_line("setup_done = 1;");

    printer->end_block();
    printer->add_newline();

    printer->add_newline(2);
    printer->add_line("/** free global variables */");
    printer->start_block("static inline void free_global_variables() ");
    if (allocated_variables.empty()) {
        printer->add_line("// do nothing");
    } else {
        for (auto& var : allocated_variables) {
            printer->add_line("mem_free({});"_format(var));
        }
    }
    printer->end_block();
    printer->add_newline();
}


void CodegenCVisitor::print_shadow_vector_setup() {
    printer->add_newline(2);
    printer->add_line("/** allocate and initialize shadow vector */");
    auto args = "{}* inst, Memb_list* ml"_format(instance_struct());
    printer->start_block("static inline void setup_shadow_vectors({}) "_format(args));
    if (channel_task_dependency_enabled()) {
        printer->add_line("int nodecount = ml->nodecount;");
        for (auto& var : shadow_variables) {
            auto name = var->get_name();
            auto type = float_data_type();
            auto allocation = "({0}*) mem_alloc(nodecount, sizeof({0}))"_format(type);
            printer->add_line("inst->{0} = {1};"_format(name, allocation));
        }
    }
    printer->end_block();
    printer->add_newline();

    printer->add_newline(2);
    printer->add_line("/** free shadow vector */");
    args = "{}* inst"_format(instance_struct());
    printer->start_block("static inline void free_shadow_vectors({}) "_format(args));
    if (channel_task_dependency_enabled()) {
        for (auto& var : shadow_variables) {
            auto name = var->get_name();
            printer->add_line("mem_free(inst->{});"_format(name));
        }
    }
    printer->end_block();
    printer->add_newline();
}


void CodegenCVisitor::print_instance_variable_setup() {
    if (channel_task_dependency_enabled() && !shadow_variables.empty()) {
        print_shadow_vector_setup();
    }
    printer->add_newline(2);
    printer->add_line("/** initialize mechanism instance variables */");
    printer->start_block("static inline void setup_instance(NrnThread* nt, Memb_list* ml) ");
    printer->add_line("{0}* inst = ({0}*) mem_alloc(1, sizeof({0}));"_format(instance_struct()));
    if (channel_task_dependency_enabled() && !shadow_variables.empty()) {
        printer->add_line("setup_shadow_vectors(inst, ml);");
    }

    std::string stride;
    if (layout == LayoutType::soa) {
        printer->add_line("int pnodecount = ml->_nodecount_padded;");
        stride = "*pnodecount";
    }

    printer->add_line("Datum* indexes = ml->pdata;");
    int id = 0;
    for (auto& var : float_variables) {
        printer->add_line("inst->{} = ml->data + {}{};"_format(var->get_name(), id, stride));
        id += var->get_length();
    }

    for (auto& var : int_variables) {
        auto name = var.symbol->get_name();
        if (var.is_index || var.is_integer) {
            printer->add_line("inst->{} = ml->pdata;"_format(name));
        } else {
            auto data = var.is_vdata ? "_vdata" : "_data";
            printer->add_line("inst->{} = nt->{};"_format(name, data));
        }
    }

    printer->add_line("ml->instance = (void*) inst;");
    printer->end_block();
    printer->add_newline();
}


void CodegenCVisitor::print_initial_block(InitialBlock* node) {
    if (info.artificial_cell) {
        printer->add_line("double v = 0.0;");
    } else {
        printer->add_line("int node_id = node_index[id];");
        printer->add_line("double v = voltage[node_id];");
    }

    if (ion_variable_struct_required()) {
        printer->add_line("IonCurVar ionvar = {0};");
    }

    /// read ion statements
    auto read_statements = ion_read_statements(BlockType::Initial);
    for (auto& statement : read_statements) {
        printer->add_line(statement);
    }

    /// initialize state variables (excluding ion state)
    for (auto& var : info.state_vars) {
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
    for (auto& statement : write_statements) {
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
    printer->add_line("{}int* {}node_index = ml->nodeindices;"_format(k_const(), k_restrict()));
    printer->add_line("double* {}data = ml->data;"_format(k_restrict()));
    printer->add_line("{}double* {}voltage = nt->_actual_v;"_format(k_const(), k_restrict()));

    if (type == BlockType::Equation) {
        printer->add_line("double* {} vec_rhs = nt->_actual_rhs;"_format(k_restrict()));
        printer->add_line("double* {} vec_d = nt->_actual_d;"_format(k_restrict()));
        print_rhs_d_shadow_variables();
    }
    printer->add_line("{}Datum* {}indexes = ml->pdata;"_format(k_const(), k_restrict()));
    printer->add_line("ThreadDatum* {}thread = ml->_thread;"_format(k_restrict()));

    if (type == BlockType::Initial) {
        printer->add_newline();
        printer->add_line("setup_instance(nt, ml);");
    }
    // clang-format off
    printer->add_line("{0}* {1}inst = ({0}*) ml->instance;"_format(instance_struct(), k_restrict()));
    // clang-format on
    printer->add_newline(1);
}


void CodegenCVisitor::print_nrn_init() {
    codegen = true;
    printer->add_newline(2);
    printer->add_line("/** initialize channel */");
    print_global_function_common_code(BlockType::Initial);
    if (info.derivimplicit_used) {
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

    print_channel_iteration_tiling_block_begin(BlockType::Initial);
    print_channel_iteration_block_begin();
    print_post_channel_iteration_common_code();
    if (info.net_receive_node != nullptr) {
        printer->add_line("{} = -1e20;"_format(get_variable_name("tsave")));
    }
    print_initial_block(info.initial_node);
    print_channel_iteration_block_end();
    print_shadow_reduction_statements();
    print_channel_iteration_tiling_block_end();

    if (info.derivimplicit_used) {
        printer->add_line("*deriv{}_advance(thread) = 1;"_format(info.derivimplicit_list_num));
    }
    print_kernel_data_present_annotation_block_end();
    printer->end_block();
    printer->add_newline();
    codegen = false;
}


void CodegenCVisitor::print_nrn_alloc() {
    printer->add_newline(2);
    auto method = method_name("nrn_alloc");
    printer->start_block("static void {}(double* data, Datum* indexes, int type) "_format(method));
    printer->add_line("// do nothing");
    printer->end_block();
    printer->add_newline();
}


void CodegenCVisitor::print_net_receive_common_code(Block* node) {
    printer->add_line("int tid = pnt->_tid;");
    printer->add_line("int id = pnt->_i_instance;");
    printer->add_line("double v = 0;");
    printer->add_line("NrnThread* nt = nrn_threads + tid;");
    printer->add_line("Memb_list* ml = nt->_ml_list[pnt->_type];");
    printer->add_line("int nodecount = ml->nodecount;");
    printer->add_line("int pnodecount = ml->_nodecount_padded;");
    printer->add_line("double* data = ml->data;");
    printer->add_line("double* weights = nt->weights;");
    printer->add_line("Datum* indexes = ml->pdata;");
    printer->add_line("ThreadDatum* thread = ml->_thread;");
    printer->add_line("{0}* inst = ({0}*) ml->instance;"_format(instance_struct()));

    /// rename variables but need to see if they are actually used
    auto arguments = info.net_receive_node->get_arguments();
    if (!arguments.empty()) {
        int i = 0;
        printer->add_newline();
        for (auto& argument : arguments) {
            auto name = argument->get_name();
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
        printer->add_text("artcell_net_send(&{}, {}, {}, t+"_format(tqitem, weight_index, pnt));
    } else {
        auto point_process = get_variable_name("point_process");
        printer->add_text("net_send_buffering(");
        printer->add_text("ml->_net_send_buffer, 0, {}, {}, {}, t+"_format(tqitem, weight_index, point_process));
    }
    // clang-format off
    print_vector_elements(arguments, ", ");
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


void CodegenCVisitor::print_net_init_kernel() {
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
    printer->end_block();
    printer->add_newline();
    codegen = false;
}


void CodegenCVisitor::print_net_receive_buffer_kernel() {
    if (!net_receive_required() || info.artificial_cell) {
        return;
    }
    printer->add_newline(2);
    printer->start_block("static inline void net_buf_receive(NrnThread* nt)");
    printer->add_line("Memb_list* ml = get_memb_list(nt);");
    printer->add_line("if (ml == NULL) {");
    printer->add_line("    return;");
    printer->add_line("}");
    printer->add_newline();

    auto net_receive = method_name("net_receive_kernel");
    printer->add_line("NetReceiveBuffer_t* nrb = ml->_net_receive_buffer;");
    printer->add_line("int count = nrb->_displ_cnt;");
    printer->add_line("for (int i = 0; i < count; i++) {");
    printer->add_line("    int start = nrb->_displ[i];");
    printer->add_line("    int end = nrb->_displ[i+1];");
    printer->add_line("    for (int j = start; j < end; j++) {");
    printer->add_line("        int index = nrb->_nrb_index[j];");
    printer->add_line("        int offset = nrb->_pnt_index[index];");
    printer->add_line("        double t = nrb->_nrb_t[i];");
    printer->add_line("        int weight_index = nrb->_weight_index[i];");
    printer->add_line("        double flag = nrb->_nrb_flag[i];");
    printer->add_line("        Point_process* point_process = nt->pntprocs + offset;");
    printer->add_line("        {}(t, point_process, weight_index, flag);"_format(net_receive));
    printer->add_line("    }");
    printer->add_line("}");

    if (info.net_send_used || info.net_event_used) {
        printer->add_newline();
        printer->add_line("NetSendBuffer_t* nsb = ml->_net_send_buffer;");
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
    }

    printer->add_newline();
    if (info.net_send_used || info.net_event_used) {
        printer->add_line("nsb->_cnt = 0;");
    }
    printer->add_line("nrb->_displ_cnt = 0;");
    printer->add_line("nrb->_cnt = 0;");

    printer->end_block();
    printer->add_newline();
}


void CodegenCVisitor::print_net_send_buffer_kernel() {
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
    printer->end_block();
    printer->add_newline();
}


void CodegenCVisitor::print_net_receive() {
    if (!net_receive_required()) {
        return;
    }
    auto node = info.net_receive_node;

    /// rename arguments but need to see if they are actually used
    auto arguments = node->get_arguments();
    for (auto& argument : arguments) {
        auto name = argument->get_name();
        auto var_used = VarUsageVisitor().variable_used(node, name);
        if (var_used) {
            RenameVisitor vr(name, "(*" + name + ")");
            node->get_statement_block()->visit_children(&vr);
        }
    }

    codegen = true;
    printing_net_receive = true;
    std::string function_name = method_name("net_receive_kernel");
    std::string function_arguments = "double t, Point_process* pnt, int weight_index, double flag";
    if (info.artificial_cell) {
        function_name = method_name("net_receive");
        function_arguments = "Point_process* pnt, int weight_index, double flag";
    }

    /// net receive kernel
    printer->add_newline(2);
    printer->start_block("static void {}({}) "_format(function_name, function_arguments));
    print_net_receive_common_code(node);
    printer->add_indent();
    node->get_statement_block()->accept(this);
    printer->add_newline();
    printer->end_block();
    printer->add_newline();

    /// net receive function
    if (!info.artificial_cell) {
        function_name = method_name("net_receive");
        function_arguments = "Point_process* pnt, int weight_index, double flag";
        printer->add_newline(2);
        printer->start_block("static void {}({}) "_format(function_name, function_arguments));
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
        printer->end_block();
        printer->add_newline();
    }
    printing_net_receive = false;
    codegen = false;
}


/**
 * When euler method is used then derivative block is printed as separate function
 * which will be used for the callback from euler_thread function. Otherwise, derivative
 * block is printed as part of nrn_state itself.
 */
void CodegenCVisitor::print_derivative_kernel_for_euler() {
    assert(info.solve_node->is_derivative_block());
    auto node = info.solve_node;
    codegen = true;
    auto instance = "{0}* inst = ({0}*)get_memb_list(nt)->instance;"_format(instance_struct());
    auto arguments = external_method_parameters();

    printer->add_newline(2);
    printer->start_block("int {}({})"_format(node->get_name(), arguments));
    printer->add_line(instance);
    print_statement_block(node->get_statement_block().get(), false, false);
    printer->add_line("return 0;");
    printer->end_block();
    printer->add_newline();
    codegen = false;
}



/**
 * Todo: data is not derived. Need to add instance into instance struct?
 * data used here is wrong in AoS because as in original implementation,
 * data is not incremented every iteration for AoS. May be better to derive
 * actual variable names? [resolved now?]
 * slist needs to added as local variable
 */
void CodegenCVisitor::print_derivative_kernel_for_derivimplicit() {
    assert(info.solve_node->is_derivative_block());
    auto node = info.solve_node;
    codegen = true;

    auto ext_args = external_method_arguments();
    auto ext_params = external_method_parameters();
    auto suffix = info.mod_suffix;
    auto list_num = info.derivimplicit_list_num;
    auto solve_block_name = info.solve_block_name;
    auto primes_size = info.primes_size;
    auto stride = (layout == LayoutType::aos) ? "" : "*pnodecount+id";

    printer->add_newline(2);
    // clang-format off
    printer->start_block("int {}_{}({})"_format(node->get_name(), suffix, ext_params));
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

    auto argument = "{}, slist{}, derivimplicit_{}_{}, dlist{}, {}"_format(primes_size, list_num+1, solve_block_name, suffix, list_num + 1, ext_args);
    printer->add_line("int reset = nrn_newton_thread(*newtonspace{}(thread), {});"_format(list_num, argument));
    printer->add_line("return reset;");
    printer->end_block();
    printer->add_newline();

    printer->add_newline(2);
    printer->start_block("int newton_{}_{}({}) "_format(node->get_name(), info.mod_suffix, external_method_parameters()));
    printer->add_line(instance);
    printer->add_line("double* savstate{} = (double*) thread[dith{}()].pval;"_format(list_num, list_num));
    printer->add_line(slist1);
    printer->add_line(dlist1);
    printer->add_line(dlist2);
    print_statement_block(node->get_statement_block().get(), false, false);
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
    codegen = false;
}


/****************************************************************************************/
/*                                Print nrn_state routine                                */
/****************************************************************************************/


void CodegenCVisitor::print_nrn_state() {
    if (!nrn_state_required()) {
        return;
    }
    codegen = true;

    if (info.euler_used) {
        print_derivative_kernel_for_euler();
    }

    if (info.derivimplicit_used) {
        print_derivative_kernel_for_derivimplicit();
    }

    printer->add_newline(2);
    printer->add_line("/** update state */");
    print_global_function_common_code(BlockType::State);
    print_channel_iteration_tiling_block_begin(BlockType::State);
    print_channel_iteration_block_begin();
    print_post_channel_iteration_common_code();

    printer->add_line("int node_id = node_index[id];");
    printer->add_line("double v = voltage[node_id];");
    if (ion_variable_struct_required()) {
        printer->add_line("IonCurVar ionvar = {0};");
    }

    auto read_statements = ion_read_statements(BlockType::State);
    for (auto& statement : read_statements) {
        printer->add_line(statement);
    }

    auto thread_args = external_method_arguments();
    auto num_primes = info.num_primes;
    auto suffix = info.mod_suffix;
    auto block_name = info.solve_block_name;
    auto num = info.derivimplicit_used ? info.derivimplicit_list_num : info.euler_list_num;
    auto slist = get_variable_name("slist{}"_format(num));
    auto dlist = get_variable_name("dlist{}"_format(num));

    if (info.derivimplicit_used) {
        auto args =
            "{}, {}, {}, derivimplicit_{}_{}, {}"
            ""_format(num_primes, slist, dlist, block_name, suffix, thread_args);
        auto statement = "derivimplicit_thread({});"_format(args);
        printer->add_line(statement);
    } else if (info.euler_used) {
        auto args =
            "{}, {}, {}, euler_{}_{}, {}"
            ""_format(num_primes, slist, dlist, block_name, suffix, thread_args);
        auto statement = "euler_thread({});"_format(args);
        printer->add_line(statement);
    } else {
        if (info.solve_node != nullptr) {
            auto block = info.solve_node->get_statement_block();
            print_statement_block(block.get(), false, false);
        }
    }

    if (info.currents.empty() && info.breakpoint_node != nullptr) {
        auto block = info.breakpoint_node->get_statement_block();
        print_statement_block(block.get(), false, false);
    }

    auto write_statements = ion_write_statements(BlockType::State);
    for (auto& statement : write_statements) {
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
    printer->end_block();
    printer->add_newline();
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
    printer->start_block("static inline double nrn_current({})"_format(args));
    printer->add_line("double current = 0.0;");
    print_statement_block(block, false, false);
    for (auto& current : info.currents) {
        auto name = get_variable_name(current);
        printer->add_line("current += {};"_format(name));
    }
    printer->add_line("return current;");
    printer->end_block();
    printer->add_newline();
}


void CodegenCVisitor::print_nrn_cur_conductance_kernel(BreakpointBlock* node) {
    auto block = node->get_statement_block();
    print_statement_block(block.get(), false, false);
    if (!info.currents.empty()) {
        std::string sum;
        for (const auto& current : info.currents) {
            sum += breakpoint_current(current);
            if (&current != &info.currents.back()) {
                sum += "+";
            }
        }
        printer->add_line("double rhs = {};"_format(sum));
    }
    if (!info.conductances.empty()) {
        std::string sum;
        for (const auto& conductance : info.conductances) {
            sum += breakpoint_current(conductance.variable);
            if (&conductance != &info.conductances.back()) {
                sum += "+";
            }
        }
        printer->add_line("double g = {};"_format(sum));
    }
    for (const auto& conductance : info.conductances) {
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
    for (auto& ion : info.ions) {
        for (auto& var : ion.writes) {
            if (ion.is_ionic_current(var)) {
                auto name = get_variable_name(var);
                printer->add_line("double di{} = {};"_format(ion.name, name));
            }
        }
    }
    printer->add_line("double rhs = nrn_current({});"_format(internal_method_arguments()));
    printer->add_line("g = (g-rhs)/0.001;");
    for (auto& ion : info.ions) {
        for (auto& var : ion.writes) {
            if (ion.is_ionic_current(var)) {
                auto lhs = "ion_di" + ion.name + "dv";
                auto rhs = "(di{}-{})/0.001"_format(ion.name, get_variable_name(var));
                if (info.point_process) {
                    auto area = get_variable_name(node_area);
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
        printer->add_line("IonCurVar ionvar = {0};");
    }

    auto read_statements = ion_read_statements(BlockType::Equation);
    for (auto& statement : read_statements) {
        printer->add_line(statement);
    }

    if (info.conductances.empty()) {
        print_nrn_cur_non_conductance_kernel();
    } else {
        print_nrn_cur_conductance_kernel(node);
    }

    auto write_statements = ion_write_statements(BlockType::Equation);
    for (auto& statement : write_statements) {
        auto text = process_shadow_update_statement(statement, BlockType::Equation);
        printer->add_line(text);
    }

    if (info.point_process) {
        auto area = get_variable_name(node_area);
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
    print_channel_iteration_block_begin();
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
    printer->end_block();
    printer->add_newline();
    codegen = false;
}


/****************************************************************************************/
/*                            Main code printing entry points                            */
/****************************************************************************************/


void CodegenCVisitor::codegen_includes() {
    print_standard_includes();
    print_backend_includes();
    print_coreneuron_includes();
}


void CodegenCVisitor::codegen_namespace_begin() {
    print_namespace_start();
    print_backend_namespace_start();
}


void CodegenCVisitor::codegen_namespace_end() {
    print_backend_namespace_end();
    print_namespace_end();
}


void CodegenCVisitor::codegen_common_getters() {
    print_first_pointer_var_index_getter();
    print_memory_layout_getter();
    print_net_receive_arg_size_getter();
    print_thread_getters();
    print_num_variable_getter();
    print_mech_type_getter();
    print_memb_list_getter();
}


void CodegenCVisitor::codegen_data_structures() {
    print_mechanism_global_structure();
    print_mechanism_var_structure();
    print_ion_variables_structure();
}


void CodegenCVisitor::codegen_compute_functions() {
    print_top_verbatim_blocks();
    print_function_prototypes();

    for (const auto& procedure : info.procedures) {
        print_procedure(procedure);
    }

    for (const auto& function : info.functions) {
        print_function(function);
    }

    print_net_init_kernel();
    print_net_send_buffer_kernel();
    print_net_receive();
    print_net_receive_buffer_kernel();
    print_nrn_init();
    print_nrn_cur();
    print_nrn_state();
}


void CodegenCVisitor::codegen_all() {
    codegen = true;
    print_backend_info();
    codegen_includes();
    codegen_namespace_begin();

    print_mechanism_info_array();
    print_global_variables_for_hoc();

    codegen_data_structures();

    codegen_common_getters();

    print_thread_memory_callbacks();
    print_memory_allocation_routine();

    print_global_variable_setup();
    print_instance_variable_setup();
    print_nrn_alloc();

    codegen_compute_functions();
    print_mechanism_register();

    codegen_namespace_end();
}


void CodegenCVisitor::visit_program(Program* node) {
    CodegenBaseVisitor::visit_program(node);
    rename_function_arguments();
    codegen_all();
}
