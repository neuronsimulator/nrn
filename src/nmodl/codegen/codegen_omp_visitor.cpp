/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "codegen/codegen_omp_visitor.hpp"


namespace nmodl {
namespace codegen {


/****************************************************************************************/
/*                      Routines must be overloaded in backend                          */
/****************************************************************************************/


void CodegenOmpVisitor::print_channel_iteration_task_begin(BlockType type) {
    std::string vars;
    if (type == BlockType::Equation) {
        vars = "start, end, node_index, indexes, voltage, vec_rhs, vec_d, inst, thread, nt";
    } else {
        vars = "start, end, node_index, indexes, voltage, inst, thread, nt";
    }
    printer->add_line(fmt::format("#pragma omp task default(shared) firstprivate({})", vars));
    printer->add_line("{");
    printer->increase_indent();
}


void CodegenOmpVisitor::print_channel_iteration_task_end() {
    printer->decrease_indent();
    printer->add_line("}");
}


/*
 * Depending on the backend, print loop for tiling channel iterations
 */
void CodegenOmpVisitor::print_channel_iteration_tiling_block_begin(BlockType type) {
    printer->add_line("const int TILE = 3;");
    printer->start_block("for (int block = 0; block < nodecount;) ");
    printer->add_line("int start = block;");
    printer->add_line("block = (block+TILE) < nodecount ? (block+TILE) : nodecount;");
    printer->add_line("int end = block;");
    print_channel_iteration_task_begin(type);
}


/**
 * End of tiled channel iteration block
 */
void CodegenOmpVisitor::print_channel_iteration_tiling_block_end() {
    print_channel_iteration_task_end();
    printer->end_block();
    printer->add_newline();
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
void CodegenOmpVisitor::print_channel_iteration_block_parallel_hint(BlockType type) {
    printer->add_line("#pragma omp simd");
}


void CodegenOmpVisitor::print_atomic_reduction_pragma() {
    printer->add_line("#pragma omp atomic update");
}


void CodegenOmpVisitor::print_backend_includes() {
    printer->add_line("#include <omp.h>");
}


std::string CodegenOmpVisitor::backend_name() const {
    return "C-OpenMP (api-compatibility)";
}


bool CodegenOmpVisitor::channel_task_dependency_enabled() {
    return true;
}


bool CodegenOmpVisitor::block_require_shadow_update(BlockType type) {
    return !(!channel_task_dependency_enabled() || type == BlockType::Initial);
}

}  // namespace codegen
}  // namespace nmodl
