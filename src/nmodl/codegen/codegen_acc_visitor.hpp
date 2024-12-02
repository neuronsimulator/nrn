/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::codegen::CodegenAccVisitor
 */

#include "codegen/codegen_coreneuron_cpp_visitor.hpp"


namespace nmodl {
namespace codegen {

/**
 * \addtogroup codegen_backends
 * \{
 */

/**
 * \class CodegenAccVisitor
 * \brief %Visitor for printing C++ code with OpenACC backend
 */
class CodegenAccVisitor: public CodegenCoreneuronCppVisitor {
  public:
    using CodegenCoreneuronCppVisitor::CodegenCoreneuronCppVisitor;

  protected:

    /// name of the code generation backend
    std::string backend_name() const override;


    /// common includes : standard c++, coreneuron and backend specific
    void print_backend_includes() override;


    /// ivdep like annotation for channel iterations
    void print_parallel_iteration_hint(BlockType type, const ast::Block* block) override;


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

    /// update global variable from host to the device
    void print_global_variable_device_update_annotation() override;

    /// transfer newtonspace structure to device
    void print_newtonspace_transfer_to_device() const override;

    /// declare helper functions for copying the instance struct to the device
    void print_instance_struct_transfer_routine_declarations() override;

    /// define helper functions for copying the instance struct to the device
    void print_instance_struct_transfer_routines(
        const std::vector<std::string>& ptr_members) override;

    /// call helper function for copying the instance struct to the device
    void print_instance_struct_copy_to_device() override;

    /// call helper function that deletes the instance struct from the device
    void print_instance_struct_delete_from_device() override;

    /// update derivimplicit advance flag on the gpu device
    void print_deriv_advance_flag_transfer_to_device() const override;

    /// update NetSendBuffer_t count from device to host
    void print_net_send_buf_count_update_to_host() const override;

    /// update NetSendBuffer_t from device to host
    void print_net_send_buf_update_to_host() const override;

    /// update NetSendBuffer_t count from host to device
    void print_net_send_buf_count_update_to_device() const override;

    /// update dt from host to device
    void print_dt_update_to_device() const override;

    // synchronise/wait on stream specific to NrnThread
    void print_device_stream_wait() const override;

    /// print atomic capture pragma
    void print_device_atomic_capture_annotation() const override;

    /// print atomic update of NetSendBuffer_t cnt
    void print_net_send_buffering_cnt_update() const override;

    /// Replace default implementation by a no-op
    /// since the buffer cannot be grown up during gpu execution
    void print_net_send_buffering_grow() override;
};

/** \} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl
