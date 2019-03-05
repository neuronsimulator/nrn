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
 * \class CodegenOmpVisitor
 * \brief Visitor for printing c code with OpenMP backend
 */
class CodegenOmpVisitor: public CodegenCVisitor {
  protected:
    /// name of the code generation backend
    std::string backend_name() override;


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
    void print_channel_iteration_block_parallel_hint() override;


    /// atomic update pragma for reduction statements
    void print_atomic_reduction_pragma() override;


    /// use of shadow updates at channel level required
    bool block_require_shadow_update(BlockType type) override;


  public:
    CodegenOmpVisitor(std::string mod_file,
                      std::string output_dir,
                      LayoutType layout,
                      std::string float_type)
        : CodegenCVisitor(mod_file, output_dir, layout, float_type) {}

    CodegenOmpVisitor(std::string mod_file,
                      std::stringstream& stream,
                      LayoutType layout,
                      std::string float_type)
        : CodegenCVisitor(mod_file, stream, layout, float_type) {}
};

}  // namespace codegen
}  // namespace nmodl