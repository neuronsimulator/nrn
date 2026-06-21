#pragma once

/**
 * \file
 * \brief \copybrief nmodl::codegen::CodegenNeuronAccVisitor
 */

#include "codegen/codegen_neuron_cpp_visitor.hpp"

namespace nmodl {
namespace codegen {

/**
 * \addtogroup codegen_backends
 * \{
 */

/**
 * \class CodegenNeuronAccVisitor
 * \brief OpenACC backend for NEURON mechanism codegen (native GPU adoption).
 *
 * Field mapping follows design §B.4: NEURON sorted SOA pointers and NrnThread::compute_gpu.
 */
class CodegenNeuronAccVisitor: public CodegenNeuronCppVisitor {
  public:
    using CodegenNeuronCppVisitor::CodegenNeuronCppVisitor;

  protected:
    std::string backend_name() const override;

    void print_standard_includes() override;

    void print_parallel_iteration_hint(BlockType type, const ast::Block* block) override;

    void print_kernel_data_present_annotation_block_begin() override;
    void print_kernel_data_present_annotation_block_end() override;

    void print_after_nrn_cur_gpu_net_send_flush() override;

    void print_net_send_call(const ast::FunctionCall& node) override;
    void print_net_move_call(const ast::FunctionCall& node) override;
    void print_net_event_call(const ast::FunctionCall& node) override;

    void print_compute_functions() override;

  private:
    void print_device_stream_wait() const;
    void print_net_send_buffering();
    void print_send_event_move();
    void print_net_send_buffering_cnt_update() const;
    void print_net_send_buffering_grow();
    void print_net_send_buf_count_update_to_host() const;
    void print_net_send_buf_update_to_host() const;
    void print_net_send_buf_count_update_to_device() const;
};

/** \} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl
