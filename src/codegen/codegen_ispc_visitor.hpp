/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::codegen::CodegenIspcVisitor
 */

#include "codegen/codegen_c_visitor.hpp"


namespace nmodl {
namespace codegen {

/**
 * @addtogroup codegen_backends
 * @{
 */

/**
 * \class CodegenIspcVisitor
 * \brief %Visitor for printing C code with ISPC backend
 */
class CodegenIspcVisitor: public CodegenCVisitor {
    /**
     * Prints an ISPC atomic operation
     *
     * \note This is currently not used because of performance issues on KNL. Instead reduction
     * operations are serialized.
     *
     * \param lhs Reduction left-hand-side operand
     * \param op  Reducation operation. Currently only \c += and \c -= are supported
     * \param rhs Reduction right-hand-side operand
     */
    void print_atomic_op(const std::string& lhs, const std::string& op, const std::string& rhs);


    /// ast nodes which are not compatible with ISPC target
    static const std::vector<ast::AstNodeType> incompatible_node_types;

    static const std::unordered_set<std::string> incompatible_var_names;

    /// flag to indicate if visitor should print the the wrapper code
    bool wrapper_codegen = false;

    /// fallback C code generator used to emit C code in the wrapper when emitting ISPC is not
    /// supported
    CodegenCVisitor fallback_codegen;

    std::vector<bool> emit_fallback =
        std::vector<bool>(static_cast<size_t>(BlockType::BlockTypeEnd), false);

    std::vector<const ast::ProcedureBlock*> wrapper_procedures;
    std::vector<const ast::FunctionBlock*> wrapper_functions;

  protected:
    /**
     * Convert a given \c double value to its string representation
     * \param value The number to convert given as string as it parsed by the modfile
     * \return      Its string representation in ISPC compliant format
     */
    std::string format_double_string(const std::string& value) override;


    /**
     * Convert a given \c float value to its string representation
     * \param value The number to convert given as string as it parsed by the modfile
     * \return      Its string representation in ISPC compliant format
     */
    std::string format_float_string(const std::string& value) override;


    /// name of the code generation backend
    std::string backend_name() const override;


    /// return name of main compute kernels
    std::string compute_method_name(BlockType type) const override;


    std::string net_receive_buffering_declaration() override;


    std::string ptr_type_qualifier() override;

    std::string global_var_struct_type_qualifier() override;

    void print_global_var_struct_decl() override;

    std::string param_type_qualifier() override;


    std::string param_ptr_qualifier() override;


    /// common includes : standard c/c++, coreneuron and backend specific
    void print_backend_includes() override;


    /// reduction to matrix elements from shadow vectors
    void print_nrn_cur_matrix_shadow_reduction() override;

    /// fast membrane current calculation

    /**
     * Print block / loop for statement requiring reduction
     *
     */
    void print_shadow_reduction_block_begin() override;

    /**
     * Print end of block / loop for statement requiring reduction
     *
     */
    void print_shadow_reduction_block_end() override;

    /// setup method for setting matrix shadow vectors
    void print_rhs_d_shadow_variables() override;


    /// if reduction block in nrn_cur required
    bool nrn_cur_reduction_loop_required() override;


    ParamVector get_global_function_parms(const std::string& arg_qualifier);


    void print_global_function_common_code(BlockType type) override;


    /// backend specific channel instance iteration block start
    void print_channel_iteration_block_begin(BlockType type) override;


    /// backend specific channel iteration bounds
    void print_channel_iteration_tiling_block_begin(BlockType type) override;


    /// backend specific channel instance iteration block end
    void print_channel_iteration_block_end() override;


    /// start of backend namespace
    void print_backend_namespace_start() override;


    /// end of backend namespace
    void print_backend_namespace_stop() override;


    void print_headers_include() override;


    void print_wrapper_headers_include();


    /// nmodl procedure definition
    void print_procedure(const ast::ProcedureBlock& node) override;


    void print_backend_compute_routine_decl();


    /// print wrapper function that calls ispc kernel
    void print_wrapper_routine(const std::string& wrapper_function, BlockType type);


    /// print initial equation and state wrapper
    void print_block_wrappers_initial_equation_state();


    void print_ispc_globals();


    void print_get_memb_list() override;


    void print_net_receive_loop_begin() override;


    void print_net_receive_buffering_wrapper();


    void print_ion_var_constructor(const std::vector<std::string>& members) override;


    void print_ion_variable() override;


    /// find out for main compute routines whether they are suitable to be emitted in ISPC backend
    bool check_incompatibilities();

    /// check incompatible name var
    template <class T>
    bool check_incompatible_var_name(const std::vector<T>& vec,
                                     const std::string& get_name(const T&)) {
        for (const auto& var: vec) {
            if (incompatible_var_names.count(get_name(var))) {
                return true;
            }
        }
        return false;
    }

    /// move procedures and functions unused by compute kernels into the wrapper
    void move_procs_to_wrapper();


    /// entry point to code generation
    void print_codegen_routines() override;

    void print_wrapper_routines() override;

  public:
    CodegenIspcVisitor(const std::string& mod_file,
                       const std::string& output_dir,
                       const std::string& float_type,
                       const bool optimize_ionvar_copies)
        : CodegenCVisitor(mod_file, output_dir, float_type, optimize_ionvar_copies, ".ispc", ".cpp")
        , fallback_codegen(mod_file, float_type, optimize_ionvar_copies, wrapper_printer) {}


    CodegenIspcVisitor(const std::string& mod_file,
                       std::ostream& stream,
                       const std::string& float_type,
                       const bool optimize_ionvar_copies)
        : CodegenCVisitor(mod_file, stream, float_type, optimize_ionvar_copies, ".ispc", ".cpp")
        , fallback_codegen(mod_file, float_type, optimize_ionvar_copies, wrapper_printer) {}

    void visit_function_call(const ast::FunctionCall& node) override;
    void visit_var_name(const ast::VarName& node) override;
    void visit_program(const ast::Program& node) override;
    void visit_local_list_statement(const ast::LocalListStatement& node) override;

    /// all compute functions for every backend
    void print_compute_functions() override;

    void print_nmodl_constants() override;
};

/** @} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl
