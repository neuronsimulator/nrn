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

    /** When true, ACC cur/jacob PP loops use `loop seq` (NRN_DETERMINISTIC_MATRIX). */
    bool force_seq_acc_loop_ = false;

    void print_kernel_data_present_annotation_block_begin() override;
    void print_kernel_data_present_annotation_block_end() override;

    void print_after_nrn_cur_gpu_net_send_flush() override;

    void print_net_send_call(const ast::FunctionCall& node) override;
    void print_net_move_call(const ast::FunctionCall& node) override;
    void print_net_event_call(const ast::FunctionCall& node) override;

    void print_compute_functions() override;

    void print_function_definitions() override;

    /** Eigen Newton functors must use _present_fp_* (same as kernel body), not _lmc. */
    void print_functors_definitions() override;

    void print_hoc_py_wrapper_before_table_update() override;

    void print_check_table_entrypoint() override;

    ParamVector internal_method_parameters() override;

    void print_gpu_phase_registration() override;

    /** Stage 2: enqueue-only GPU pnt_receive + device net_buf_receive. */
    void print_net_receive() override;
    void print_net_receive_registration() override;

    std::string global_variable_name(const SymbolType& symbol,
                                     bool use_instance) const override;

    void print_kernel_global_device_setup() override;

    void print_kernel_instance_data_copyin() override;

    void print_global_var_struct_decl() override;

    void print_data_structures(bool print_initializers) override;

    void print_mechanism_range_var_structure(bool print_initializers) override;

    std::string float_variable_name(const SymbolType& symbol, bool use_instance) const override;

    /** In device net_buf_receive, NMODL t is the event time (nrb->_nrb_t), not nt->_t. */
    std::string get_variable_name(const std::string& name, bool use_instance = true) const override;

    std::string int_variable_name(const IndexVariableInfo& symbol,
                                  const std::string& name,
                                  bool use_instance) const override;

    void print_v_unused() const override;
    void print_g_unused() const override;

    void print_nrn_init(bool skip_init_check = true) override;
    void print_nrn_current(const ast::BreakpointBlock& node) override;
    void print_nrn_state() override;
    void print_nrn_cur() override;
    void print_nrn_cur_kernel(const ast::BreakpointBlock& node) override;
    void print_nrn_jacob() override;

    void print_entrypoint_setup_code_from_prop() override;

    ParamVector nrn_current_parameters() override;

    void print_make_instance() const override;

    void print_make_node_data() const override;

    void print_entrypoint_setup_code_from_memb_list() override;

  private:
    /** When true, RANGE reads use _present_fp_* (OpenACC kernel); else _lmc.fpfield (HOC/net_receive). */
    mutable bool use_present_fp_indexing_{false};

    /** True while emitting device net_buf_receive body (net_send → buffering, not host net_send). */
    mutable bool printing_net_buf_receive_kernel_{false};

    [[nodiscard]] std::string indexed_fp_var(std::string_view name,
                                             std::string_view index_expr = "id") const;
    [[nodiscard]] int conductance_fp_index() const;

    void print_present_fp_pointer_declarations() const;
    void print_present_dptr_pointer_declarations() const;
    [[nodiscard]] std::string present_dptr_deviceptr_clause() const;

    bool host_only_parallel_block(BlockType type) const;
    void print_global_variable_enter_data_once() const;
    void print_global_variable_device_update_annotation() const;
    void print_device_stream_wait() const;
    void print_net_send_buffering();
    void print_send_event_move();
    void print_net_send_buffering_cnt_update() const;
    void print_net_send_buffering_grow();
    void print_net_send_buf_count_update_to_host() const;
    void print_net_send_buf_update_to_host() const;
    void print_net_send_buf_count_update_to_device() const;

    void print_net_receive_buffering();
};

/** \} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl
