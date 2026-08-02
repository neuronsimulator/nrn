#include "codegen/codegen_neuron_acc_visitor.hpp"

#include "ast/all.hpp"
#include "ast/block.hpp"
#include "ast/function_call.hpp"
#include "visitors/visitor_utils.hpp"

#include <cstring>

namespace nmodl {
namespace codegen {

std::string CodegenNeuronAccVisitor::backend_name() const {
    return "C++-OpenAcc-NEURON";
}

void CodegenNeuronAccVisitor::print_standard_includes() {
    CodegenNeuronCppVisitor::print_standard_includes();
    printer->add_line("#include <neuron/gpu/offload.hpp>");
    printer->add_line("#include <neuron/gpu/net_send_buffer.hpp>");
    printer->add_line("#include <neuron/gpu/net_receive_buffer.hpp>");
    printer->add_line("#include <neuron/gpu/mechanism_phases.hpp>");
    printer->add_line("#include <neuron/gpu/sync.hpp>");
    printer->add_line("#include <neuron/gpu/download.hpp>");
    printer->add_line("#include <neuron/event_order.hpp>");
    // net_buf_receive needs complete model_sorted_token + nrn_ensure_model_data_are_sorted.
    printer->add_line("#include \"nrn_ansi.h\"");
    printer->add_line("#include \"neuron/model_data.hpp\"");
}

bool CodegenNeuronAccVisitor::host_only_parallel_block(BlockType type) const {
    // INITIAL: host for ion wrote_conc, net_send/net_move (queue API), RANDOM
    // (nrnran123 not device-callable yet), or nrn_ghk (host codata/celsius).
    if (type != BlockType::Initial) {
        return false;
    }
    if (info.require_wrote_conc || info.net_send_used || info.net_event_used ||
        info.nrn_ghk_used) {
        return true;
    }
    for (const auto& sem: info.semantics) {
        if (sem.name == naming::RANDOM_SEMANTIC) {
            return true;
        }
    }
    return false;
}

void CodegenNeuronAccVisitor::print_global_var_struct_decl() {
    CodegenNeuronCppVisitor::print_global_var_struct_decl();
    if (!info.artificial_cell && !codegen_global_variables.empty()) {
        auto const& global = global_struct_instance();
        printer->fmt_line("static bool {}_gpu_resident = false;", global);
        // Host TABLE rebuild / GLOBAL mutation sets this; kernels H→D only when true.
        // Avoids per-step update device of full globals (HH: 6×201 table doubles).
        printer->fmt_line("static bool {}_device_stale = true;", global);
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
    // copyin snapshots host; further host edits require stale→update.
    printer->add_line(global + "_device_stale = false;");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_global_variable_device_update_annotation() const {
    if (info.artificial_cell || codegen_global_variables.empty()) {
        return;
    }
    // H4a: only H→D when host marked globals dirty (TABLE rebuild, etc.).
    // Prior codegen updated the full struct every CURRENT/STATE/JACOB launch.
    auto const& global = global_struct_instance();
    printer->fmt_push_block("if (nt->compute_gpu && {}_gpu_resident && {}_device_stale)",
                            global,
                            global);
    printer->fmt_line("nrn_pragma_acc(update device ({}))", global);
    printer->fmt_line("nrn_pragma_omp(target update to({}))", global);
    printer->add_line(global + "_device_stale = false;");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_mark_global_device_stale() const {
    if (info.artificial_cell || codegen_global_variables.empty()) {
        return;
    }
    printer->fmt_line("{}_device_stale = true;", global_struct_instance());
}

void CodegenNeuronAccVisitor::print_after_host_table_rebuild() {
    print_mark_global_device_stale();
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
        // GPU cache is the full-instance pdata table; host dptr_field_ptr is already
        // advanced by ml storage offset. When using the GPU table, also record
        // storage_offset so (*_present_dptr_i[id + base]) hits the right instance
        // (multi-thread: offset>0). Host path uses base=0.
        printer->fmt_line(
            "double* const* _present_dptr_{0} = nullptr;", i);
        printer->fmt_line("int _present_dptr_base_{0} = 0;", i);
        printer->push_block(
            "if (nt->compute_gpu && "
            "neuron::mechanism::_get::gpu_pdata_ptr_cache(_sorted_token, _ml_arg->type()))");
        printer->fmt_line(
            "_present_dptr_{0} = "
            "neuron::mechanism::_get::gpu_pdata_ptr_cache(_sorted_token, _ml_arg->type())[{0}];",
            i);
        printer->fmt_line(
            "_present_dptr_base_{0} = static_cast<int>(_ml_arg->get_storage_offset());", i);
        printer->pop_block();
        printer->push_block("else");
        printer->fmt_line("_present_dptr_{0} = _lmc.template dptr_field_ptr<{0}>();", i);
        printer->pop_block();
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

void CodegenNeuronAccVisitor::print_functors_definitions() {
    // Functor params include _present_fp_* (internal_method_parameters) and the
    // state loop constructs them with those pointers. initialize()/operator()
    // must index via present_fp, not _lmc.template fpfield (no _lmc member).
    use_present_fp_indexing_ = true;
    CodegenCppVisitor::print_functors_definitions();
    use_present_fp_indexing_ = false;
}

void CodegenNeuronAccVisitor::print_hoc_py_wrapper_before_table_update() {
    // Include artificial cells (VecStim): ACC MOD FUNCTIONs take _present_fp_*.
    if (info.mod_suffix != "nothing" && !codegen_float_variables.empty()) {
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

    if (type == BlockType::NetReceive) {
        // Present pointers declared by print_net_receive_buffering before the loop.
    } else if (!info.artificial_cell) {
        printer->add_line("auto const* nodeindices = node_data.nodeindices;");
        // Device-resident voltages: use deviceptr, never present(host V).
        // present(node_voltages) re-uploads stale host V on nvc++ when host is
        // dirty/out of date — host must not participate in psolve V traffic.
        printer->add_line(
            "double* _d_voltages = nt->compute_gpu "
            "? static_cast<double*>(acc_deviceptr(const_cast<double*>(node_data.node_voltages))) "
            ": const_cast<double*>(node_data.node_voltages);");
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
        // nrb metadata + Weight SoA + mechanism RANGE columns for NET_RECEIVE body.
        present_clause << ", nrb"
                          ", nrb->_displ[:nrb->_displ_cnt + 1]"
                          ", nrb->_nrb_index[:nrb->_cnt]"
                          ", nrb->_pnt_index[:nrb->_cnt]"
                          ", nrb->_weight_index[:nrb->_cnt]"
                          ", nrb->_nrb_t[:nrb->_cnt]"
                          ", nrb->_nrb_flag[:nrb->_cnt]"
                          ", weights[:weight_count]";
        if (info.net_send_used || info.net_event_used) {
            // Device net_send_buffering writes; host flush resolves indices after wait.
            present_clause << ", nsb"
                              ", nsb->_sendtype[:nsb->_size]"
                              ", nsb->_vdata_index[:nsb->_size]"
                              ", nsb->_weight_index[:nsb->_size]"
                              ", nsb->_pnt_index[:nsb->_size]"
                              ", nsb->_nsb_t[:nsb->_size]"
                              ", nsb->_nsb_flag[:nsb->_size]";
        }
        const auto codegen_float_variables_size = codegen_float_variables.size();
        for (int i = 0; i < codegen_float_variables_size; ++i) {
            const auto& float_var = codegen_float_variables[i];
            auto const array_len = float_var->is_array() ? float_var->get_length() : 1;
            present_clause << fmt::format(
                ", _present_fp_{}[:static_cast<std::size_t>(_ml_arg->nodecount) * {}]", i, array_len);
        }
    } else if (!info.artificial_cell) {
        present_clause << ", nodeindices, _thread";
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

    std::string deviceptr_extra = present_dptr_deviceptr_clause();
    if (!info.artificial_cell && type != BlockType::NetReceive) {
        if (deviceptr_extra.empty()) {
            deviceptr_extra = " deviceptr(_d_voltages)";
        } else {
            // present_dptr returns " deviceptr(a, b, ...)"; insert _d_voltages first.
            auto pos = deviceptr_extra.find("deviceptr(");
            if (pos != std::string::npos) {
                deviceptr_extra.insert(pos + std::strlen("deviceptr("), "_d_voltages, ");
            }
        }
    }

    // force_seq_acc_loop_: NRN_DETERMINISTIC_MATRIX PP path — serial instance order
    // matches CPU association for vec_rhs/vec_d (no unordered atomics).
    if (force_seq_acc_loop_) {
        printer->fmt_line(
            "nrn_pragma_acc(parallel loop seq {}{} async(nt->stream_id) if(nt->compute_gpu))",
            present_clause.str(),
            type == BlockType::NetReceive ? std::string{} : deviceptr_extra);
        printer->add_line(
            "nrn_pragma_omp(target teams distribute parallel for if(nt->compute_gpu))");
    } else {
        printer->fmt_line(
            "nrn_pragma_acc(parallel loop {}{} async(nt->stream_id) if(nt->compute_gpu))",
            present_clause.str(),
            type == BlockType::NetReceive ? std::string{} : deviceptr_extra);
        printer->add_line(
            "nrn_pragma_omp(target teams distribute parallel for if(nt->compute_gpu))");
    }
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
    // Always atomic capture. Do not branch on nt->compute_gpu: the device copy of
    // NrnThread can have a stale compute_gpu==0 while the parallel region still
    // runs on the GPU (OpenACC evaluates the loop's if() on the host). A non-atomic
    // cnt++ then races and drops almost all net_send slots (Traub: ~300 of ~1e5).
    printer->add_line("nrn_pragma_acc(atomic capture)");
    printer->add_line("nrn_pragma_omp(atomic capture)");
    printer->add_line("i = nsb->_cnt++;");
}

void CodegenNeuronAccVisitor::print_net_send_buffering_grow() {
    // No-op for ACC: OpenACC device-compiles this whole function when it is called
    // from a parallel loop, so host-only ensure/grow symbols must not appear here
    // (nvlink undefined reference). Host must pre-size before the OpenACC region
    // (net_send_buffer_ensure_for_events: headroom × events + high-water). If
    // i >= nsb->_size the slot is not written; update_net_send_buffer_on_host
    // aborts when _cnt > _size (never silent drop).
    (void) 0;
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
    // Index ABI: tqitem ppvar field, weight SoA index, mechanism instance id (not host pointers).
    // Device-callable: no host-only ensure/grow (pre-size on host before OpenACC regions).
    auto args =
        "NrnThread* nt, Memb_list* ml, neuron::gpu::NetSendBuffer_t* nsb, int type, "
        "int vdata_index, int weight_index, int pnt_instance_id, double t, double flag";
    printer->fmt_push_block("static inline void net_send_buffering({})", args);
    printer->add_line("(void) ml;");
    printer->add_line("int i = 0;");
    print_net_send_buffering_cnt_update();
    printer->push_block("if (i < nsb->_size)");
    printer->add_multi_line(R"CODE(
         nsb->_sendtype[i] = type;
         nsb->_vdata_index[i] = vdata_index;
         nsb->_weight_index[i] = weight_index;
         nsb->_pnt_index[i] = pnt_instance_id;
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
    printer->add_line("neuron::gpu::deliver_net_send_buffer_events(nt, _ml_arg, nsb);");
    print_net_send_buf_count_update_to_device();
}

void CodegenNeuronAccVisitor::print_after_nrn_cur_gpu_net_send_flush() {
    if (info.net_send_used && !info.artificial_cell) {
        print_send_event_move();
    }
}

void CodegenNeuronAccVisitor::print_compute_functions() {
    print_net_send_buffering();
    print_nrn_init();
    print_nrn_cur();
    print_nrn_state();
    print_nrn_jacob();
    // Host WATCH cond/alloc before NET_RECEIVE (WATCH mechs use host receive).
    print_watch_support();
    // Stage 2: device NET_RECEIVE kernel + buffering before host pnt_receive (rename once).
    print_net_receive_buffering();
    print_net_receive();
    print_net_init();
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
    auto print_jacob_device_loop = [&](bool det_matrix) {
        force_seq_acc_loop_ = det_matrix;
        print_parallel_iteration_hint(BlockType::Equation, nullptr);
        printer->push_block("for (int id = 0; id < nodecount; id++)");
        printer->add_line("int node_id = node_data.nodeindices[id];");
        // g_unused from nrn_cur. PP: atomic unless NRN_DETERMINISTIC_MATRIX (seq loop).
        if (info.point_process && !det_matrix) {
            printer->add_line("nrn_pragma_acc(atomic update)");
            printer->add_line("nrn_pragma_omp(atomic update)");
        }
        // _present_fp_* from fpfield_ptr() already include ml storage offset.
        printer->fmt_line("vec_d[node_id] {} _present_fp_{}[id];",
                          operator_for_d(),
                          conductance_fp_index());
        printer->pop_block();
        force_seq_acc_loop_ = false;
    };
    if (info.point_process) {
        printer->push_block("if (neuron::event_order::matrix_enabled())");
        print_jacob_device_loop(true);
        printer->chain_block("else");
        print_jacob_device_loop(false);
        printer->pop_block();
    } else {
        print_jacob_device_loop(false);
    }
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
    if (info.electrode_current) {
        printer->push_block("if (auto* vec_sav_d = nt->node_sav_d_storage())");
        printer->fmt_line("vec_sav_d[node_id] {} _lmc.template fpfield<{}>(id);",
                          operator_for_d(),
                          conductance_fp_index());
        printer->pop_block();
    }
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

void CodegenNeuronAccVisitor::print_net_receive_registration() {
    CodegenNeuronCppVisitor::print_net_receive_registration();
    if (net_receive_buffering_required()) {
        printer->fmt_line("hoc_register_net_receive_buffering(net_buf_receive_{}, mech_type);",
                          info.mod_suffix);
    }
}

void CodegenNeuronAccVisitor::print_net_receive() {
    auto node = info.net_receive_node;
    if (!node) {
        return;
    }

    printing_net_receive = true;
    // Rename NET_RECEIVE args → _args[i] once for host + device bodies.
    rename_net_receive_arguments(*node, *node);

    printer->fmt_push_block("static void nrn_net_receive_{}({})",
                            info.mod_suffix,
                            get_parameter_str(net_receive_args()));

    // Always resolve thread once (shared by GPU enqueue and host path).
    printer->add_line("auto* nt = static_cast<NrnThread*>(_pnt->_vnt);");

    // Stage 2: device net_buf_receive for ordinary NET_RECEIVE. Host path when:
    // - WATCH (host WatchCondition + RANGE write for device CURRENT)
    // - BBCOREPOINTER (Gfluct3 oup/mynormrand: host Random123; push SoA after)
    const bool host_net_receive =
        info.is_watch_used() || info.bbcore_pointer_used;
    if (net_receive_buffering_required() && !host_net_receive) {
        printer->push_block("if (nt && nt->compute_gpu)");
        printer->add_line(
            "Memb_list* ml = nt->_ml_list[_nrn_mechanism_get_type(_pnt->prop)];");
        printer->push_block("if (ml)");
        printer->add_line(
            "int pnt_index = static_cast<int>(neuron::mechanism::_get::_current_row(_pnt->prop) - "
            "ml->get_storage_offset());");
        printer->add_line("neuron::gpu::net_receive_buffer_ensure(ml);");
        printer->add_line(
            "neuron::gpu::net_receive_buffer_enqueue(nt, ml, pnt_index, _weight_index, flag);");
        printer->pop_block();
        printer->add_line("return;");
        printer->pop_block();
    }

    // Host path (CPU backend, non-GPU threads, or WATCH mechs under native GPU).
    // (nt already declared above — do not redeclare.)
    printer->add_line("_nrn_mechanism_cache_instance _lmc{_pnt->prop};");
    printer->add_line("auto * _ppvar = _nrn_mechanism_access_dparam(_pnt->prop);");
    printer->fmt_line("double* _args = _nrn_netrec_wsoa(_weight_index, {});",
                      info.num_net_receive_parameters);
    printer->fmt_line("auto inst = make_instance_{}(&_lmc);", info.mod_suffix);
    if (!info.artificial_cell) {
        printer->fmt_line("auto node_data = make_node_data_{}(_pnt->prop);", info.mod_suffix);
    }
    printer->add_line("Datum * _thread = nullptr;");
    printer->add_line("size_t id = 0;");
    printer->add_line("double t = nt->_t;");
    // MOD FUNCTION/PROCEDURE signatures always take _present_fp_* (ACC). Host
    // NET_RECEIVE body must declare them or Gfluct3-style calls fail to compile.
    print_present_fp_pointer_declarations();
    if (info.is_watch_used()) {
        printer->add_line("int _watch_rm = 0;");
    }
    // Reset watch index counter so activates match _watchN_cond numbering.
    current_watch_statement = 0;
    print_statement_block(*node->get_statement_block(), false, false);
    printer->fmt_line("_nrn_netrec_wsoa_done(_weight_index, {}, _args);",
                      info.num_net_receive_parameters);
    if (host_net_receive) {
        // Device CURRENT (and BA fold) need host-written RANGE (g,e / g_e1,g_i1).
        printer->push_block("if (nt && nt->compute_gpu)");
        printer->add_line(
            "neuron::gpu::upload_present_mechanism_soa_to_device(_nrn_mechanism_get_type(_pnt->prop));");
        printer->pop_block();
    }
    printer->add_newline();
    printer->pop_block();
    printing_net_receive = false;
}

void CodegenNeuronAccVisitor::print_net_receive_buffering() {
    if (!net_receive_buffering_required()) {
        return;
    }

    auto* node = info.net_receive_node;
    if (!node) {
        return;
    }

    // WATCH / BBCOREPOINTER use host NET_RECEIVE only. Emitting an ACC kernel that
    // calls VERBATIM RANDOM (Gfluct3 mynormrand) fails device compile even if the
    // kernel is never entered — stub the device buffer path.
    if (info.is_watch_used() || info.bbcore_pointer_used) {
        printer->add_newline(2);
        printer->fmt_push_block("static void net_buf_receive_{}(NrnThread* /*nt*/)",
                                info.mod_suffix);
        printer->add_line("/* host-only NET_RECEIVE (WATCH or BBCOREPOINTER RANDOM) */");
        printer->pop_block();
        return;
    }

    // Ensure args are renamed before generating host + device bodies (mutates AST).
    rename_net_receive_arguments(*node, *node);

    printer->add_newline(2);
    printer->add_line(
        "/** CoreNEURON-style: apply queued NET_RECEIVE on device (Weight SoA + net_send buffer). */");
    printer->fmt_push_block("static void net_buf_receive_{}(NrnThread* nt)", info.mod_suffix);

    printer->push_block("if (!nt || !nt->compute_gpu)");
    printer->add_line("return;");
    printer->pop_block();

    printer->add_line("Memb_list* _ml_arg = nt->_ml_list[mech_type];");
    printer->push_block("if (!_ml_arg)");
    printer->add_line("return;");
    printer->pop_block();

    printer->add_line("neuron::gpu::NetReceiveBuffer_t* nrb = _ml_arg->_net_receive_buffer;");
    printer->push_block("if (!nrb || !nrb->_cnt)");
    printer->add_line("return;");
    printer->pop_block();

    printer->add_line("auto const& _sorted_token = nrn_ensure_model_data_are_sorted();");
    printer->add_line(
        "_nrn_mechanism_cache_range _lmc{_sorted_token, *nt, *_ml_arg, _ml_arg->type()};");
    printer->fmt_line(
        "auto inst = make_instance_{}(_ml_arg->get_storage_offset(), &_lmc, /*use_device_ptrs*/ true);",
        info.mod_suffix);
    if (!info.artificial_cell) {
        // Needed when NET_RECEIVE body calls MOD FUNCTIONs (args include node_data).
        printer->fmt_line(
            "auto node_data = make_node_data_{}(*nt, *_ml_arg, /*use_device_ptrs*/ true);",
            info.mod_suffix);
    }
    printer->add_line("auto* _thread = _ml_arg->_thread;");
    if (!codegen_thread_variables.empty()) {
        printer->fmt_line("auto _thread_vars = {}(_thread[{}].get<double*>());",
                          thread_variables_struct(),
                          info.thread_var_thread_id);
    }

    const auto codegen_float_variables_size = codegen_float_variables.size();
    for (int i = 0; i < codegen_float_variables_size; ++i) {
        printer->fmt_line("double* _present_fp_{0} = _lmc.template fpfield_ptr<{0}>();", i);
    }
    // POINTER/RANDOM dptrs for FUNCTION bodies called from NET_RECEIVE (e.g. Gfluct3).
    print_present_dptr_pointer_declarations();

    printer->add_line("double* weights = neuron::gpu::weight_soa_values();");
    printer->add_line("std::size_t weight_count = neuron::gpu::weight_soa_count();");
    printer->push_block("if (!weights || weight_count == 0)");
    printer->add_line("return;");
    printer->pop_block();

    if (info.net_send_used || info.net_event_used) {
        // Pre-size for this flush: min_events × headroom (default 4) + high-water.
        // Device cannot grow mid-kernel; overflow aborts on host flush.
        printer->add_line(
            "neuron::gpu::net_send_buffer_ensure_for_events(_ml_arg, nrb->_cnt);");
    }

    // Parallel over unique instances; serial over events per instance (CoreNEURON).
    // Body uses _present_fp_* + weights[weight_index+arg]; net_send → NetSendBuffer.
    printer->add_line("int count = nrb->_displ_cnt;");
    print_kernel_global_device_setup();
    // Keep device nt->_t fresh for any residual uses; net_send absolute times use
    // per-event local `t` (nrb->_nrb_t) — see get_variable_name("t").
    printer->push_block("if (nt->compute_gpu)");
    printer->add_line("nrn_pragma_acc(update device(nt->_t))");
    printer->add_line("nrn_pragma_omp(target update to(nt->_t))");
    printer->pop_block();
    // Scope nsb for present() so print_send_event_move can redeclare after.
    printer->add_line("{");
    printer->increase_indent();
    if (info.net_send_used || info.net_event_used) {
        printer->add_line("neuron::gpu::NetSendBuffer_t* nsb = _ml_arg->_net_send_buffer;");
        printer->push_block("if (!nsb)");
        printer->add_line("return;");
        printer->pop_block();
    }
    print_kernel_data_present_annotation_block_begin();
    use_present_fp_indexing_ = true;
    printing_net_buf_receive_kernel_ = true;
    print_parallel_iteration_hint(BlockType::NetReceive, node);
    printer->push_block("for (int i = 0; i < count; i++)");
    printer->add_line("int start = nrb->_displ[i];");
    printer->add_line("int end = nrb->_displ[i + 1];");
    printer->push_block("for (int j = start; j < end; j++)");
    printer->add_multi_line(R"CODE(
        int index = nrb->_nrb_index[j];
        int id = nrb->_pnt_index[index];
        double t = nrb->_nrb_t[index];  // event delivery time (not nt->_t after restore)
        int weight_index = nrb->_weight_index[index];
        double flag = nrb->_nrb_flag[index];
        double* _args = weights + weight_index;
        Datum* _ppvar = _ml_arg->pdata ? _ml_arg->pdata[id] : nullptr;
    )CODE");
    printing_net_receive = true;
    print_statement_block(*node->get_statement_block(), false, false);
    printing_net_receive = false;
    printer->pop_block();  // inner j
    printer->pop_block();  // outer i
    printing_net_buf_receive_kernel_ = false;
    use_present_fp_indexing_ = false;
    print_kernel_data_present_annotation_block_end();  // wait(stream)
    printer->decrease_indent();
    printer->add_line("}");

    printer->add_line("nrb->_displ_cnt = 0;");
    printer->add_line("nrb->_cnt = 0;");
    printer->push_block("if (nt->compute_gpu && nrb->device_uploaded)");
    printer->add_line("nrn_pragma_acc(update device(nrb->_cnt, nrb->_displ_cnt))");
    printer->add_line("nrn_pragma_omp(target update to(nrb->_cnt, nrb->_displ_cnt))");
    printer->pop_block();

    if (info.net_send_used || info.net_event_used) {
        print_send_event_move();
    }

    printer->pop_block();  // net_buf_receive
}

void CodegenNeuronAccVisitor::print_net_send_call(const ast::FunctionCall& node) {
    // Host pnt_receive path, or host-only INITIAL (no present_fp / no nsb):
    // direct net_send (CPU queue API).
    if (((printing_net_receive || printing_net_init) && !printing_net_buf_receive_kernel_) ||
        !use_present_fp_indexing_) {
        CodegenNeuronCppVisitor::print_net_send_call(node);
        return;
    }

    auto const& arguments = node.get_arguments();
    if (info.artificial_cell) {
        const auto& tqitem = get_variable_name("tqitem", /* use_instance */ false);
        std::string point_process = get_variable_name(naming::POINT_PROCESS_VARIABLE, false);
        if (!printing_net_receive) {
            point_process = fmt::format("(Point_process*){}", point_process);
        }
        std::string weight_index = printing_net_receive ? "weight_index" : "-1";
        printer->fmt_text("{}(/* tqitem */ &{}, {}, {}, {} + ",
                          "artcell_net_send",
                          tqitem,
                          weight_index,
                          point_process,
                          get_variable_name("t"));
        print_vector_elements(arguments, ", ");
        printer->add_text(')');
        return;
    }

    // Device path: buffer indices; host deliver resolves via Memb_list pdata.
    // Use present-mapped local `nsb` (same object as update self(nsb->_cnt)).
    int const tq_field = info.tqitem_index >= 0 ? info.tqitem_index : -1;
    std::string const weight_index = printing_net_receive ? "weight_index" : "-1";
    const auto& t = get_variable_name("t");
    printer->add_text("net_send_buffering(");
    printer->fmt_text(
        "nt, _ml_arg, nsb, 0, {}, {}, id, {}+",
        tq_field,
        weight_index,
        t);
    print_vector_elements(arguments, ", ");
    printer->add_text(')');
}

void CodegenNeuronAccVisitor::print_net_move_call(const ast::FunctionCall& node) {
    if (!printing_net_receive && !printing_net_init) {
        throw std::runtime_error("Error : net_move only allowed in NET_RECEIVE block");
    }
    if ((printing_net_receive || printing_net_init) && !printing_net_buf_receive_kernel_) {
        CodegenNeuronCppVisitor::print_net_move_call(node);
        return;
    }

    if (info.artificial_cell) {
        const auto& tqitem = get_variable_name("tqitem", false);
        const auto& point_process = get_variable_name(naming::POINT_PROCESS_VARIABLE, false);
        printer->fmt_text("artcell_net_move(&{}, {}, ", tqitem, point_process);
        print_vector_elements(node.get_arguments(), ", ");
        printer->add_text(")");
        return;
    }

    int const tq_field = info.tqitem_index >= 0 ? info.tqitem_index : -1;
    printer->add_text("net_send_buffering(");
    printer->fmt_text(
        "nt, _ml_arg, nsb, 2, {}, -1, id, ",
        tq_field);
    print_vector_elements(node.get_arguments(), ", ");
    printer->add_text(", 0.0");
    printer->add_text(")");
}

void CodegenNeuronAccVisitor::print_net_event_call(const ast::FunctionCall& node) {
    const auto& arguments = node.get_arguments();
    if (info.artificial_cell) {
        // Host NET_RECEIVE param is `_pnt` (see net_receive_args).
        printer->add_text("net_event(_pnt, ");
        print_vector_elements(arguments, ", ");
        printer->add_text(")");
        return;
    }
    if (printing_net_buf_receive_kernel_ || (!printing_net_receive && !printing_net_init)) {
        printer->add_text("net_send_buffering(");
        // present-mapped local nsb — see print_net_send_call.
        printer->add_text(
            "nt, _ml_arg, nsb, 1, -1, -1, id, ");
        print_vector_elements(arguments, ", ");
        printer->add_text(", 0.0");
        printer->add_text(")");
        return;
    }
    // Host pnt_receive
    const auto& point_process = get_variable_name(naming::POINT_PROCESS_VARIABLE, false);
    printer->fmt_text("net_event({}, t)", point_process);
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
            // Host POINTER; not device-present via Instance.
        } else if (info.semantics[position].name == naming::RANDOM_SEMANTIC || var.is_vdata) {
            // RANDOM / vdata use _ppvar on host (and device NET_RECEIVE); not in
            // Instance SoA — including them mis-orders make_instance args.
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
    if (!use_present_fp_indexing_) {
        // Host-only paths (e.g. INITIAL with wrote_conc) never declare _present_fp_*.
        return fmt::format("_lmc.template fpfield<{}>({})", position, index_expr);
    }
    // present_fp pointers are already advanced by ml storage offset (see
    // MechanismRange::fpfield_ptr). Index with local id only — adding
    // inst._data_offset again double-counts and SEGVs on multi-thread.
    return fmt::format("_present_fp_{}[{}]", position, index_expr);
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

    // Host-only Initial (e.g. ion wrote_conc) falls back to CPU ivdep path and
    // never declares _present_fp_* / _present_dptr_*. Must not use present_fp
    // indexing in that body or cad/ion WRITE INITIAL fails to compile.
    const bool host_only_init = host_only_parallel_block(BlockType::Initial);
    use_present_fp_indexing_ = !host_only_init;
    print_parallel_iteration_hint(BlockType::Initial, info.initial_node);
    printer->push_block("for (int id = 0; id < nodecount; id++)");

    printer->add_line("auto* _ppvar = _ml_arg->pdata[id];");
    if (!info.artificial_cell) {
        printer->add_line("int node_id = node_data.nodeindices[id];");
        // Host-only INITIAL (wrote_conc) uses the CPU ivdep path and never
        // declares _d_voltages / present_fp_* — read host node_voltages there.
        if (host_only_init) {
            printer->fmt_line("{} = node_data.node_voltages[node_id];",
                              indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
        } else {
            printer->fmt_line("{} = _d_voltages[node_id];",
                              indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
        }
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

void CodegenNeuronAccVisitor::print_before_breakpoint_inline() {
    // NEURON ACC does not emit separate hoc_reg_ba for BEFORE BREAKPOINT yet.
    // Fold into CURRENT so ival/g updates (Gfluct3) run on device after host
    // NET_RECEIVE has pushed OU state (g_e1/g_i1).
    for (const auto* block: info.before_after_blocks) {
        if (!block || !block->is_before_block()) {
            continue;
        }
        auto ba_block =
            dynamic_cast<const ast::BeforeBlock*>(block)->get_bablock();
        if (!ba_block || ba_block->get_type()->get_value() != ast::BATYPE_BREAKPOINT) {
            continue;
        }
        print_statement_block(*ba_block->get_statement_block(), false, false);
    }
}

void CodegenNeuronAccVisitor::print_nrn_current(const ast::BreakpointBlock& node) {
    use_present_fp_indexing_ = true;
    // Body reads t via get_variable_name → _nrn_thread_t parameter (firstprivate
    // from host). Device nt->_t is not reliable during CURRENT (IClamp window).
    use_host_captured_t_ = true;
    const auto& args = nrn_current_parameters();
    const auto& block = node.get_statement_block();
    printer->add_newline(2);
    printer->fmt_push_block("static inline double nrn_current_{}({})",
                            info.mod_suffix,
                            get_parameter_str(args));
    printer->fmt_line("{} = v;", indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
    printer->add_line("double current = 0.0;");
    print_before_breakpoint_inline();
    print_statement_block(*block, false, false);
    for (auto& current: info.currents) {
        const auto& name = get_variable_name(current);
        printer->fmt_line("current += {};", name);
    }
    printer->add_line("return current;");
    printer->pop_block();
    use_present_fp_indexing_ = false;
    use_host_captured_t_ = false;
}

void CodegenNeuronAccVisitor::print_nrn_state() {
    if (!nrn_state_required()) {
        return;
    }

    printer->add_newline(2);
    print_global_function_common_code(BlockType::State);
    if ((info.net_send_used || info.net_event_used) && !info.artificial_cell) {
        printer->add_line(
            "neuron::gpu::net_send_buffer_ensure_for_events(_ml_arg, nodecount);");
    }
    // Host-captured t for any STATE body that reads t (same as nrn_cur).
    printer->add_line("double const _nrn_thread_t = nt->_t;");
    printer->add_line("(void) _nrn_thread_t;");  // unused when STATE does not reference t
    use_host_captured_t_ = true;

    use_present_fp_indexing_ = true;
    print_parallel_iteration_hint(BlockType::State, info.nrn_state_block);
    printer->push_block("for (int id = 0; id < nodecount; id++)");
    printer->add_line("int node_id = node_data.nodeindices[id];");
    printer->add_line("auto* _ppvar = _ml_arg->pdata[id];");
    if (!info.artificial_cell) {
        printer->fmt_line("{} = _d_voltages[node_id];",
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
    use_host_captured_t_ = false;
}

CodegenNeuronAccVisitor::ParamVector CodegenNeuronAccVisitor::nrn_current_parameters() {
    auto params = CodegenNeuronCppVisitor::nrn_current_parameters();
    auto const v_param = params.back();
    params.pop_back();
    for (int i = 0; i < static_cast<int>(codegen_float_variables.size()); ++i) {
        params.emplace_back("", "double*", "", fmt::format("_present_fp_{}", i));
    }
    // Host-captured sim time (see get_variable_name for t under present_fp indexing).
    params.emplace_back("", "double", "", "_nrn_thread_t");
    params.push_back(v_param);
    return params;
}

void CodegenNeuronAccVisitor::print_nrn_cur_kernel(const ast::BreakpointBlock& node) {
    // Use _d_voltages (deviceptr), not present(host node_voltages).
    printer->add_line("int node_id = node_data.nodeindices[id];");
    printer->add_line("double v = _d_voltages[node_id];");
    printer->add_line("auto* _ppvar = _ml_arg->pdata[id];");
    const auto& read_statements = ion_read_statements(BlockType::Equation);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    // Conductance path does not call nrn_current_*; fold BEFORE BREAKPOINT here.
    // Caller already set use_present_fp_indexing_ for the ACC CURRENT loop.
    // Mirror nrn_current_*: store local v into voltage SoA then run BA (uses v).
    if (!info.conductances.empty()) {
        printer->fmt_line("{} = v;", indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
        print_before_breakpoint_inline();
    }

    if (info.conductances.empty()) {
        print_nrn_cur_non_conductance_kernel();
    } else {
        print_nrn_cur_conductance_kernel(node);
    }

    const auto& write_statements = ion_write_statements(BlockType::Equation);
    for (auto& statement: write_statements) {
        auto text = process_shadow_update_statement(statement, BlockType::Equation);
        printer->add_line(text);
    }

    if (info.point_process) {
        const auto& area = get_variable_name(naming::NODE_AREA_VARIABLE);
        printer->fmt_line("double mfactor = 1.e2/{};", area);
        printer->add_line("g = g*mfactor;");
        printer->add_line("rhs = rhs*mfactor;");
    }
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
    // Manual function open: capture host t for ACC CURRENT (IClamp window) as
    // firstprivate; device nt->_t is not used (update device(nt._t) is unsafe).
    {
        ParamVector args = {{"", "const _nrn_model_sorted_token&", "", "_sorted_token"},
                            {"", "NrnThread*", "", "nt"},
                            {"", "Memb_list*", "", "_ml_arg"},
                            {"", "int", "", "_type"}};
        printer->fmt_push_block("static void {}({})",
                                compute_method_name(BlockType::Equation),
                                get_parameter_str(args));
    }
    print_kernel_global_device_setup();
    printer->add_line("auto nodecount = _ml_arg->nodecount;");
    printer->add_line("double const _nrn_thread_t = nt->_t;");
    use_host_captured_t_ = true;
    if ((info.net_send_used || info.net_event_used) && !info.artificial_cell) {
        printer->add_line(
            "neuron::gpu::net_send_buffer_ensure_for_events(_ml_arg, nodecount);");
    }
    print_kernel_data_present_annotation_block_begin();
    print_entrypoint_setup_code_from_memb_list();
    use_present_fp_indexing_ = true;

    // Point processes may share a node. Parallel OpenACC needs atomics (Stage 3c)
    // but atomics are non-associative. NRN_DETERMINISTIC_MATRIX=1: serial instance
    // order (matches CPU left-to-right) for testing / raster stability.
    auto print_cur_loop_body = [&](bool det_matrix) {
        force_seq_acc_loop_ = det_matrix;
        print_parallel_iteration_hint(BlockType::Equation, info.breakpoint_node);
        printer->push_block("for (int id = 0; id < nodecount; id++)");
        print_nrn_cur_kernel(*info.breakpoint_node);
        if (info.point_process && !det_matrix) {
            printer->add_line("nrn_pragma_acc(atomic update)");
            printer->add_line("nrn_pragma_omp(atomic update)");
        }
        printer->fmt_line("vec_rhs[node_id] {} rhs;", operator_for_rhs());
        // ELECTRODE sav_rhs is a post-pass (below) — writing sav inside this ACC
        // region with mechanism present caused CUDA illegal address on hh models.
        if (breakpoint_exist()) {
            printer->fmt_line(
                "{} = g;",
                indexed_fp_var(info.vectorize ? naming::CONDUCTANCE_UNUSED_VARIABLE
                                              : naming::CONDUCTANCE_VARIABLE));
        }
        printer->pop_block();
        force_seq_acc_loop_ = false;
    };

    if (info.point_process) {
        printer->push_block("if (neuron::event_order::matrix_enabled())");
        print_cur_loop_body(true);
        printer->chain_block("else");
        print_cur_loop_body(false);
        printer->pop_block();
    } else {
        print_cur_loop_body(false);
    }

    print_after_nrn_cur_gpu_net_send_flush();

    // ELECTRODE_CURRENT → fast_imem sav_rhs after ACC cur (device i is live).
    // Host: serial apply. Device: pull i + sav, scale on host, push sav
    // (in-ACC sav next to mechanism present hit CUDA illegal address on hh).
    if (info.electrode_current && !info.currents.empty()) {
        auto const i_pos = position_of_float_var(info.currents.front());
        printer->push_block("if (auto* vec_sav_rhs = nt->node_sav_rhs_storage())");
        printer->fmt_line("double* _i_col = _lmc.template fpfield_ptr<{}>();", i_pos);
        printer->add_line("auto const* _ni = node_data.nodeindices;");
        printer->add_line("double* _area = nt->node_area_storage();");
        printer->push_block("if (!nt->compute_gpu)");
        printer->push_block("for (int id = 0; id < nodecount; id++)");
        printer->add_line("int node_id = _ni[id];");
        printer->add_line("double mfac = 1.e2 / _area[node_id];");
        printer->fmt_line("vec_sav_rhs[node_id] {} _i_col[id] * mfac;", operator_for_rhs());
        printer->pop_block();
        printer->chain_block("else");
        // Device: wait for cur, pull i, scale on host, RMW sav on device via memcpy.
        // (In-ACC write of sav next to mechanism present hit CUDA illegal address.)
        printer->add_line("nrn_pragma_acc(wait(nt->stream_id))");
        printer->add_line(
            "double* _d_i = static_cast<double*>(acc_deviceptr(_i_col));");
        printer->add_line(
            "double* _d_sav = static_cast<double*>(acc_deviceptr(vec_sav_rhs));");
        printer->push_block("if (_d_i && _d_sav)");
        printer->add_line(
            "std::vector<double> _host_i(static_cast<std::size_t>(nodecount));");
        printer->add_line(
            "acc_memcpy_from_device(_host_i.data(), _d_i, "
            "static_cast<std::size_t>(nodecount) * sizeof(double));");
        printer->add_line(
            "std::vector<double> _host_sav(static_cast<std::size_t>(nt->end));");
        printer->add_line(
            "acc_memcpy_from_device(_host_sav.data(), _d_sav, "
            "static_cast<std::size_t>(nt->end) * sizeof(double));");
        printer->push_block("for (int id = 0; id < nodecount; id++)");
        printer->add_line("int node_id = _ni[id];");
        printer->add_line("double mfac = 1.e2 / _area[node_id];");
        printer->fmt_line("_host_sav[static_cast<std::size_t>(node_id)] {} _host_i["
                          "static_cast<std::size_t>(id)] * mfac;",
                          operator_for_rhs());
        printer->pop_block();
        printer->add_line(
            "acc_memcpy_to_device(_d_sav, _host_sav.data(), "
            "static_cast<std::size_t>(nt->end) * sizeof(double));");
        printer->pop_block();  // _d_i && _d_sav
        printer->pop_block();  // !compute_gpu / else
        printer->pop_block();  // vec_sav_rhs
    }

    print_kernel_data_present_annotation_block_end();
    use_present_fp_indexing_ = false;
    use_host_captured_t_ = false;
    printer->pop_block();
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
        // GPU full table: index id + storage_offset (see present_dptr declarations).
        // Host dptr_field_ptr path sets _present_dptr_base_*=0 (already offset).
        return fmt::format("(*_present_dptr_{}[id + _present_dptr_base_{}])", position, position);
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
        return fmt::format("(_present_fp_{} + id * {})", position, dimension);
    }
    return fmt::format("_present_fp_{}[id]", position);
}

std::string CodegenNeuronAccVisitor::get_variable_name(const std::string& name,
                                                       bool use_instance) const {
    // Device net_buf_receive: local `t` is nrb->_nrb_t[index] (event delivery time
    // captured at enqueue). After deliver_net_events(), nt->_t is restored to the
    // step start (tsav), so net_send/net_move must use event t — same as CoreNEURON
    // (`t + time_interval`), not nt->_t + delay (that mis-times self-events).
    if (printing_net_buf_receive_kernel_ && name == "t") {
        return "t";
    }
    // ACC CURRENT/STATE only: host-captured _nrn_thread_t (firstprivate). Do not
    // substitute in INITIAL/net_buf paths that never declare that local.
    if (use_host_captured_t_ && use_present_fp_indexing_ &&
        name == naming::NTHREAD_T_VARIABLE) {
        return "_nrn_thread_t";
    }
    return CodegenNeuronCppVisitor::get_variable_name(name, use_instance);
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
                return "";  // RANDOM etc. — not in Instance
            } else if (sem == naming::POINTER_SEMANTIC || sem == naming::RANDOM_SEMANTIC) {
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

    // Host NET_RECEIVE / HOC path: cache_instance (single Prop). Must still fill
    // global/area — incomplete `0` left inst.global null and WATCH+GLOBAL mechs SEGV.
    printer->add_newline(2);
    printer->fmt_push_block("static {} make_instance_{}(_nrn_mechanism_cache_instance* _lmc)",
                            instance_struct(),
                            info.mod_suffix);
    printer->push_block("if(_lmc == nullptr)");
    printer->fmt_line("return {}();", instance_struct());
    printer->pop_block_nl(2);
    printer->fmt_push_block("return {}", instance_struct());
    std::vector<std::string> host_inst_args;
    for (auto const& [var, type]: info.neuron_global_variables) {
        host_inst_args.push_back(fmt::format("&::{0}", var->get_name()));
    }
    host_inst_args.emplace_back("0");  // data_offset
    const auto codegen_int_variables_size_h = codegen_int_variables.size();
    for (size_t i = 0; i < codegen_int_variables_size_h; ++i) {
        const auto& var = codegen_int_variables[i];
        auto sem = info.semantics[i].name;
        if (var.is_index || var.is_integer || var.is_vdata) {
            continue;
        }
        if (sem == naming::POINTER_SEMANTIC || sem == naming::RANDOM_SEMANTIC) {
            continue;
        }
        host_inst_args.push_back(fmt::format("_lmc->template dptr_field_ptr<{}>()", i));
    }
    if (!codegen_global_variables.empty()) {
        host_inst_args.push_back(fmt::format("&{0}", global_struct_instance()));
    }
    printer->add_multi_line(fmt::format("{}", fmt::join(host_inst_args, ",\n")));
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

void CodegenNeuronAccVisitor::visit_watch_statement(const ast::WatchStatement& node) {
    // Device net_buf_receive cannot arm host WatchCondition. WATCH mechs use
    // host NET_RECEIVE (see print_net_receive); skip WATCH in the device kernel.
    if (printing_net_buf_receive_kernel_) {
        return;
    }
    CodegenNeuronCppVisitor::visit_watch_statement(node);
}

}  // namespace codegen
}  // namespace nmodl