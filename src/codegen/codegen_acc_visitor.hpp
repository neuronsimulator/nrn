/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::codegen::CodegenAccVisitor
 */

#include "codegen/codegen_c_visitor.hpp"


namespace nmodl {
namespace codegen {

/**
 * @addtogroup codegen_backends
 * @{
 */

/**
 * \class CodegenAccVisitor
 * \brief %Visitor for printing C code with OpenACC backend
 */
class CodegenAccVisitor: public CodegenCVisitor {
  protected:
    /// name of the code generation backend
    std::string backend_name() const override;


    /// common includes : standard c/c++, coreneuron and backend specific
    void print_backend_includes() override;


    /// ivdep like annotation for channel iterations
    void print_channel_iteration_block_parallel_hint(BlockType type) override;


    /// atomic update pragma for reduction statements
    void print_atomic_reduction_pragma() override;


    /// memory allocation routine
    void print_memory_allocation_routine() const override;


    /// abort routine
    void print_abort_routine() const override;


    /// annotations like "acc enter data present(...)" for main kernel
    void print_kernel_data_present_annotation_block_begin() override;


    /// end of annotation like "acc enter data"
    void print_kernel_data_present_annotation_block_end() override;


    /// start of annotation "acc kernels" for net_init kernel
    void print_net_init_acc_serial_annotation_block_begin() override;


    /// end of annotation "acc kernels" for net_init kernel
    void print_net_init_acc_serial_annotation_block_end() override;


    /// update to matrix elements with/without shadow vectors
    void print_nrn_cur_matrix_shadow_update() override;


    /// reduction to matrix elements from shadow vectors
    void print_nrn_cur_matrix_shadow_reduction() override;

    /// fast membrane current calculation
    void print_fast_imem_calculation() override;

    /// setup method for setting matrix shadow vectors
    void print_rhs_d_shadow_variables() override;


    /// if reduction block in nrn_cur required
    bool nrn_cur_reduction_loop_required() override;


    /// create global variable on the device
    void print_global_variable_device_create_annotation_pre() override;
    void print_global_variable_device_create_annotation_post() override;

    /// update global variable from host to the device
    void print_global_variable_device_update_annotation() override;

    /// transfer newtonspace structure to device
    void print_newtonspace_transfer_to_device() const override;

    // update instance variable object pointer on the gpu device
    void print_instance_variable_transfer_to_device() const override;

    // update derivimplicit advance flag on the gpu device
    void print_deriv_advance_flag_transfer_to_device() const override;

    // update NetSendBuffer_t count from device to host
    void print_net_send_buf_count_update_to_host() const override;

    // update NetSendBuffer_t from device to host
    void print_net_send_buf_update_to_host() const override;

    // update NetSendBuffer_t count from host to device
    virtual void print_net_send_buf_count_update_to_device() const override;

    // update dt from host to device
    virtual void print_dt_update_to_device() const override;

    // synchronise/wait on stream specific to NrnThread
    virtual void print_device_stream_wait() const override;

    // print atomic capture pragma
    void print_device_atomic_capture_annotation() const override;

    std::string get_variable_device_pointer(const std::string& variable,
                                            const std::string& type) const override;


    void print_net_send_buffering_grow() override;


    void print_eigen_linear_solver(const std::string& float_type, int N) override;


  public:
    CodegenAccVisitor(const std::string& mod_file,
                      const std::string& output_dir,
                      const std::string& float_type,
                      bool optimize_ionvar_copies)
        : CodegenCVisitor(mod_file, output_dir, float_type, optimize_ionvar_copies) {}

    CodegenAccVisitor(const std::string& mod_file,
                      std::ostream& stream,
                      const std::string& float_type,
                      bool optimize_ionvar_copies)
        : CodegenCVisitor(mod_file, stream, float_type, optimize_ionvar_copies) {}
};

/** @} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl
