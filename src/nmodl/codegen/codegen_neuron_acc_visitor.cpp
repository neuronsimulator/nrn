/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codegen/codegen_neuron_acc_visitor.hpp"

#include "ast/block.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace codegen {

std::string CodegenNeuronAccVisitor::backend_name() const {
    return "C++-OpenAcc-NEURON";
}

void CodegenNeuronAccVisitor::print_standard_includes() {
    CodegenNeuronCppVisitor::print_standard_includes();
    printer->add_line("#include <neuron/gpu/offload.hpp>");
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
    printer->add_line("nrn_pragma_acc(wait(nt->stream_id))");
}

}  // namespace codegen
}  // namespace nmodl