/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "codegen/codegen_acc_visitor.hpp"


using namespace fmt::literals;


namespace nmodl {
namespace codegen {

/****************************************************************************************/
/*                      Routines must be overloaded in backend                          */
/****************************************************************************************/


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
void CodegenAccVisitor::print_channel_iteration_block_parallel_hint(BlockType type) {
    if (info.artificial_cell) {
        return;
    }

    std::ostringstream present_clause;
    present_clause << "present(inst";

    if (type == BlockType::NetReceive) {
        present_clause << ", nrb";
    } else {
        present_clause << ", node_index, data, voltage, indexes, thread";
        if (type == BlockType::Equation) {
            present_clause << ", vec_rhs, vec_d";
        }
    }
    present_clause << ')';
    printer->add_line(
        "#pragma acc parallel loop {} async(nt->stream_id) if(nt->compute_gpu)"_format(
            present_clause.str()));
}


void CodegenAccVisitor::print_atomic_reduction_pragma() {
    if (!info.artificial_cell) {
        printer->add_line("#pragma acc atomic update");
    }
}


void CodegenAccVisitor::print_backend_includes() {
    /**
     * Artificial cells are executed on CPU. As Random123 is allocated on GPU by default,
     * we have to disable GPU allocations using `DISABLE_OPENACC` macro.
     */
    if (info.artificial_cell) {
        printer->add_line("#undef DISABLE_OPENACC");
        printer->add_line("#define DISABLE_OPENACC");
    } else {
        printer->add_line("#include <cuda.h>");
        printer->add_line("#include <cuda_runtime_api.h>");
        printer->add_line("#include <openacc.h>");
    }
}


std::string CodegenAccVisitor::backend_name() const {
    return "C-OpenAcc (api-compatibility)";
}


void CodegenAccVisitor::print_memory_allocation_routine() const {
    // memory for artificial cells should be allocated on CPU
    if (info.artificial_cell) {
        CodegenCVisitor::print_memory_allocation_routine();
        return;
    }
    printer->add_newline(2);
    auto args = "size_t num, size_t size, size_t alignment = 16";
    printer->add_line("static inline void* mem_alloc({}) {}"_format(args, "{"));
    printer->add_line("    void* ptr;");
    printer->add_line("    cudaMallocManaged(&ptr, num*size);");
    printer->add_line("    cudaMemset(ptr, 0, num*size);");
    printer->add_line("    return ptr;");
    printer->add_line("}");

    printer->add_newline(2);
    printer->add_line("static inline void mem_free(void* ptr) {");
    printer->add_line("    cudaFree(ptr);");
    printer->add_line("}");
}

/**
 * OpenACC kernels running on GPU doesn't support `abort()`. CUDA/OpenACC supports
 * `assert()` in device kernel that can be used for similar purpose. Also, `printf`
 * is supported on device.
 *
 * @todo : we need to implement proper error handling mechanism to propogate errors
 *         from GPU to CPU. For example, error code can be returned like original
 *         neuron implementation. For now we use `assert(0==1)` pattern which is
 *         used for OpenACC/CUDA.
 */
void CodegenAccVisitor::print_abort_routine() const {
    printer->add_newline(2);
    printer->add_line("static inline void coreneuron_abort() {");
    printer->add_line("    printf(\"Error : Issue while running OpenACC kernel \\n\");");
    printer->add_line("    assert(0==1);");
    printer->add_line("}");
}

void CodegenAccVisitor::print_net_send_buffering_grow() {
    auto error = add_escape_quote("Error : netsend buffer size (%d) exceeded\\n");

    printer->add_line("printf({}, nsb->_cnt);"_format(error));
    printer->add_line("coreneuron_abort();");
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
 * \endcode
 */
void CodegenAccVisitor::print_kernel_data_present_annotation_block_begin() {
    if (!info.artificial_cell) {
        auto global_variable = "{}_global"_format(info.mod_suffix);
        printer->add_line(
            "#pragma acc data present(nt, ml, {}) if(nt->compute_gpu)"_format(global_variable));
        printer->add_line("{");
        printer->increase_indent();
    }
}


void CodegenAccVisitor::print_nrn_cur_matrix_shadow_update() {
    auto rhs_op = operator_for_rhs();
    auto d_op = operator_for_d();
    print_atomic_reduction_pragma();
    printer->add_line("vec_rhs[node_id] {} rhs;"_format(rhs_op));
    print_atomic_reduction_pragma();
    printer->add_line("vec_d[node_id] {} g;"_format(d_op));
}

void CodegenAccVisitor::print_fast_imem_calculation() {
    if (!info.electrode_current) {
        return;
    }

    auto rhs_op = operator_for_rhs();
    auto d_op = operator_for_d();
    printer->start_block("if (nt->nrn_fast_imem)");
    print_atomic_reduction_pragma();
    printer->add_line("nt->nrn_fast_imem->nrn_sav_rhs[node_id] {} rhs;"_format(rhs_op));
    print_atomic_reduction_pragma();
    printer->add_line("nt->nrn_fast_imem->nrn_sav_d[node_id] {} g;"_format(d_op));
    printer->end_block(1);
}

void CodegenAccVisitor::print_nrn_cur_matrix_shadow_reduction() {
    // do nothing
}


/**
 * End of print_kernel_enter_data_begin
 */
void CodegenAccVisitor::print_kernel_data_present_annotation_block_end() {
    if (!info.artificial_cell) {
        printer->decrease_indent();
        printer->add_line("}");
    }
}


void CodegenAccVisitor::print_rhs_d_shadow_variables() {
    // do nothing
}


bool CodegenAccVisitor::nrn_cur_reduction_loop_required() {
    return false;
}


void CodegenAccVisitor::print_global_variable_device_create_annotation() {
    if (!info.artificial_cell) {
        printer->add_line("#pragma acc declare create ({}_global)"_format(info.mod_suffix));
    }
}


void CodegenAccVisitor::print_global_variable_device_update_annotation() {
    if (!info.artificial_cell) {
        printer->add_line("#pragma acc update device ({}_global)"_format(info.mod_suffix));
    }
}


std::string CodegenAccVisitor::get_variable_device_pointer(const std::string& variable,
                                                           const std::string& type) const {
    if (info.artificial_cell) {
        return variable;
    }
    return "reinterpret_cast<{}>( nt->compute_gpu ? acc_deviceptr({}) : {} )"_format(type,
                                                                                     variable,
                                                                                     variable);
}


void CodegenAccVisitor::print_newtonspace_transfer_to_device() const {
    int list_num = info.derivimplicit_list_num;
    printer->add_line("if (nt->compute_gpu) {");
    printer->add_line("    auto device_vec = static_cast<double*>(acc_copyin(vec, vec_size));");
    printer->add_line("    auto device_ns = static_cast<NewtonSpace*>(acc_deviceptr(*ns));");
    printer->add_line("    auto device_thread = static_cast<ThreadDatum*>(acc_deviceptr(thread));");
    printer->add_line(
        "    acc_memcpy_to_device(&(device_thread[{}]._pvoid), &device_ns, sizeof(void*));"_format(
            info.thread_data_index - 1));
    printer->add_line(
        "    acc_memcpy_to_device(&(device_thread[dith{}()].pval), &device_vec, sizeof(double*));"_format(
            list_num));
    printer->add_line("}");
}


void CodegenAccVisitor::print_instance_variable_transfer_to_device() const {
    printer->add_line("if (nt->compute_gpu) {");
    printer->add_line("    auto dml = (Memb_list*) acc_deviceptr(ml);");
    printer->add_line("    acc_memcpy_to_device(&(dml->instance), &inst, sizeof(void*));");
    printer->add_line("}");
}


void CodegenAccVisitor::print_deriv_advance_flag_transfer_to_device() const {
    printer->add_line("#pragma acc update device (deriv_advance_flag) if (nt->compute_gpu)");
}


void CodegenAccVisitor::print_device_atomic_capture_annotation() const {
    printer->add_line("#pragma acc atomic capture");
}


void CodegenAccVisitor::print_device_stream_wait() const {
    printer->add_line("#pragma acc wait(nt->stream_id)");
}


void CodegenAccVisitor::print_net_send_buf_count_update_to_host() const {
    print_device_stream_wait();
    printer->add_line("#pragma acc update self(nsb->_cnt) if(nt->compute_gpu)");
}


void CodegenAccVisitor::print_net_send_buf_count_update_to_device() const {
    printer->add_line("#pragma acc update device(nsb->_cnt) if (nt->compute_gpu)");
}

}  // namespace codegen
}  // namespace nmodl
