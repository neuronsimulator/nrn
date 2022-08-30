/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "codegen/codegen_ispc_visitor.hpp"

#include <cmath>

#include "ast/all.hpp"
#include "codegen/codegen_naming.hpp"
#include "codegen/codegen_utils.hpp"
#include "symtab/symbol_table.hpp"
#include "utils/logger.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace codegen {

using symtab::syminfo::Status;

using visitor::RenameVisitor;

const std::vector<ast::AstNodeType> CodegenIspcVisitor::incompatible_node_types = {
    ast::AstNodeType::VERBATIM,
    ast::AstNodeType::EIGEN_NEWTON_SOLVER_BLOCK,
    ast::AstNodeType::EIGEN_LINEAR_SOLVER_BLOCK,
    ast::AstNodeType::WATCH_STATEMENT,
    ast::AstNodeType::TABLE_STATEMENT};

const std::unordered_set<std::string> CodegenIspcVisitor::incompatible_var_names = {
    "cdo",           "cfor",           "cif",    "cwhile",   "foreach",      "foreach_active",
    "foreach_tiled", "foreach_unique", "in",     "noinline", "__vectorcall", "int8",
    "int16",         "int32",          "int64",  "launch",   "print",        "soa",
    "sync",          "task",           "varying"};

/****************************************************************************************/
/*                            Overloaded visitor methods                                */
/****************************************************************************************/

/*
 * Rename math functions for ISPC backend
 */
void CodegenIspcVisitor::visit_function_call(const ast::FunctionCall& node) {
    if (!codegen) {
        return;
    }
    if (node.get_node_name() == "printf") {
        logger->warn(fmt::format("Not emitted in ispc: {}", to_nmodl(node)));
        return;
    }
    auto& fname = *node.get_name();
    RenameVisitor("fabs", "abs").visit_name(fname);
    RenameVisitor("exp", "vexp").visit_name(fname);
    CodegenCVisitor::visit_function_call(node);
}


/*
 * Rename special global variables
 */
void CodegenIspcVisitor::visit_var_name(const ast::VarName& node) {
    if (!codegen) {
        return;
    }
    RenameVisitor pi_rename("PI", "ISPC_PI");
    node.accept(pi_rename);
    CodegenCVisitor::visit_var_name(node);
}

void CodegenIspcVisitor::visit_local_list_statement(const ast::LocalListStatement& node) {
    if (!codegen) {
        return;
    }
    /// Correct indentation
    printer->add_newline();
    printer->add_indent();

    /// Name of the variable _dt given by
    /// nmodl::visitor::SteadystateVisitor::create_steadystate_block()
    const std::string steadystate_dt_variable_name = "dt_saved_value";
    auto type = CodegenCVisitor::local_var_type() + " ";
    printer->add_text(type);
    auto local_variables = node.get_variables();
    bool dt_saved_value_exists = false;

    /// Remove dt_saved_value from local_variables if it exists
    local_variables.erase(std::remove_if(local_variables.begin(),
                                         local_variables.end(),
                                         [&](const std::shared_ptr<ast::LocalVar>& local_variable) {
                                             if (local_variable->get_node_name() ==
                                                 steadystate_dt_variable_name) {
                                                 dt_saved_value_exists = true;
                                                 return true;
                                             } else {
                                                 return false;
                                             }
                                         }),
                          local_variables.end());

    /// Print the local_variables like normally
    CodegenCVisitor::print_vector_elements(local_variables, ", ");

    /// Print dt_saved_value as uniform
    if (dt_saved_value_exists) {
        printer->add_text(";");
        /// Correct indentation
        printer->add_newline();
        printer->add_indent();

        type = CodegenCVisitor::local_var_type() + " uniform ";
        printer->add_text(type + steadystate_dt_variable_name);
    }
}

/****************************************************************************************/
/*                      Routines must be overloaded in backend                          */
/****************************************************************************************/

/**
 * In ISPC we have to explicitly append `d` to a floating point number
 * otherwise it is treated as float. A value stored in the AST can be in
 * scientific notation and hence we can't just append `d` to the string.
 * Hence, we have to transform the value into the ISPC compliant format by
 * replacing `e` and `E` with `d` to keep the same representation of the
 * number as in the cpp backend.
 */
std::string CodegenIspcVisitor::format_double_string(const std::string& s_value) {
    return utils::format_double_string<CodegenIspcVisitor>(s_value);
}


/**
 * For float variables we don't have to do the conversion with changing `e` and
 * `E` with `f`, since the scientific notation numbers are already parsed as
 * floats by ISPC. Instead we need to take care of only appending `f` to the
 * end of floating point numbers, which is optional on ISPC.
 */
std::string CodegenIspcVisitor::format_float_string(const std::string& s_value) {
    return utils::format_float_string<CodegenIspcVisitor>(s_value);
}


std::string CodegenIspcVisitor::compute_method_name(BlockType type) const {
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
    throw std::runtime_error("compute_method_name not implemented");
}


std::string CodegenIspcVisitor::net_receive_buffering_declaration() {
    auto params = ParamVector();
    params.emplace_back(param_type_qualifier(),
                        fmt::format("{}*", instance_struct()),
                        param_ptr_qualifier(),
                        "inst");
    params.emplace_back(param_type_qualifier(), "NrnThread*", param_ptr_qualifier(), "nt");
    params.emplace_back(param_type_qualifier(), "Memb_list*", param_ptr_qualifier(), "ml");

    return fmt::format("export void {}({})",
                       method_name("ispc_net_buf_receive"),
                       get_parameter_str(params));
}


void CodegenIspcVisitor::print_backend_includes() {
    printer->add_line("#include \"nmodl/fast_math.ispc\"");
    printer->add_line("#include \"coreneuron/mechanism/nrnoc_ml.ispc\"");
    printer->add_newline();
    printer->add_newline();
}


std::string CodegenIspcVisitor::backend_name() const {
    return "ispc (api-compatibility)";
}


void CodegenIspcVisitor::print_channel_iteration_tiling_block_begin(BlockType /* type */) {
    // no tiling for ispc backend but make sure variables are declared as uniform
    printer->add_line("int uniform start = 0;");
    printer->add_line("int uniform end = nodecount;");
}


/*
 * Depending on the backend, print condition/loop for iterating over channels
 *
 * Use ispc foreach loop
 */
void CodegenIspcVisitor::print_channel_iteration_block_begin(BlockType /* type */) {
    printer->start_block("foreach (id = start ... end)");
}


void CodegenIspcVisitor::print_channel_iteration_block_end() {
    printer->end_block();
    printer->add_newline();
}


void CodegenIspcVisitor::print_net_receive_loop_begin() {
    printer->add_line("uniform int count = nrb->_displ_cnt;");
    printer->start_block("foreach (i = 0 ... count)");
}


void CodegenIspcVisitor::print_get_memb_list() {
    // do nothing
}


void CodegenIspcVisitor::print_atomic_op(const std::string& lhs,
                                         const std::string& op,
                                         const std::string& rhs) {
    std::string function;
    if (op == "+") {
        function = "atomic_add_local";
    } else if (op == "-") {
        function = "atomic_subtract_local";
    } else {
        throw std::runtime_error(fmt::format("ISPC backend error : {} not supported", op));
    }
    printer->fmt_line("{}(&{}, {});", function, lhs, rhs);
}


void CodegenIspcVisitor::print_nrn_cur_matrix_shadow_reduction() {
    auto rhs_op = operator_for_rhs();
    auto d_op = operator_for_d();
    if (info.point_process) {
        printer->add_line("uniform int node_id = node_index[id];");
        printer->fmt_line("vec_rhs[node_id] {} shadow_rhs[id];", rhs_op);
        printer->fmt_line("vec_d[node_id] {} shadow_d[id];", d_op);
    }
}


void CodegenIspcVisitor::print_shadow_reduction_block_begin() {
    printer->start_block("for (uniform int id = start; id < end; id++) ");
    if (info.point_process) {
        printer->start_block("if (programIndex == 0)");
    }
}

void CodegenIspcVisitor::print_shadow_reduction_block_end() {
    if (info.point_process) {
        printer->end_block(1);
    }
    printer->end_block(1);
}

void CodegenIspcVisitor::print_rhs_d_shadow_variables() {
    if (info.point_process) {
        printer->fmt_line("double* uniform shadow_rhs = nt->{};", naming::NTHREAD_RHS_SHADOW);
        printer->fmt_line("double* uniform shadow_d = nt->{};", naming::NTHREAD_D_SHADOW);
    }
}


bool CodegenIspcVisitor::nrn_cur_reduction_loop_required() {
    return info.point_process;
}


std::string CodegenIspcVisitor::ptr_type_qualifier() {
    if (wrapper_codegen) {
        return CodegenCVisitor::ptr_type_qualifier();
    } else {
        return "uniform ";  // @note: extra space needed to separate qualifier from var name.
    }
}

std::string CodegenIspcVisitor::global_var_struct_type_qualifier() {
    if (wrapper_codegen) {
        return CodegenCVisitor::global_var_struct_type_qualifier();
    } else {
        return "uniform ";  // @note: extra space needed to separate qualifier from var name.
    }
}

void CodegenIspcVisitor::print_global_var_struct_decl() {
    if (wrapper_codegen) {
        CodegenCVisitor::print_global_var_struct_decl();
    }
}

void CodegenIspcVisitor::print_global_var_struct_assertions() const {
    // Print static_assert in .cpp but not .ispc
    if (wrapper_codegen) {
        CodegenCVisitor::print_global_var_struct_assertions();
    }
}

std::string CodegenIspcVisitor::param_type_qualifier() {
    if (wrapper_codegen) {
        return CodegenCVisitor::param_type_qualifier();
    } else {
        return "uniform ";
    }
}


std::string CodegenIspcVisitor::param_ptr_qualifier() {
    if (wrapper_codegen) {
        return CodegenCVisitor::param_ptr_qualifier();
    } else {
        return "uniform ";
    }
}


void CodegenIspcVisitor::print_backend_namespace_start() {
    // no ispc namespace
}


void CodegenIspcVisitor::print_backend_namespace_stop() {
    // no ispc namespace
}


CodegenIspcVisitor::ParamVector CodegenIspcVisitor::get_global_function_parms(
    const std::string& /* arg_qualifier */) {
    auto params = ParamVector();
    params.emplace_back(param_type_qualifier(),
                        fmt::format("{}*", instance_struct()),
                        param_ptr_qualifier(),
                        "inst");
    params.emplace_back(param_type_qualifier(), "NrnThread*", param_ptr_qualifier(), "nt");
    params.emplace_back(param_type_qualifier(), "Memb_list*", param_ptr_qualifier(), "ml");
    params.emplace_back(param_type_qualifier(), "int", "", "type");
    return params;
}


void CodegenIspcVisitor::print_procedure(const ast::ProcedureBlock& node) {
    codegen = true;
    const auto& name = node.get_node_name();
    print_function_or_procedure(node, name);
    codegen = false;
}


void CodegenIspcVisitor::print_global_function_common_code(BlockType type,
                                                           const std::string& function_name) {
    // If we are printing the cpp file, we have to use the c version of this function
    if (wrapper_codegen) {
        return CodegenCVisitor::print_global_function_common_code(type, function_name);
    }

    std::string method = compute_method_name(type);

    auto params = get_global_function_parms(ptr_type_qualifier());
    print_global_method_annotation();
    printer->fmt_start_block("export void {}({})", method, get_parameter_str(params));

    print_kernel_data_present_annotation_block_begin();
    printer->add_line("uniform int nodecount = ml->nodecount;");
    printer->add_line("uniform int pnodecount = ml->_nodecount_padded;");
    printer->fmt_line("const int* {}node_index = ml->nodeindices;", ptr_type_qualifier());
    printer->fmt_line("double* {}data = ml->data;", ptr_type_qualifier());
    printer->fmt_line("const double* {}voltage = nt->_actual_v;", ptr_type_qualifier());

    if (type == BlockType::Equation) {
        printer->fmt_line("double* {}vec_rhs = nt->_actual_rhs;", ptr_type_qualifier());
        printer->fmt_line("double* {}vec_d = nt->_actual_d;", ptr_type_qualifier());
        print_rhs_d_shadow_variables();
    }
    printer->fmt_line("Datum* {}indexes = ml->pdata;", ptr_type_qualifier());
    printer->fmt_line("ThreadDatum* {}thread = ml->_thread;", ptr_type_qualifier());
    printer->add_newline(1);
}


void CodegenIspcVisitor::print_compute_functions() {
    for (const auto& function: info.functions) {
        if (!program_symtab->lookup(function->get_node_name())->has_all_status(Status::inlined)) {
            print_function(*function);
        }
    }
    for (const auto& procedure: info.procedures) {
        if (!program_symtab->lookup(procedure->get_node_name())->has_all_status(Status::inlined)) {
            print_procedure(*procedure);
        }
    }
    if (!emit_fallback[BlockType::NetReceive]) {
        print_net_receive_kernel();
        print_net_receive_buffering(false);
    }
    if (!emit_fallback[BlockType::Initial]) {
        print_nrn_init(false);
    }
    if (!emit_fallback[BlockType::Equation]) {
        print_nrn_cur();
    }
    if (!emit_fallback[BlockType::State]) {
        print_nrn_state();
    }
}


void CodegenIspcVisitor::print_ion_var_constructor(const std::vector<std::string>& members) {
    /// no constructor for ispc
}


void CodegenIspcVisitor::print_ion_variable() {
    /// c syntax to zero initialize struct
    printer->add_line("IonCurVar ionvar = {0};");
}


/****************************************************************************************/
/*                    Main code printing entry points and wrappers                      */
/****************************************************************************************/

void CodegenIspcVisitor::print_net_receive_buffering_wrapper() {
    if (!net_receive_required() || info.artificial_cell) {
        return;
    }
    printer->add_newline(2);
    printer->fmt_start_block("void {}(NrnThread* nt)", method_name("net_buf_receive"));
    printer->add_line("Memb_list* ml = get_memb_list(nt);");
    printer->start_block("if (ml == NULL)");
    printer->add_line("return;");
    printer->end_block(1);
    printer->fmt_line("auto* const {1}inst = static_cast<{0}*>(ml->instance);",
                      instance_struct(),
                      ptr_type_qualifier());

    printer->fmt_line("{}(inst, nt, ml);", method_name("ispc_net_buf_receive"));

    printer->end_block(1);
}


void CodegenIspcVisitor::print_headers_include() {
    print_backend_includes();
}

void CodegenIspcVisitor::print_nmodl_constants() {
    if (!info.factor_definitions.empty()) {
        printer->add_newline(2);
        printer->add_line("/** constants used in nmodl */");
        for (auto& it: info.factor_definitions) {
            const std::string name = it->get_node_name() == "PI" ? "ISPC_PI" : it->get_node_name();
            const std::string value = format_double_string(it->get_value()->get_value());
            printer->fmt_line("static const uniform double {} = {};", name, value);
        }
    }
}

void CodegenIspcVisitor::print_wrapper_headers_include() {
    print_standard_includes();
    print_coreneuron_includes();
}


void CodegenIspcVisitor::print_wrapper_routine(const std::string& wrapper_function,
                                               BlockType type) {
    static const auto args = "NrnThread* nt, Memb_list* ml, int type";
    const auto function_name = method_name(wrapper_function);
    auto compute_function = compute_method_name(type);

    printer->add_newline(2);
    printer->fmt_start_block("void {}({})", function_name, args);
    printer->add_line("int nodecount = ml->nodecount;");
    printer->fmt_line("auto* const {1}inst = static_cast<{0}*>(ml->instance);",
                      instance_struct(),
                      ptr_type_qualifier());

    if (type == BlockType::Initial) {
        printer->add_newline();
        printer->add_line("setup_instance(nt, ml);");
        printer->add_newline();
        printer->start_block("if (_nrn_skip_initmodel)");
        printer->add_line("return;");
        printer->end_block(1);
    }
    printer->fmt_line("{}(inst, nt, ml, type);", compute_function);
    printer->end_block(1);
}


void CodegenIspcVisitor::print_backend_compute_routine_decl() {
    auto params = get_global_function_parms("");
    auto compute_function = compute_method_name(BlockType::Initial);
    if (!emit_fallback[BlockType::Initial]) {
        printer->fmt_line("extern \"C\" void {}({});", compute_function, get_parameter_str(params));
    }

    if (nrn_cur_required() && !emit_fallback[BlockType::Equation]) {
        compute_function = compute_method_name(BlockType::Equation);
        printer->fmt_line("extern \"C\" void {}({});", compute_function, get_parameter_str(params));
    }

    if (nrn_state_required() && !emit_fallback[BlockType::State]) {
        compute_function = compute_method_name(BlockType::State);
        printer->fmt_line("extern \"C\" void {}({});", compute_function, get_parameter_str(params));
    }

    if (net_receive_required()) {
        auto net_recv_params = ParamVector();
        net_recv_params.emplace_back("", fmt::format("{}*", instance_struct()), "", "inst");
        net_recv_params.emplace_back("", "NrnThread*", "", "nt");
        net_recv_params.emplace_back("", "Memb_list*", "", "ml");
        printer->fmt_line("extern \"C\" void {}({});",
                          method_name("ispc_net_buf_receive"),
                          get_parameter_str(net_recv_params));
    }
}

