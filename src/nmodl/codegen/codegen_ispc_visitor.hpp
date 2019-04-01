/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include "codegen/codegen_c_visitor.hpp"


namespace nmodl {
namespace codegen {

/**
 * \class CodegenIspcVisitor
 * \brief Visitor for printing ispc backend
 */
class CodegenIspcVisitor: public CodegenCVisitor {
    void print_atomic_op(const std::string& lhs, const std::string& op, const std::string& rhs);

    /// ast nodes which are not compatible with ISPC target
    const std::vector<ast::AstNodeType> incompatible_node_types{
        ast::AstNodeType::VERBATIM, ast::AstNodeType::EIGEN_NEWTON_SOLVER_BLOCK,
        ast::AstNodeType::EIGEN_LINEAR_SOLVER_BLOCK, ast::AstNodeType::WATCH_STATEMENT,
        ast::AstNodeType::TABLE_STATEMENT};

    /// flag to indicate if visitor should print the the wrapper code
    bool wrapper_codegen = false;

    /// fallback C code generator used to emit C code in the wrapper when emitting ISPC is not
    /// supported
    CodegenCVisitor fallback_codegen;

    std::map<BlockType, bool> emit_fallback;

    std::vector<ast::ProcedureBlock*> wrapper_procedures;
    std::vector<ast::FunctionBlock*> wrapper_functions;

  protected:
    /// doubles are differently represented in ispc than in C
    std::string double_to_string(double value) override;


    /// floats are differently represented in ispc than in C
    std::string float_to_string(float value) override;


    /// name of the code generation backend
    std::string backend_name() override;


    /// return name of main compute kernels
    std::string compute_method_name(BlockType type) override;


    std::string net_receive_buffering_declaration() override;


    std::string ptr_type_qualifier() override;


    std::string param_type_qualifier() override;


    std::string param_ptr_qualifier() override;


    /// common includes : standard c/c++, coreneuron and backend specific
    void print_backend_includes() override;


    /// update to matrix elements with/without shadow vectors
    void print_nrn_cur_matrix_shadow_update() override;


    /// reduction to matrix elements from shadow vectors
    void print_nrn_cur_matrix_shadow_reduction() override;


    /// setup method for setting matrix shadow vectors
    void print_rhs_d_shadow_variables() override;


    /// if reduction block in nrn_cur required
    bool nrn_cur_reduction_loop_required() override;


    ParamVector get_global_function_parms(std::string arg_qualifier);


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

    void print_nmodl_constant() override;


    /// all compute functions for every backend
    void print_compute_functions() override;


    /// nmodl procedure definition
    void print_procedure(ast::ProcedureBlock* node) override;


    void print_backend_compute_routine_decl();


    /// print wrapper function that calls ispc kernel
    void print_wrapper_routine(std::string wraper_function, BlockType type);


    /// wrapper/caller routines for nrn_state and nrn_cur
    void codegen_wrapper_routines();


    /// structure that wraps all global variables in the mod file
    void print_mechanism_global_var_structure(bool wrapper = false) override;


    void print_data_structures() override;


    void print_wrapper_data_structures();


    void print_ispc_globals();


    void print_get_memb_list() override;


    void print_net_receive_loop_begin() override;


    void print_net_receive_buffering_wrapper();


    void print_ion_var_constructor(const std::vector<std::string>& members) override;


    void print_ion_variable() override;


    /// find out for main compute routines whether they are suitable to be emitted in ISPC backend
    void determine_target();


    /// move procedures and functions unused by compute kernels into the wrapper
    void move_procs_to_wrapper();


    /// entry point to code generation
    void print_codegen_routines() override;


    void print_codegen_wrapper_routines();

  public:
    CodegenIspcVisitor(std::string mod_file,
                       std::string output_dir,
                       LayoutType layout,
                       std::string float_type)
        : CodegenCVisitor(mod_file, output_dir, layout, float_type, ".ispc", ".cpp")
        , fallback_codegen(mod_file, layout, float_type, wrapper_printer) {}


    CodegenIspcVisitor(std::string mod_file,
                       std::stringstream& stream,
                       LayoutType layout,
                       std::string float_type)
        : CodegenCVisitor(mod_file, stream, layout, float_type)
        , fallback_codegen(mod_file, layout, float_type, wrapper_printer) {}

    void visit_function_call(ast::FunctionCall* node) override;
    void visit_var_name(ast::VarName* node) override;
    void visit_program(ast::Program* node) override;
};

}  // namespace codegen
}  // namespace nmodl
