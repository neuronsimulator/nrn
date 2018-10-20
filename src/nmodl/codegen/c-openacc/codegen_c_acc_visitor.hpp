#ifndef NMODL_CODEGEN_C_ACC_VISITOR_HPP
#define NMODL_CODEGEN_C_ACC_VISITOR_HPP

#include "codegen/c/codegen_c_visitor.hpp"


/**
 * \class CodegenCAccVisitor
 * \brief Visitor for printing c code with OpenMP backend
 *
 * \todo :
 *      - handle define i.e. macro statement printing
 *      - return statement in the verbatim block of inline function not handled (e.g. netstim.mod)
 */
class CodegenCAccVisitor : public CodegenCVisitor {
  protected:
    /// name of the code generation backend
    std::string backend_name() override;


    /// common includes : standard c/c++, coreneuron and backend specific
    void print_backend_includes() override;


    /// ivdep like annotation for channel iterations
    void print_channel_iteration_block_parallel_hint() override;


    /// atomic update pragma for reduction statements
    void print_atomic_reduction_pragma() override;


    /// memory allocation routine
    void print_memory_allocation_routine() override;


    /// annotations like "acc enter data present(...)" for main kernel
    void print_kernel_data_present_annotation_block_begin() override;


    /// end of annotation like "acc enter data"
    void print_kernel_data_present_annotation_block_end() override;


    /// update to matrix elements with/without shadow vectors
    void print_nrn_cur_matrix_shadow_update() override;


    /// reduction to matrix elements from shadow vectors
    void print_nrn_cur_matrix_shadow_reduction() override;


    /// setup method for setting matrix shadow vectors
    void print_rhs_d_shadow_variables() override;

    /// if reduction block in nrn_cur required
    bool nrn_cur_reduction_loop_required() override;


  public:
    CodegenCAccVisitor(std::string mod_file,
                       std::string output_dir,
                       bool aos,
                       std::string float_type)
        : CodegenCVisitor(mod_file, output_dir, aos, float_type) {
    }

    CodegenCAccVisitor(std::string mod_file,
                       std::stringstream& stream,
                       bool aos,
                       std::string float_type)
        : CodegenCVisitor(mod_file, stream, aos, float_type) {
    }
};


#endif
