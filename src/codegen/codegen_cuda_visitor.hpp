/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::codegen::CodegenCudaVisitor
 */

#include "codegen/codegen_c_visitor.hpp"

namespace nmodl {
namespace codegen {

/**
 * @addtogroup codegen_backends
 * @{
 */

/**
 * \class CodegenCudaVisitor
 * \brief %Visitor for printing CUDA backend
 */
class CodegenCudaVisitor: public CodegenCVisitor {
    void print_atomic_op(const std::string& lhs, const std::string& op, const std::string& rhs);

  protected:
    /// name of the code generation backend
    std::string backend_name() override;

    /// if variable is qualified as constant
    bool is_constant_variable(std::string name) override;

    /// return name of main compute kernels
    std::string compute_method_name(BlockType type) override;


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


    /// backend specific channel instance iteration block start
    void print_channel_iteration_block_begin(BlockType type) override;


    /// backend specific channel instance iteration block end
    void print_channel_iteration_block_end() override;


    /// start of backend namespace
    void print_backend_namespace_start() override;


    /// end of backend namespace
    void print_backend_namespace_stop() override;


    /// backend specific global method annotation
    void print_global_method_annotation() override;


    /// backend specific device method annotation
    void print_device_method_annotation() override;


    /// all compute functions for every backend
    void print_compute_functions() override;


    /// print wrapper function that calls cuda kernel
    void print_wrapper_routine(std::string wraper_function, BlockType type);


    /// wrapper/caller routines for nrn_state and nrn_cur
    void codegen_wrapper_routines();


    /// entry point to code generation
    void print_codegen_routines() override;

  public:
    CodegenCudaVisitor(std::string mod_file,
                       std::string output_dir,
                       LayoutType layout,
                       std::string float_type)
        : CodegenCVisitor(mod_file, output_dir, layout, float_type, ".cu") {}

    CodegenCudaVisitor(std::string mod_file,
                       std::stringstream& stream,
                       LayoutType layout,
                       std::string float_type)
        : CodegenCVisitor(mod_file, stream, layout, float_type) {}
};

/** @} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl