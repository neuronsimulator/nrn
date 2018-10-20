#ifndef NMODL_CODEGEN_C_CUDA_VISITOR_HPP
#define NMODL_CODEGEN_C_CUDA_VISITOR_HPP

#include "codegen/c/codegen_c_visitor.hpp"


/**
 * \class CodegenCCudaVisitor
 * \brief Visitor for printing CUDA backend
 *
 * \todo :
 *      - handle define i.e. macro statement printing
 *      - return statement in the verbatim block of inline function not handled (e.g. netstim.mod)
 */
class CodegenCCudaVisitor : public CodegenCVisitor {
    void print_atomic_op(std::string lhs, std::string op, std::string rhs);

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
    void print_channel_iteration_block_begin() override;


    /// backend specific channel instance iteration block end
    void print_channel_iteration_block_end() override;


    /// start of backend namespace
    void print_backend_namespace_start() override;


    /// end of backend namespace
    void print_backend_namespace_end() override;


    /// backend specific global method annotation
    void print_global_method_annotation() override;


    /// backend specific device method annotation
    void print_device_method_annotation() override;


    /// all compute functions for every backend
    void codegen_compute_functions() override;


    /// print wrapper function that calls cuda kernel
    void print_wrapper_routine(std::string wraper_function, BlockType type);


    /// wrapper/caller routines for nrn_state and nrn_cur
    void codegen_wrapper_routines();


    /// entry point to code generation
    void codegen_all() override;

  public:
    CodegenCCudaVisitor(std::string mod_file,
                        std::string output_dir,
                        bool aos,
                        std::string float_type)
        : CodegenCVisitor(mod_file, output_dir, aos, float_type, ".cu") {
    }

    CodegenCCudaVisitor(std::string mod_file,
                        std::stringstream& stream,
                        bool aos,
                        std::string float_type)
        : CodegenCVisitor(mod_file, stream, aos, float_type) {
    }
};

#endif