bool CodegenIspcVisitor::check_incompatibilities() {
    const auto& has_incompatible_nodes = [](const ast::Ast& node) {
        return !collect_nodes(node, incompatible_node_types).empty();
    };

    const auto get_name_from_symbol_type_vector = [](const SymbolType& var) -> const std::string& {
        return var->get_name();
    };

    // instance vars
    if (check_incompatible_var_name<SymbolType>(codegen_float_variables,
                                                get_name_from_symbol_type_vector)) {
        return true;
    }

    if (check_incompatible_var_name<SymbolType>(codegen_shadow_variables,
                                                get_name_from_symbol_type_vector)) {
        return true;
    }
    if (check_incompatible_var_name<IndexVariableInfo>(
            codegen_int_variables, [](const IndexVariableInfo& var) -> const std::string& {
                return var.symbol->get_name();
            })) {
        return true;
    }


    if (check_incompatible_var_name<std::string>(info.currents,
                                                 [](const std::string& var) -> const std::string& {
                                                     return var;
                                                 })) {
        return true;
    }


    // global vars
    // info.top_local_variables is not checked because it should be addressed by the
    // renameIspcVisitor
    if (check_incompatible_var_name<SymbolType>(info.global_variables,
                                                get_name_from_symbol_type_vector)) {
        return true;
    }


    if (check_incompatible_var_name<SymbolType>(info.constant_variables,
                                                get_name_from_symbol_type_vector)) {
        return true;
    }

    // ion vars
    for (const auto& ion: info.ions) {
        if (check_incompatible_var_name<std::string>(
                ion.writes, [](const std::string& var) -> const std::string& { return var; })) {
            return true;
        }
    }


    emit_fallback = std::vector<bool>(BlockType::BlockTypeEnd, false);

    if (info.initial_node) {
        emit_fallback[BlockType::Initial] =
            emit_fallback[BlockType::Initial] || has_incompatible_nodes(*info.initial_node) ||
            visitor::calls_function(*info.initial_node, "net_send") || info.require_wrote_conc ||
            info.net_send_used || info.net_event_used;
    } else {
        emit_fallback[BlockType::Initial] = emit_fallback[BlockType::Initial] ||
                                            info.net_receive_initial_node ||
                                            info.require_wrote_conc;
    }

    emit_fallback[BlockType::NetReceive] =
        emit_fallback[BlockType::NetReceive] ||
        (info.net_receive_node && (has_incompatible_nodes(*info.net_receive_node) ||
                                   visitor::calls_function(*info.net_receive_node, "net_send")));

    emit_fallback[BlockType::Equation] = emit_fallback[BlockType::Equation] ||
                                         (nrn_cur_required() && info.breakpoint_node &&
                                          has_incompatible_nodes(*info.breakpoint_node));

    emit_fallback[BlockType::State] = emit_fallback[BlockType::State] ||
                                      (nrn_state_required() && info.nrn_state_block &&
                                       has_incompatible_nodes(*info.nrn_state_block));


    return false;
}


void CodegenIspcVisitor::move_procs_to_wrapper() {
    auto nameset = std::set<std::string>();

    auto populate_nameset = [&nameset](const ast::Block* block) {
        if (block) {
            const auto& names = collect_nodes(*block, {ast::AstNodeType::NAME});
            for (const auto& name: names) {
                nameset.insert(name->get_node_name());
            }
        }
    };
    populate_nameset(info.initial_node);
    populate_nameset(info.nrn_state_block);
    populate_nameset(info.breakpoint_node);

    const auto& has_incompatible_nodes = [](const ast::Ast& node) {
        return !collect_nodes(node, incompatible_node_types).empty();
    };

    auto target_procedures = std::vector<const ast::ProcedureBlock*>();
    for (const auto& procedure: info.procedures) {
        const auto& name = procedure->get_name()->get_node_name();
        if (nameset.find(name) == nameset.end() || has_incompatible_nodes(*procedure)) {
            wrapper_procedures.push_back(procedure);
        } else {
            target_procedures.push_back(procedure);
        }
    }
    info.procedures = target_procedures;
    auto target_functions = std::vector<const ast::FunctionBlock*>();
    for (const auto& function: info.functions) {
        const auto& name = function->get_name()->get_node_name();
        if (nameset.find(name) == nameset.end() || has_incompatible_nodes(*function)) {
            wrapper_functions.push_back(function);
        } else {
            target_functions.push_back(function);
        }
    }
    info.functions = target_functions;
}

void CodegenIspcVisitor::print_block_wrappers_initial_equation_state() {
    if (emit_fallback[BlockType::Initial]) {
        logger->warn("Falling back to C backend for emitting Initial block");
        fallback_codegen.print_nrn_init();
    } else {
        print_wrapper_routine(naming::NRN_INIT_METHOD, BlockType::Initial);
    }

    if (nrn_cur_required()) {
        if (emit_fallback[BlockType::Equation]) {
            logger->warn("Falling back to C backend for emitting breakpoint block");
            fallback_codegen.print_nrn_cur();
        } else {
            print_wrapper_routine(naming::NRN_CUR_METHOD, BlockType::Equation);
        }
    }

    if (nrn_state_required()) {
        if (emit_fallback[BlockType::State]) {
            logger->warn("Falling back to C backend for emitting state block");
            fallback_codegen.print_nrn_state();
        } else {
            print_wrapper_routine(naming::NRN_STATE_METHOD, BlockType::State);
        }
    }
}


void CodegenIspcVisitor::visit_program(const ast::Program& node) {
    setup(node);

    // we need setup to check incompatibilities
    if (check_incompatibilities()) {
        logger->warn(
            "ISPC reserved keyword used as variable name in mod file. Using C++ backend as "
            "fallback");
        print_backend_info();
        print_headers_include();
        fallback_codegen.visit_program(node);
    } else {
        fallback_codegen.setup(node);
        // we do not want to call setup twice
        print_codegen_routines();
        print_wrapper_routines();
    }
}


void CodegenIspcVisitor::print_codegen_routines() {
    codegen = true;
    move_procs_to_wrapper();
    print_backend_info();
    print_headers_include();
    print_nmodl_constants();
    print_data_structures(false);
    print_compute_functions();
}


void CodegenIspcVisitor::print_wrapper_routines() {
    printer = wrapper_printer;
    wrapper_codegen = true;
    print_backend_info();
    print_wrapper_headers_include();
    print_namespace_begin();

    CodegenCVisitor::print_nmodl_constants();
    print_mechanism_info();
    print_data_structures(true);
    print_global_variables_for_hoc();
    print_common_getters();

    print_memory_allocation_routine();
    print_thread_memory_callbacks();
    print_abort_routine();
    /* this is a godawful mess.. the global variables have to be copied over into the fallback
     * such that they are available to the fallback generator.
     */
    fallback_codegen.set_codegen_global_variables(codegen_global_variables);
    print_instance_variable_setup();
    print_nrn_alloc();
    print_top_verbatim_blocks();


    for (const auto& function: wrapper_functions) {
        if (!program_symtab->lookup(function->get_node_name())->has_all_status(Status::inlined)) {
            fallback_codegen.print_function(*function);
        }
    }
    for (const auto& procedure: wrapper_procedures) {
        if (!program_symtab->lookup(procedure->get_node_name())->has_all_status(Status::inlined)) {
            fallback_codegen.print_procedure(*procedure);
        }
    }

    print_check_table_thread_function();

    print_net_send_buffering();
    print_net_init();
    print_watch_activate();
    fallback_codegen.print_watch_check();  // requires C style variable declarations and loops

    if (emit_fallback[BlockType::NetReceive]) {
        logger->warn(
            "Found VERBATIM code or ISPC keyword in NET_RECEIVE block, using C++ backend as "
            "fallback"
            "backend");
        fallback_codegen.print_net_receive_kernel();
        fallback_codegen.print_net_receive_buffering();
    }

    print_net_receive();

    print_backend_compute_routine_decl();

    if (!emit_fallback[BlockType::NetReceive]) {
        print_net_receive_buffering_wrapper();
    }

    print_block_wrappers_initial_equation_state();

    print_nrn_constructor();
    print_nrn_destructor();

    print_mechanism_register();

    print_namespace_end();
}

}  // namespace codegen
}  // namespace nmodl
