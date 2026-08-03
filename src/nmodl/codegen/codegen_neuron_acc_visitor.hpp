#pragma once

/**
 * \file
 * \brief \copybrief nmodl::codegen::CodegenNeuronAccVisitor
 */

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

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

    /** Thin specialized procedure call from STATE (rates_*_state). */
    void print_function_call(const ast::FunctionCall& node) override;

    /** Skip dead node_data V load in state-specialized procedure bodies. */
    void print_function_or_procedure(
        const ast::Block& node,
        const std::string& name,
        const std::unordered_set<CppObjectSpecifier>& specifiers = {
            CppObjectSpecifier::Inline}) override;

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
    void print_nrn_cur_non_conductance_kernel() override;
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
     * Session A: emitting rates_*_state (or f_rates_*_state) thin ABI — only
     * inst + live double& TABLE temps + live present_fp columns + MOD args.
     */
    mutable bool emitting_state_specialized_procedure_{false};

    /**
     * Session A residual: force-inlining a unique/safe STATE specialized
     * procedure body into the STATE loop (hand-edit shape). Prefer present
     * `hh_global` / bare celsius over `inst.global` / `*(inst.celsius)`.
     */
    mutable bool inlining_state_specialized_body_{false};

    /**
     * Session B: force-inlining BREAKPOINT body into CURRENT (numerical di/dv
     * non-conductance path). Prefer stack temps + local v (hand-edit shape).
     */
    mutable bool inlining_current_body_{false};

    /** Session B: CURRENT uses stack `_cur_v` instead of v_unused SoA. */
    mutable bool current_local_v_active_{false};

    /**
     * Session B: intermediate ASSIGNED / ion shadows are stack locals on the
     * force-inlined CURRENT path (not SoA present).
     */
    mutable bool current_stack_temps_active_{false};

    /**
     * Live non-table present_fp indices for the procedure currently being
     * specialized (feeds internal_method_parameters while emitting).
     */
    mutable std::optional<std::unordered_set<int>> state_specialized_live_fp_;

    /**
     * TABLE temp names (minf, mtau, …) for the procedure currently being
     * specialized — double& params in thin ABI order.
     */
    mutable std::vector<std::string> state_specialized_table_temps_;

    /**
     * MOD procedure names for which a *_state specialized version was emitted.
     * STATE call sites force-inline the body when the name is in this set.
     */
    std::unordered_set<std::string> state_specialized_procedures_;

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

    /**
     * Safety gate for state-specialized procedure versions: no VERBATIM, no
     * net_send/move/event, no ion/POINTER/RANDOM RANGE uses, no nested MOD
     * procedure/function calls (after InlineVisitor those are rare).
     */
    [[nodiscard]] bool procedure_safe_for_state_specialization(const ast::Block& node) const;

    /** True if STATE / matexp path has a call to \p proc_name. */
    [[nodiscard]] bool procedure_called_from_state(const std::string& proc_name) const;

    /** Non-table float SoA indices the procedure body actually touches. */
    [[nodiscard]] std::unordered_set<int> procedure_live_present_fp_indices(
        const ast::Block& node) const;

    /** TABLE statement temp names used by the procedure body. */
    [[nodiscard]] std::vector<std::string> procedure_table_temp_names(
        const ast::Block& node) const;

    /** Thin internal params for *_state specialized versions of \p node. */
    [[nodiscard]] ParamVector state_specialized_method_parameters(const ast::Block& node) const;

    /** Thin internal args for STATE call of specialized \p proc_name. */
    [[nodiscard]] std::string state_specialized_method_arguments(
        const std::string& proc_name) const;

    /** Emit f_*_hh_state / rates_*_hh_state after the general versions. */
    void print_state_specialized_procedure_versions(const ast::Block& node);

    /** Fully-mangled declaration: method_name(base) + "_state". */
    void print_state_specialized_function_declaration(
        const ast::Block& node,
        const std::string& cpp_method_name,
        const std::unordered_set<CppObjectSpecifier>& specifiers);

    /** Analytic / non-TABLE specialized procedure body. */
    void print_state_specialized_function_or_procedure(const ast::Block& node,
                                                       const std::string& cpp_method_name);

    /** TABLE-aware rates_*_hh_state body (thin ABI). */
    void print_state_specialized_table_replacement(const ast::Block& node);

    /**
     * Force-inline unique/safe specialized procedure body at a STATE call site
     * (TABLE path when applicable). No device call; writes stack TABLE temps.
     */
    void print_state_specialized_call_force_inlined(const ast::FunctionCall& node);

    /** TABLE interpolation body without returns (for force-inline into STATE). */
    void print_inlined_table_procedure_body(const ast::Block& node,
                                            const std::string& formal_v_name);

    /** True when every defined MOD call from STATE has a thin specialized version. */
    [[nodiscard]] bool state_kernel_uses_only_specialized_procedures() const;

    /**
     * Session B: safe to force-inline BREAKPOINT into CURRENT (no VERBATIM,
     * net_send/move/event, POINTER/RANDOM). Ion dptrs are allowed (unlike STATE).
     */
    [[nodiscard]] bool current_force_inline_safe() const;

    /**
     * Session B: float SoA column is a CURRENT stack temp (ASSIGNED intermediate,
     * ion shadow, nonspecific current) — not PARAMETER/STATE/g_unused.
     */
    [[nodiscard]] bool is_current_stack_temp_float(const std::string& name) const;

    /** Stack temp names used by BREAKPOINT (+ BA) on force-inline CURRENT. */
    [[nodiscard]] std::vector<std::string> current_stack_temp_names() const;

    /** Emit one inlined BREAKPOINT evaluation at voltage expression \p v_expr. */
    void print_inlined_nrn_current_at_v(const std::string& v_expr);

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
