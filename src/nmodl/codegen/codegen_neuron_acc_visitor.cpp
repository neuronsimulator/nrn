#include "codegen/codegen_neuron_acc_visitor.hpp"

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
}

void CodegenNeuronAccVisitor::print_parallel_iteration_hint(BlockType type,
                                                            const ast::Block* block) {
    if (info.artificial_cell) {
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

    std::ostringstream present_clause;
    present_clause << "present(ml, nt";
    if (type == BlockType::NetReceive) {
        present_clause << ", nrb";
    } else {
        present_clause << ", nodeindices, thread";
        if (type == BlockType::Equation) {
            present_clause << ", vec_rhs, vec_d";
        }
    }
    present_clause << ')';

    printer->fmt_line("nrn_pragma_acc(parallel loop {} async(nt->stream_id) if(nt->compute_gpu))",
                      present_clause.str());
    printer->add_line("nrn_pragma_omp(target teams distribute parallel for if(nt->compute_gpu))");
}

void CodegenNeuronAccVisitor::print_kernel_data_present_annotation_block_begin() {
    if (!info.artificial_cell) {
        printer->add_line("nrn_pragma_acc(data present(nt, ml) if(nt->compute_gpu))");
        printer->add_line("{");
        printer->increase_indent();
    }
}

void CodegenNeuronAccVisitor::print_kernel_data_present_annotation_block_end() {
    if (!info.artificial_cell) {
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
    printer->add_line("neuron::gpu::NetSendBuffer_t* nsb = ml->_net_send_buffer;");
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

void CodegenNeuronAccVisitor::print_net_send_call(const ast::FunctionCall& node) {
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
            "nt, ml, ml->_net_send_buffer, 0, (intptr_t)&{}, {}, "
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
        "nt, ml, ml->_net_send_buffer, 2, (intptr_t)&{}, (intptr_t)-1, "
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
        "nt, ml, ml->_net_send_buffer, 1, (intptr_t)-1, (intptr_t)-1, "
        "(intptr_t){}, ",
        point_process);
    print_vector_elements(arguments, ", ");
    printer->add_text(", 0.0, 0.0");
    printer->add_text(")");
}

}  // namespace codegen
}  // namespace nmodl