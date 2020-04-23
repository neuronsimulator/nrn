/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <fmt/format.h>

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

    std::string present_clause = "present(inst";

    if (type == BlockType::NetReceive) {
        present_clause += ", nrb";
    } else {
        present_clause += ", node_index, data, voltage, indexes, thread";
        if (type == BlockType::Equation) {
            present_clause += ", vec_rhs, vec_d";
        }
    }
    present_clause += ")";
    printer->add_line("#pragma acc parallel loop {} async(nt->stream_id)"_format(present_clause));
}


void CodegenAccVisitor::print_atomic_reduction_pragma() {
    if (!info.artificial_cell) {
        printer->add_line("#pragma acc atomic update");
    }
}


void CodegenAccVisitor::print_backend_includes() {
    printer->add_line("#include <cuda.h>");
    printer->add_line("#include <cuda_runtime_api.h>");
    printer->add_line("#include <openacc.h>");
}


std::string CodegenAccVisitor::backend_name() const {
    return "C-OpenAcc (api-compatibility)";
}


void CodegenAccVisitor::print_memory_allocation_routine() const {
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
        printer->add_line("#pragma acc data present(nt, ml, {})"_format(global_variable));
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
    return "({}) acc_deviceptr({})"_format(type, variable);
}

}  // namespace codegen
}  // namespace nmodl
