/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::codegen::CodegenOmpVisitor
 */

#include "codegen/codegen_c_visitor.hpp"


namespace nmodl {
namespace codegen {

/**
 * @addtogroup codegen_backends
 * @{
 */

/**
 * \class CodegenOmpVisitor
 * \brief %Visitor for printing C code with OpenMP backend
 */
class CodegenOmpVisitor: public CodegenCVisitor {
  protected:
    /// name of the code generation backend
    std::string backend_name() const override;


    /// common includes : standard c/c++, coreneuron and backend specific
    void print_backend_includes() override;


    /// channel execution with dependency (backend specific)
    bool channel_task_dependency_enabled() override;


    /// channel iterations from which task can be created
    void print_channel_iteration_task_begin(BlockType type) override;


    /// end of task for channel iteration
    void print_channel_iteration_task_end() override;


    /// backend specific block start for tiling on channel iteration
    void print_channel_iteration_tiling_block_begin(BlockType type) override;


    /// backend specific block end for tiling on channel iteration
    void print_channel_iteration_tiling_block_end() override;


    /// ivdep like annotation for channel iterations
    void print_channel_iteration_block_parallel_hint(BlockType type) override;


    /// atomic update pragma for reduction statements
    void print_atomic_reduction_pragma() override;


    /// use of shadow updates at channel level required
    bool block_require_shadow_update(BlockType type) override;


  public:
    CodegenOmpVisitor(std::string mod_file,
                      std::string output_dir,
                      std::string float_type,
                      const bool optimize_ionvar_copies)
        : CodegenCVisitor(mod_file, output_dir, float_type, optimize_ionvar_copies) {}

    CodegenOmpVisitor(std::string mod_file,
                      std::stringstream& stream,
                      std::string float_type,
                      const bool optimize_ionvar_copies)
        : CodegenCVisitor(mod_file, stream, float_type, optimize_ionvar_copies) {}
};

/** @} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl
