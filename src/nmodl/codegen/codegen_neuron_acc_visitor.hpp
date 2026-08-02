#pragma once

/**
 * \file
 * \brief \copybrief nmodl::codegen::CodegenNeuronAccVisitor
 */

#include <optional>
#include <string>
#include <unordered_set>

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

    /** When true, ACC kernel body may use firstprivate `_nrn_thread_t` (must be declared). */
    bool use_host_captured_t_ = false;

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

    std::string internal_method_arguments() override;

    /** Functors need full RANGE present_fp set (deriv body ≠ procedure live set). */
    ParamVector functor_params() override;

    void print_function_procedure_helper(const ast::Block& node) override;

    void print_gpu_phase_registration() override;

    /** Stage 2: enqueue-only GPU pnt_receive + device net_buf_receive. */
    void print_net_receive() override;
    void print_net_receive_registration() override;

    std::string global_variable_name(const SymbolType& symbol,
                                     bool use_instance) const override;

    void print_kernel_global_device_setup() override;

    void print_kernel_instance_data_copyin() override;

    void print_global_var_struct_decl() override;

    void print_after_host_table_rebuild() override;

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

    void visit_watch_statement(const ast::WatchStatement& node) override;

  private:
    /** When true, RANGE reads use _present_fp_* (OpenACC kernel); else _lmc.fpfield (HOC/net_receive). */
    mutable bool use_present_fp_indexing_{false};

    /** True while emitting device net_buf_receive body (net_send → buffering, not host net_send). */
    mutable bool printing_net_buf_receive_kernel_{false};

    /**
     * H4c: TABLE / rates ASSIGNED temps as double& (_kl_*) in procedure bodies.
     * False while printing table-update host glue (reads SoA after f_rates).
     */
    mutable bool use_kl_ref_in_float_name_{false};

    /**
     * H4c: STATE loop has stack locals for table_statement_variables; call sites
     * pass those names instead of _present_fp_i[id].
     */
    mutable bool state_kernel_locals_active_{false};

    /**
     * H4c: HOC/Python wrappers bind TABLE temps as _present_fp_i[id] into double&.
     * Procedure bodies (incl. table update) pass _kl_* params through.
     */
    mutable bool hoc_wrapper_table_temp_as_soa_{false};

    /** H4c: STATE uses stack `v` instead of writing v_unused SoA. */
    mutable bool state_local_v_active_{false};

    /**
     * Optional live SoA column set for the next parallel loop (e.g. jacob only
     * needs g_unused). When nullopt, compute from BlockType AST usage.
     */
    mutable std::optional<std::unordered_set<int>> live_float_indices_override_;

    [[nodiscard]] std::string indexed_fp_var(std::string_view name,
                                             std::string_view index_expr = "id") const;
    [[nodiscard]] int conductance_fp_index() const;

    /** TABLE statement vars (minf, mtau, …) — pure temps on STATE hot path. */
    [[nodiscard]] bool is_table_statement_float(const std::string& name) const;

    /** Float SoA indices that the kernel must present (named _present_fp_N). */
    [[nodiscard]] std::unordered_set<int> live_float_indices_for_kernel(BlockType type) const;

    /** Ion dptr indices used on this kernel path (empty → no deviceptr ions). */
    [[nodiscard]] std::unordered_set<int> live_dptr_indices_for_kernel(BlockType type);


    void collect_ast_names(const ast::Ast& node, std::unordered_set<std::string>& names) const;

    void print_present_fp_pointer_declarations() const;
    void print_present_fp_pointer_declarations_for(const std::unordered_set<int>& indices) const;
    void print_present_dptr_pointer_declarations() const;
    void print_present_dptr_pointer_declarations_for(const std::unordered_set<int>& indices) const;
    [[nodiscard]] std::string present_dptr_deviceptr_clause() const;
    [[nodiscard]] std::string present_dptr_deviceptr_clause_for(
        const std::unordered_set<int>& indices) const;

    /** Inline BEFORE BREAKPOINT bodies into CURRENT (device path; no separate BA). */
    void print_before_breakpoint_inline();

    bool host_only_parallel_block(BlockType type) const;
    void print_global_variable_enter_data_once() const;
    void print_global_variable_device_update_annotation() const;
    /** After host mutates mech globals (e.g. TABLE rebuild), mark for next H→D. */
    void print_mark_global_device_stale() const;
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
