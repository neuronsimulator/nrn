#include "codegen/codegen_neuron_acc_visitor.hpp"

#include "ast/all.hpp"
#include "ast/block.hpp"
#include "ast/function_call.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace codegen {

std::string CodegenNeuronAccVisitor::backend_name() const {
    return "C++-OpenAcc-NEURON";
}

void CodegenNeuronAccVisitor::print_standard_includes() {
    CodegenNeuronCppVisitor::print_standard_includes();
    printer->add_line("#include <neuron/gpu/offload.hpp>");
    printer->add_line("#include <neuron/gpu/net_send_buffer.hpp>");
    printer->add_line("#include <neuron/gpu/mechanism_phases.hpp>");
    printer->add_line("#include <neuron/gpu/sync.hpp>");
}

bool CodegenNeuronAccVisitor::host_only_parallel_block(BlockType type) const {
    return type == BlockType::Initial && info.require_wrote_conc;
}

void CodegenNeuronAccVisitor::print_global_var_struct_decl() {
    CodegenNeuronCppVisitor::print_global_var_struct_decl();
    if (!info.artificial_cell && !codegen_global_variables.empty()) {
        printer->fmt_line("static bool {}_gpu_resident = false;", global_struct_instance());
    }
}

void CodegenNeuronAccVisitor::print_global_variable_enter_data_once() const {
    if (info.artificial_cell || codegen_global_variables.empty()) {
        return;
    }
    auto const& global = global_struct_instance();
    printer->fmt_push_block("if (nt->compute_gpu && !{}_gpu_resident)", global);
    printer->fmt_line("(void) nrn_target_copyin(&{}, 1);", global);
    printer->add_line(global + "_gpu_resident = true;");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_global_variable_device_update_annotation() const {
    if (info.artificial_cell || codegen_global_variables.empty()) {
        return;
    }
    printer->push_block("if (nt->compute_gpu)");
    printer->fmt_line("nrn_pragma_acc(update device ({}))", global_struct_instance());
    printer->fmt_line("nrn_pragma_omp(target update to({}))", global_struct_instance());
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_kernel_global_device_setup() {
    print_global_variable_enter_data_once();
}

void CodegenNeuronAccVisitor::print_kernel_instance_data_copyin() {
    // Mechanism SOA is uploaded once; kernels use present(_ml_arg, ...) only.
}

std::string CodegenNeuronAccVisitor::global_variable_name(const SymbolType& symbol,
                                                          bool use_instance) const {
    if (use_present_fp_indexing_ || use_instance) {
        return fmt::format("inst.{}->{}", naming::INST_GLOBAL_MEMBER, symbol->get_name());
    }
    return CodegenNeuronCppVisitor::global_variable_name(symbol, false);
}

void CodegenNeuronAccVisitor::print_present_fp_pointer_declarations() const {
    const auto codegen_float_variables_size = codegen_float_variables.size();
    for (int i = 0; i < codegen_float_variables_size; ++i) {
        printer->fmt_line("double* _present_fp_{0} = _lmc.template fpfield_ptr<{0}>();", i);
    }
}

void CodegenNeuronAccVisitor::print_present_dptr_pointer_declarations() const {
    const auto codegen_int_variables_size = codegen_int_variables.size();
    for (size_t i = 0; i < codegen_int_variables_size; ++i) {
        const auto& var = codegen_int_variables[i];
        auto const sem = info.semantics[i].name;
        if (var.is_index || var.is_integer || var.is_vdata || sem == naming::POINTER_SEMANTIC) {
            continue;
        }
        printer->fmt_line(
            "double* const* _present_dptr_{0} = (nt->compute_gpu && "
            "neuron::mechanism::_get::gpu_pdata_ptr_cache(_sorted_token, _ml_arg->type())) "
            "? neuron::mechanism::_get::gpu_pdata_ptr_cache(_sorted_token, _ml_arg->type())[{0}] "
            ": _lmc.template dptr_field_ptr<{0}>();",
            i);
    }
}

std::string CodegenNeuronAccVisitor::present_dptr_deviceptr_clause() const {
    std::vector<std::string> dptr_names;
    const auto codegen_int_variables_size = codegen_int_variables.size();
    for (size_t i = 0; i < codegen_int_variables_size; ++i) {
        const auto& var = codegen_int_variables[i];
        auto const sem = info.semantics[i].name;
        if (var.is_index || var.is_integer || var.is_vdata || sem == naming::POINTER_SEMANTIC) {
            continue;
        }
        dptr_names.push_back(fmt::format("_present_dptr_{}", i));
    }
    if (dptr_names.empty()) {
        return {};
    }
    return fmt::format(" deviceptr({})", fmt::join(dptr_names, ", "));
}

CodegenNeuronAccVisitor::ParamVector CodegenNeuronAccVisitor::internal_method_parameters() {
    if (info.mod_suffix == "nothing") {
        return {};
    }

    ParamVector params;
    params.emplace_back("", fmt::format("{}&", instance_struct()), "", "inst");
    if (!info.artificial_cell) {
        params.emplace_back("", fmt::format("{}&", node_data_struct()), "", "node_data");
    }
    params.emplace_back("", "size_t", "", "id");
    params.emplace_back("", "Datum*", "", "_ppvar");
    params.emplace_back("", "Datum*", "", "_thread");
    if (!codegen_thread_variables.empty()) {
        params.emplace_back("", fmt::format("{}&", thread_variables_struct()), "", "_thread_vars");
    }
    params.emplace_back("", "NrnThread*", "", "nt");
    for (int i = 0; i < static_cast<int>(codegen_float_variables.size()); ++i) {
        params.emplace_back("", "double*", "", fmt::format("_present_fp_{}", i));
    }
    return params;
}

void CodegenNeuronAccVisitor::print_function_definitions() {
    print_hoc_py_wrapper_function_definitions();
    use_present_fp_indexing_ = true;
    for (const auto& procedure: info.procedures) {
        print_procedure(*procedure);
    }
    for (const auto& function: info.functions) {
        print_function(*function);
    }
    for (const auto& function_table: info.function_tables) {
        print_function_tables(*function_table);
    }
    use_present_fp_indexing_ = false;
}

void CodegenNeuronAccVisitor::print_hoc_py_wrapper_before_table_update() {
    if (info.mod_suffix != "nothing" && !info.artificial_cell) {
        print_present_fp_pointer_declarations();
    }
}

void CodegenNeuronAccVisitor::print_parallel_iteration_hint(BlockType type,
                                                            const ast::Block* block) {
    if (info.artificial_cell) {
        return;
    }

    if (host_only_parallel_block(type)) {
        CodegenNeuronCppVisitor::print_parallel_iteration_hint(type, block);
        return;
    }

    // Reuse CPU ivdep path when block uses mutex/protect (atomics + SIMD conflict).
    std::vector<std::shared_ptr<const ast::Ast>> nodes;
    if (block) {
        nodes = collect_nodes(*block,
                              {ast::AstNodeType::PROTECT_STATEMENT,
                               ast::AstNodeType::MUTEX_LOCK,
                               ast::AstNodeType::MUTEX_UNLOCK});
    }
    if (!nodes.empty()) {
        CodegenNeuronCppVisitor::print_parallel_iteration_hint(type, block);
        return;
    }

    print_global_variable_device_update_annotation();

    if (type != BlockType::NetReceive && !info.artificial_cell) {
        printer->add_line("auto const* nodeindices = node_data.nodeindices;");
        if (type == BlockType::Equation) {
            printer->add_line("double* vec_rhs = node_data.node_rhs;");
            printer->add_line("double* vec_d = node_data.node_diagonal;");
        }
        const auto codegen_float_variables_size = codegen_float_variables.size();
        for (int i = 0; i < codegen_float_variables_size; ++i) {
            printer->fmt_line("double* _present_fp_{0} = _lmc.template fpfield_ptr<{0}>();", i);
        }
        print_present_dptr_pointer_declarations();
    }

    std::ostringstream present_clause;
    present_clause << "present(_ml_arg, nt";
    if (type == BlockType::NetReceive) {
        present_clause << ", nrb";
    } else if (!info.artificial_cell) {
        present_clause << ", nodeindices, _thread, node_data.node_voltages[:nt->end]";
        if (type == BlockType::Equation) {
            present_clause << ", vec_rhs[:nt->end], vec_d[:nt->end]";
        }
        const auto codegen_float_variables_size = codegen_float_variables.size();
        for (int i = 0; i < codegen_float_variables_size; ++i) {
            const auto& float_var = codegen_float_variables[i];
            auto const array_len = float_var->is_array() ? float_var->get_length() : 1;
            present_clause << fmt::format(
                ", _present_fp_{}[:static_cast<std::size_t>(_ml_arg->nodecount) * {}]", i, array_len);
        }
        // pdata ion rows live in device memory via upload_mechanism_pointer_tables;
        // do not add them to present() (OpenACC cannot track deviceptr slices here).
    }
    present_clause << ')';

    printer->fmt_line("nrn_pragma_acc(parallel loop {}{} async(nt->stream_id) if(nt->compute_gpu))",
                      present_clause.str(),
                      present_dptr_deviceptr_clause());
    printer->add_line("nrn_pragma_omp(target teams distribute parallel for if(nt->compute_gpu))");
}

void CodegenNeuronAccVisitor::print_kernel_data_present_annotation_block_begin() {
    if (!info.artificial_cell) {
        if (codegen_global_variables.empty()) {
            printer->add_line("nrn_pragma_acc(data present(nt, _ml_arg) if(nt->compute_gpu))");
        } else {
            printer->fmt_line("nrn_pragma_acc(data present(nt, _ml_arg, {}) if(nt->compute_gpu))",
                              global_struct_instance());
        }
        printer->add_line("{");
        printer->increase_indent();
    }
}

void CodegenNeuronAccVisitor::print_kernel_data_present_annotation_block_end() {
    if (!info.artificial_cell) {
        print_device_stream_wait();
        printer->pop_block();
    }
}

void CodegenNeuronAccVisitor::print_device_stream_wait() const {
    printer->push_block("if(nt->compute_gpu)");
    printer->add_line("nrn_pragma_acc(wait(nt->stream_id))");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_net_send_buffering_cnt_update() const {
    printer->push_block("if (nt->compute_gpu)");
    printer->add_line("nrn_pragma_acc(atomic capture)");
    printer->add_line("nrn_pragma_omp(atomic capture)");
    printer->add_line("i = nsb->_cnt++;");
    printer->chain_block("else");
    printer->add_line("i = nsb->_cnt++;");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_net_send_buffering_grow() {
    printer->add_line("neuron::gpu::net_send_buffer_ensure(ml);");
    printer->push_block("if (!nt->compute_gpu)");
    printer->push_block("if (i >= nsb->_size)");
    printer->add_line("nsb->grow();");
    printer->pop_block();
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_net_send_buf_count_update_to_host() const {
    printer->add_line("nrn_pragma_acc(update self(nsb->_cnt))");
    printer->add_line("nrn_pragma_omp(target update from(nsb->_cnt))");
}

void CodegenNeuronAccVisitor::print_net_send_buf_update_to_host() const {
    print_device_stream_wait();
    printer->push_block("if (nsb && nt->compute_gpu)");
    print_net_send_buf_count_update_to_host();
    printer->add_line("neuron::gpu::update_net_send_buffer_on_host(nt, nsb);");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_net_send_buf_count_update_to_device() const {
    printer->push_block("if (nt->compute_gpu)");
    printer->add_line("nrn_pragma_acc(update device(nsb->_cnt))");
    printer->add_line("nrn_pragma_omp(target update to(nsb->_cnt))");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_net_send_buffering() {
    if (!net_send_buffer_required()) {
        return;
    }

    printer->add_newline(2);
    auto args =
        "NrnThread* nt, Memb_list* ml, neuron::gpu::NetSendBuffer_t* nsb, int type, "
        "intptr_t vdata_ptr, intptr_t weight_ptr, intptr_t point_ptr, double t, double flag";
    printer->fmt_push_block("static inline void net_send_buffering({})", args);
    printer->add_line("int i = 0;");
    print_net_send_buffering_grow();
    print_net_send_buffering_cnt_update();
    printer->push_block("if (i < nsb->_size)");
    printer->add_multi_line(R"CODE(
         nsb->_sendtype[i] = type;
         nsb->_vdata_index[i] = static_cast<int>(vdata_ptr);
         nsb->_weight_index[i] = static_cast<int>(weight_ptr);
         nsb->_pnt_index[i] = static_cast<int>(point_ptr);
         nsb->_nsb_t[i] = t;
         nsb->_nsb_flag[i] = flag;
    )CODE");
    printer->pop_block();
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_send_event_move() {
    printer->add_newline();
    printer->add_line("neuron::gpu::NetSendBuffer_t* nsb = _ml_arg->_net_send_buffer;");
    print_net_send_buf_update_to_host();
    printer->add_line("neuron::gpu::deliver_net_send_buffer_events(nt, nsb);");
    print_net_send_buf_count_update_to_device();
}

void CodegenNeuronAccVisitor::print_after_nrn_cur_gpu_net_send_flush() {
    if (info.net_send_used && !info.artificial_cell) {
        print_send_event_move();
    }
}

void CodegenNeuronAccVisitor::print_compute_functions() {
    print_net_send_buffering();
    CodegenNeuronCppVisitor::print_compute_functions();
}

void CodegenNeuronAccVisitor::print_nrn_jacob() {
    if (!breakpoint_exist() || info.artificial_cell) {
        CodegenNeuronCppVisitor::print_nrn_jacob();
        return;
    }

    printer->add_newline(2);

    ParamVector args = {{"", "const _nrn_model_sorted_token&", "", "_sorted_token"},
                        {"", "NrnThread*", "", "nt"},
                        {"", "Memb_list*", "", "_ml_arg"},
                        {"", "int", "", "_type"}};

    printer->fmt_push_block("static void {}({})",
                            method_name(naming::NRN_JACOB_METHOD),
                            get_parameter_str(args));
    // Gate on matrix residency (Gate A), not the global Gate-B aggregate: nrn_cur already
    // wrote g_unused on device; host jacob would read stale host SOA and skip vec_d updates.
    printer->push_block(
        "if (nt->compute_gpu && neuron::gpu::matrix_rhs_d_stays_on_device_for_solve(*nt))");
    print_kernel_global_device_setup();
    print_entrypoint_setup_code_from_memb_list();
    printer->fmt_line("auto nodecount = _ml_arg->nodecount;");
    use_present_fp_indexing_ = true;
    print_parallel_iteration_hint(BlockType::Equation, nullptr);
    printer->push_block("for (int id = 0; id < nodecount; id++)");
    // Recompute g on device (same kernel as nrn_cur). Use vec_d[] directly like nrn_cap_jacob
    // and axial lhs; nested data present(nt,_ml_arg) around this loop dropped vec_d updates.
    print_nrn_cur_kernel(*info.breakpoint_node);
    printer->fmt_line("vec_d[node_id] {} g;", operator_for_d());
    printer->fmt_line(
        "{} = g;",
        indexed_fp_var(info.vectorize ? naming::CONDUCTANCE_UNUSED_VARIABLE
                                      : naming::CONDUCTANCE_VARIABLE));
    printer->pop_block();
    use_present_fp_indexing_ = false;
    print_device_stream_wait();
    printer->chain_block("else");
    print_entrypoint_setup_code_from_memb_list();
    printer->fmt_line("auto nodecount = _ml_arg->nodecount;");
    printer->push_block("for (int id = 0; id < nodecount; id++)");
    printer->add_line("int node_id = node_data.nodeindices[id];");
    printer->fmt_line("node_data.node_diagonal[node_id] {} _lmc.template fpfield<{}>(id);",
                      operator_for_d(),
                      conductance_fp_index());
    printer->pop_block();
    printer->pop_block();
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_check_table_entrypoint() {
    if (info.table_count == 0) {
        return;
    }

    for (const auto& function: info.functions_with_table) {
        auto name = function->get_node_name();
        auto internal_params = internal_method_parameters();
        printer->fmt_line("void {}({});",
                          table_update_function_name(name),
                          get_parameter_str(internal_params));
    }

    ParamVector args = {{"", "Memb_list*", "", "_ml"},
                        {"", "size_t", "", "id"},
                        {"", "Datum*", "", "_ppvar"},
                        {"", "Datum*", "", "_thread"},
                        {"", "double*", "", "_globals"},
                        {"", "NrnThread*", "", "nt"},
                        {"", "int", "", "_type"},
                        {"", "const _nrn_model_sorted_token&", "", "_sorted_token"}};

    printer->fmt_line("static void {}({})", table_thread_function_name(), get_parameter_str(args));
    printer->push_block();
    printer->add_line("_nrn_mechanism_cache_range _lmc{_sorted_token, *nt, *_ml, _type};");
    print_present_fp_pointer_declarations();
    printer->fmt_line("auto inst = make_instance_{}(_ml->get_storage_offset() + id, &_lmc, false);",
                      info.mod_suffix);
    if (!info.artificial_cell) {
        printer->fmt_line("auto node_data = make_node_data_{}(*nt, *_ml);", info.mod_suffix);
    }
    if (!codegen_thread_variables.empty()) {
        printer->fmt_line("auto _thread_vars = {}(_thread[{}].get<double*>());",
                          thread_variables_struct(),
                          info.thread_var_thread_id);
    }

    for (const auto& function: info.functions_with_table) {
        auto method_name = function->get_node_name();
        auto method_args = get_arg_str(internal_method_parameters());
        printer->fmt_line("{}({});", table_update_function_name(method_name), method_args);
    }
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_gpu_phase_registration() {
    std::vector<std::string> flags;
    if (nrn_cur_required()) {
        flags.emplace_back("neuron::gpu::MechanismGpuPhase::Current");
    }
    if (breakpoint_exist()) {
        flags.emplace_back("neuron::gpu::MechanismGpuPhase::Jacobian");
    }
    if (nrn_state_required()) {
        flags.emplace_back("neuron::gpu::MechanismGpuPhase::Solve");
    }
    if (flags.empty()) {
        return;
    }
    std::string phase_flags;
    for (std::size_t i = 0; i < flags.size(); ++i) {
        if (i > 0) {
            phase_flags += " | ";
        }
        phase_flags += flags[i];
    }
    printer->fmt_line("neuron::gpu::register_mechanism_gpu_phases(mech_type, {});", phase_flags);
}

void CodegenNeuronAccVisitor::print_net_send_call(const ast::FunctionCall& node) {
    if (printing_net_receive || printing_net_init) {
        CodegenNeuronCppVisitor::print_net_send_call(node);
        return;
    }

    auto const& arguments = node.get_arguments();
    const auto& tqitem = get_variable_name("tqitem", /* use_instance */ false);
    std::string weight_index = "weight_index";
    std::string point_process = get_variable_name(naming::POINT_PROCESS_VARIABLE, false);

    if (!printing_net_receive && !printing_net_init) {
        weight_index = "0";
        if (info.artificial_cell) {
            point_process = fmt::format("(Point_process*){}", point_process);
        } else {
            point_process += ".get<Point_process*>()";
        }
    }

    if (info.artificial_cell) {
        printer->fmt_text("{}(/* tqitem */ &{}, {}, {}, {} + ",
                          "artcell_net_send",
                          tqitem,
                          "nullptr",
                          point_process,
                          get_variable_name("t"));
    } else {
        const auto& t = get_variable_name("t");
        printer->add_text("net_send_buffering(");
        std::string weight_ptr = weight_index == "0" ? "0"
                                                     : fmt::format("(intptr_t){}", weight_index);
        printer->fmt_text(
            "nt, _ml_arg, _ml_arg->_net_send_buffer, 0, (intptr_t)&{}, {}, "
            "(intptr_t){}, {}+",
            tqitem,
            weight_ptr,
            point_process,
            t);
    }
    print_vector_elements(arguments, ", ");
    printer->add_text(')');
}

void CodegenNeuronAccVisitor::print_net_move_call(const ast::FunctionCall& node) {
    if (!printing_net_receive && !printing_net_init) {
        throw std::runtime_error("Error : net_move only allowed in NET_RECEIVE block");
    }
    if (printing_net_receive || printing_net_init) {
        CodegenNeuronCppVisitor::print_net_move_call(node);
        return;
    }

    const auto& tqitem = get_variable_name("tqitem", false);
    const auto& point_process = get_variable_name(naming::POINT_PROCESS_VARIABLE, false);
    if (info.artificial_cell) {
        printer->fmt_text("artcell_net_move(&{}, {}, ", tqitem, point_process);
        print_vector_elements(node.get_arguments(), ", ");
        printer->add_text(")");
        return;
    }
    printer->add_text("net_send_buffering(");
    printer->fmt_text(
        "nt, _ml_arg, _ml_arg->_net_send_buffer, 2, (intptr_t)&{}, (intptr_t)-1, "
        "(intptr_t){}, ",
        tqitem,
        point_process);
    print_vector_elements(node.get_arguments(), ", ");
    printer->add_text(", 0.0, 0.0");
    printer->add_text(")");
}

void CodegenNeuronAccVisitor::print_net_event_call(const ast::FunctionCall& node) {
    const auto& arguments = node.get_arguments();
    if (info.artificial_cell) {
        printer->add_text("net_event(pnt, ");
        print_vector_elements(arguments, ", ");
        printer->add_text(")");
        return;
    }
    const auto& point_process = get_variable_name(naming::POINT_PROCESS_VARIABLE, false);
    printer->add_text("net_send_buffering(");
    printer->fmt_text(
        "nt, _ml_arg, _ml_arg->_net_send_buffer, 1, (intptr_t)-1, (intptr_t)-1, "
        "(intptr_t){}, ",
        point_process);
    print_vector_elements(arguments, ", ");
    printer->add_text(", 0.0, 0.0");
    printer->add_text(")");
}

void CodegenNeuronAccVisitor::print_mechanism_range_var_structure(bool print_initializers) {
    auto const value_initialize = print_initializers ? "{}" : "";
    printer->add_newline(2);
    printer->add_line("/** mechanism instance: SoA index base + ion pdata handles (GPU index path) */");
    printer->fmt_push_block("struct {} ", instance_struct());

    for (auto const& [var, type]: info.neuron_global_variables) {
        auto const name = var->get_name();
        printer->fmt_line("{}* {}{};",
                          type,
                          name,
                          print_initializers ? fmt::format("{{&::{}}}", name) : std::string{});
    }

    printer->fmt_line("std::size_t _data_offset{};", value_initialize);

    for (auto& var: codegen_int_variables) {
        const auto& name = var.symbol->get_name();
        auto position = position_of_int_var(name);

        if (name == naming::POINT_PROCESS_VARIABLE) {
            continue;
        } else if (var.is_index || var.is_integer) {
        } else if (info.semantics[position].name == naming::POINTER_SEMANTIC) {
        } else {
            auto qualifier = var.is_constant ? "const " : "";
            auto type = var.is_vdata ? "void*" : default_float_data_type();
            printer->fmt_line("{}{}* const* {}{};", qualifier, type, name, value_initialize);
        }
    }

    if (!codegen_global_variables.empty()) {
        printer->fmt_line("{}* {}{};",
                          global_struct(),
                          naming::INST_GLOBAL_MEMBER,
                          print_initializers ? fmt::format("{{&{}}}", global_struct_instance())
                                             : std::string{});
    }
    printer->pop_block(";");
}

std::string CodegenNeuronAccVisitor::indexed_fp_var(std::string_view name,
                                                    std::string_view index_expr) const {
    auto const position = position_of_float_var(std::string{name});
    return fmt::format("_present_fp_{}[inst._data_offset + {}]", position, index_expr);
}

int CodegenNeuronAccVisitor::conductance_fp_index() const {
    auto const& name = info.vectorize ? naming::CONDUCTANCE_UNUSED_VARIABLE
                                      : naming::CONDUCTANCE_VARIABLE;
    return position_of_float_var(name);
}

void CodegenNeuronAccVisitor::print_v_unused() const {
    if (!info.vectorize) {
        return;
    }
    printer->fmt_line("{} = v;", indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
}

void CodegenNeuronAccVisitor::print_g_unused() const {
    printer->add_multi_line(R"CODE(
        #if NRN_PRCELLSTATE
    )CODE");
    printer->fmt_line("{} = g;", indexed_fp_var(naming::CONDUCTANCE_UNUSED_VARIABLE));
    printer->add_line("#endif");
}

void CodegenNeuronAccVisitor::print_nrn_init(bool skip_init_check) {
    (void) skip_init_check;
    printer->add_newline(2);

    print_global_function_common_code(BlockType::Initial);

    use_present_fp_indexing_ = true;
    print_parallel_iteration_hint(BlockType::Initial, info.initial_node);
    printer->push_block("for (int id = 0; id < nodecount; id++)");

    printer->add_line("auto* _ppvar = _ml_arg->pdata[id];");
    if (!info.artificial_cell) {
        printer->add_line("int node_id = node_data.nodeindices[id];");
        printer->fmt_line("{} = node_data.node_voltages[node_id];",
                          indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
    }

    print_rename_state_vars();

    if (!info.changed_dt.empty()) {
        printer->fmt_line("double _save_prev_dt = {};",
                          get_variable_name(naming::NTHREAD_DT_VARIABLE));
        printer->fmt_line("{} = {};",
                          get_variable_name(naming::NTHREAD_DT_VARIABLE),
                          info.changed_dt);
    }

    print_initial_block(info.initial_node);

    if (!info.changed_dt.empty()) {
        printer->fmt_line("{} = _save_prev_dt;", get_variable_name(naming::NTHREAD_DT_VARIABLE));
    }

    printer->pop_block();
    print_kernel_data_present_annotation_block_end();
    printer->pop_block();
    use_present_fp_indexing_ = false;
}

void CodegenNeuronAccVisitor::print_nrn_current(const ast::BreakpointBlock& node) {
    use_present_fp_indexing_ = true;
    const auto& args = nrn_current_parameters();
    const auto& block = node.get_statement_block();
    printer->add_newline(2);
    printer->fmt_push_block("static inline double nrn_current_{}({})",
                            info.mod_suffix,
                            get_parameter_str(args));
    printer->fmt_line("{} = v;", indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
    printer->add_line("double current = 0.0;");
    print_statement_block(*block, false, false);
    for (auto& current: info.currents) {
        const auto& name = get_variable_name(current);
        printer->fmt_line("current += {};", name);
    }
    printer->add_line("return current;");
    printer->pop_block();
    use_present_fp_indexing_ = false;
}

void CodegenNeuronAccVisitor::print_nrn_state() {
    if (!nrn_state_required()) {
        return;
    }

    printer->add_newline(2);
    print_global_function_common_code(BlockType::State);

    use_present_fp_indexing_ = true;
    print_parallel_iteration_hint(BlockType::State, info.nrn_state_block);
    printer->push_block("for (int id = 0; id < nodecount; id++)");
    printer->add_line("int node_id = node_data.nodeindices[id];");
    printer->add_line("auto* _ppvar = _ml_arg->pdata[id];");
    if (!info.artificial_cell) {
        printer->fmt_line("{} = node_data.node_voltages[node_id];",
                          indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
    }

    if (ion_variable_struct_required()) {
        throw std::runtime_error("Not implemented.");
    }

    auto read_statements = ion_read_statements(BlockType::State);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    if (info.nrn_state_block) {
        info.nrn_state_block->visit_children(*this);
    }

    for (auto& block: info.matexp_blocks) {
        if (!block->get_steadystate().get()->eval()) {
            block->accept(*this);
        }
    }

    if (info.currents.empty() && info.breakpoint_node != nullptr) {
        auto block = info.breakpoint_node->get_statement_block();
        print_statement_block(*block, false, false);
    }

    const auto& write_statements = ion_write_statements(BlockType::State);
    for (auto& statement: write_statements) {
        const auto& text = process_shadow_update_statement(statement, BlockType::State);
        printer->add_line(text);
    }

    printer->pop_block();
    print_kernel_data_present_annotation_block_end();
    printer->pop_block();
    use_present_fp_indexing_ = false;
}

CodegenNeuronAccVisitor::ParamVector CodegenNeuronAccVisitor::nrn_current_parameters() {
    auto params = CodegenNeuronCppVisitor::nrn_current_parameters();
    auto const v_param = params.back();
    params.pop_back();
    for (int i = 0; i < static_cast<int>(codegen_float_variables.size()); ++i) {
        params.emplace_back("", "double*", "", fmt::format("_present_fp_{}", i));
    }
    params.push_back(v_param);
    return params;
}

void CodegenNeuronAccVisitor::print_nrn_cur() {
    if (!nrn_cur_required()) {
        return;
    }

    if (info.conductances.empty()) {
        print_nrn_current(*info.breakpoint_node);
    }

    printer->add_newline(2);
    printer->add_line("/** update current */");
    print_global_function_common_code(BlockType::Equation);
    use_present_fp_indexing_ = true;
    print_parallel_iteration_hint(BlockType::Equation, info.breakpoint_node);
    printer->push_block("for (int id = 0; id < nodecount; id++)");
    print_nrn_cur_kernel(*info.breakpoint_node);

    printer->fmt_line("vec_rhs[node_id] {} rhs;", operator_for_rhs());

    if (breakpoint_exist()) {
        printer->fmt_line("{} = g;", indexed_fp_var(info.vectorize ? naming::CONDUCTANCE_UNUSED_VARIABLE
                                                                    : naming::CONDUCTANCE_VARIABLE));
    }
    printer->pop_block();

    print_after_nrn_cur_gpu_net_send_flush();
    print_kernel_data_present_annotation_block_end();
    printer->pop_block();
    use_present_fp_indexing_ = false;
}

void CodegenNeuronAccVisitor::print_entrypoint_setup_code_from_prop() {
    if (info.mod_suffix == "nothing") {
        return;
    }

    printer->add_line("Datum* _ppvar = _nrn_mechanism_access_dparam(prop);");
    printer->add_line("_nrn_mechanism_cache_instance _lmc{prop};");
    printer->add_line("const size_t id = 0;");

    printer->fmt_line("auto inst = make_instance_{}(&_lmc);", info.mod_suffix);
    if (!info.artificial_cell) {
        printer->fmt_line("auto node_data = make_node_data_{}(prop);", info.mod_suffix);
    }

    if (!codegen_thread_variables.empty()) {
        printer->fmt_line("auto _thread_vars = {}({}_global.thread_data);",
                          thread_variables_struct(),
                          info.mod_suffix);
    }

    printer->add_newline();
}

std::string CodegenNeuronAccVisitor::int_variable_name(const IndexVariableInfo& symbol,
                                                       const std::string& name,
                                                       bool use_instance) const {
    if (use_present_fp_indexing_ && use_instance) {
        auto const position = position_of_int_var(name);
        if (info.semantics[position].name == naming::RANDOM_SEMANTIC ||
            info.semantics[position].name == naming::FOR_NETCON_SEMANTIC ||
            info.semantics[position].name == naming::POINTER_SEMANTIC) {
            return CodegenNeuronCppVisitor::int_variable_name(symbol, name, use_instance);
        }
        if (symbol.is_index || symbol.is_integer) {
            return CodegenNeuronCppVisitor::int_variable_name(symbol, name, use_instance);
        }
        return fmt::format("(*_present_dptr_{}[inst._data_offset + id])", position);
    }
    return CodegenNeuronCppVisitor::int_variable_name(symbol, name, use_instance);
}

std::string CodegenNeuronAccVisitor::float_variable_name(const SymbolType& symbol,
                                                         bool use_instance) const {
    if (!use_instance) {
        return CodegenNeuronCppVisitor::float_variable_name(symbol, use_instance);
    }

    auto const position = position_of_float_var(symbol->get_name());
    if (!use_present_fp_indexing_) {
        if (symbol->is_array()) {
            auto const dimension = symbol->get_length();
            return fmt::format("(_lmc.template data_array_ptr<{}, {}>() + id * {})",
                               position,
                               dimension,
                               dimension);
        }
        return fmt::format("_lmc.template fpfield<{}>(id)", position);
    }
    if (symbol->is_array()) {
        auto const dimension = symbol->get_length();
        return fmt::format("(_present_fp_{} + inst._data_offset + id * {})", position, dimension);
    }
    return fmt::format("_present_fp_{}[inst._data_offset + id]", position);
}

void CodegenNeuronAccVisitor::print_data_structures(bool print_initializers) {
    print_mechanism_global_var_structure(print_initializers);
    print_mechanism_range_var_structure(false);
    print_node_data_structure(print_initializers);
    print_thread_variables_structure(print_initializers);
    print_make_instance();
    print_make_node_data();
}

void CodegenNeuronAccVisitor::print_make_instance() const {
    printer->add_newline(2);
    printer->fmt_push_block("static {} make_instance_{}(std::size_t data_offset, "
                            "_nrn_mechanism_cache_range* _lmc, "
                            "bool use_device_ptrs = false)",
                            instance_struct(),
                            info.mod_suffix);

    printer->push_block("if(_lmc == nullptr)");
    printer->fmt_line("return {}();", instance_struct());
    printer->pop_block_nl(2);

    printer->fmt_push_block("return {}", instance_struct());

    std::vector<std::string> make_instance_args;

    for (auto const& [var, type]: info.neuron_global_variables) {
        auto const name = var->get_name();
        // Host address only: OpenACC present(hh_global, ...) maps globals; device
        // pointers in inst confuse the compiler's implicit inst.global upload.
        make_instance_args.push_back(fmt::format("&::{0}", name));
    }

    make_instance_args.emplace_back("data_offset");

    const auto codegen_int_variables_size = codegen_int_variables.size();
    for (size_t i = 0; i < codegen_int_variables_size; ++i) {
        const auto& var = codegen_int_variables[i];
        auto name = var.symbol->get_name();
        auto sem = info.semantics[i].name;
        auto const variable = [&var, &sem, i]() -> std::string {
            if (var.is_index || var.is_integer) {
                return "";
            } else if (var.is_vdata) {
                return "";
            } else if (sem == naming::POINTER_SEMANTIC) {
                return "";
            } else {
                return fmt::format("_lmc->template dptr_field_ptr<{}>()", i);
            }
        }();
        if (variable != "") {
            make_instance_args.push_back(variable);
        }
    }

    if (!codegen_global_variables.empty()) {
        make_instance_args.push_back(
            fmt::format("&{0}", global_struct_instance()));
    }

    printer->add_multi_line(fmt::format("{}", fmt::join(make_instance_args, ",\n")));

    printer->pop_block(";");
    printer->pop_block();

    printer->add_newline(2);
    printer->fmt_push_block("static {} make_instance_{}(_nrn_mechanism_cache_instance* _lmc)",
                            instance_struct(),
                            info.mod_suffix);
    printer->push_block("if(_lmc == nullptr)");
    printer->fmt_line("return {}();", instance_struct());
    printer->pop_block_nl(2);
    printer->fmt_push_block("return {}", instance_struct());
    printer->add_line("0");
    printer->pop_block(";");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_make_node_data() const {
    printer->add_newline(2);
    printer->fmt_push_block("static {} make_node_data_{}(NrnThread& nt, Memb_list& _ml_arg, "
                            "bool use_device_ptrs = false)",
                            node_data_struct(),
                            info.mod_suffix);

    // Host pointers: OpenACC present() maps these to device copies inside kernels.
    std::vector<std::string> make_node_data_args = {"_ml_arg.nodeindices",
                                                    "nt.node_voltage_storage()",
                                                    "nt.node_d_storage()",
                                                    "nt.node_rhs_storage()",
                                                    "_ml_arg.nodecount"};

    printer->fmt_push_block("return {}", node_data_struct());
    printer->add_multi_line(fmt::format("{}", fmt::join(make_node_data_args, ",\n")));
    printer->pop_block(";");
    printer->pop_block();

    printer->fmt_push_block("static {} make_node_data_{}(Prop* _prop)",
                            node_data_struct(),
                            info.mod_suffix);

    printer->push_block("if(!_prop)");
    printer->fmt_line("return {}();", node_data_struct());
    printer->pop_block_nl(2);

    printer->add_line("static std::vector<int> node_index{0};");
    printer->add_line("Node* _node = _nrn_mechanism_access_node(_prop);");

    make_node_data_args = {"node_index.data()",
                           "&_nrn_mechanism_access_voltage(_node)",
                           "&_nrn_mechanism_access_d(_node)",
                           "&_nrn_mechanism_access_rhs(_node)",
                           "1"};

    printer->fmt_push_block("return {}", node_data_struct());
    printer->add_multi_line(fmt::format("{}", fmt::join(make_node_data_args, ",\n")));
    printer->pop_block(";");
    printer->pop_block();
    printer->add_newline();
}

void CodegenNeuronAccVisitor::print_entrypoint_setup_code_from_memb_list() {
    if (info.mod_suffix == "nothing") {
        return;
    }

    printer->add_line(
        "_nrn_mechanism_cache_range _lmc{_sorted_token, *nt, *_ml_arg, _ml_arg->type()};");
    printer->fmt_line("auto inst = make_instance_{}(_ml_arg->get_storage_offset(), &_lmc, "
                      "nt->compute_gpu);",
                      info.mod_suffix);
    if (!info.artificial_cell) {
        printer->fmt_line("auto node_data = make_node_data_{}(*nt, *_ml_arg, nt->compute_gpu);",
                          info.mod_suffix);
    }
    printer->add_line("auto* _thread = _ml_arg->_thread;");
    if (!codegen_thread_variables.empty()) {
        printer->fmt_line("auto _thread_vars = {}(_thread[{}].get<double*>());",
                          thread_variables_struct(),
                          info.thread_var_thread_id);
    }
}

}  // namespace codegen
}  // namespace nmodl