#ifndef NMODL_CODEGEN_C_VISITOR_HPP
#define NMODL_CODEGEN_C_VISITOR_HPP

#include <string>
#include <algorithm>

#include "fmt/format.h"
#include "codegen/codegen_info.hpp"
#include "printer/code_printer.hpp"
#include "symtab/symbol_table.hpp"
#include "visitors/ast_visitor.hpp"
#include "codegen/base/codegen_base_visitor.hpp"


using namespace fmt::literals;


/**
 * \class CodegenCVisitor
 * \brief Visitor for printing c code compatible with legacy api
 *
 * \todo :
 *      - handle define i.e. macro statement printing
 *      - return statement in the verbatim block of inline function not handled (e.g. netstim.mod)
 */
class CodegenCVisitor : public CodegenBaseVisitor {
  protected:
    using SymbolType = std::shared_ptr<symtab::Symbol>;

    /// currently net_receive block being printed
    bool printing_net_receive = false;

    /// currently printing verbatim blocks in top level block
    bool printing_top_verbatim_blocks = false;

    /**
     * Commonly used variables in the verbatim blocks and their corresponding
     * variable name in the new code generation backend.
     */
    std::map<std::string, std::string> verbatim_variables_mapping{
        {"_nt", "nt"},
        {"_p", "data"},
        {"_ppvar", "indexes"},
        {"_thread", "thread"},
        {"_iml", "id"},
        {"_cntml_padded", "pnodecount"},
        {"_cntml", "nodecount"},
        {"_tqitem", "tqitem"},
        {"_threadargs_", nrn_thread_arguments()},
        {"_threadargsproto_", external_method_parameters()}};


    /// number of threads to allocate
    int num_thread_objects() {
        return info.vectorize ? (info.thread_data_index + 1) : 0;
    }


    /// create temporary symbol
    SymbolType make_symbol(std::string name) {
        return std::make_shared<symtab::Symbol>(name, ModToken());
    }


    /// check if structure for ion variables required
    bool ion_variable_struct_required();


    /// when ion variable copies optimized then change name (e.g. ena to ion_ena)
    std::string update_if_ion_variable_name(std::string name);


    /// name of the code generation backend
    virtual std::string backend_name();


    /// get variable name for float variable
    std::string float_variable_name(SymbolType& symbol, bool use_instance);


    /// get variable name for int variable
    std::string int_variable_name(IndexVariableInfo& symbol, std::string name, bool use_instance);


    /// get variable name for global variable
    std::string global_variable_name(SymbolType& symbol);


    /// get ion shadow variable name
    std::string ion_shadow_variable_name(SymbolType& symbol);


    /// get variable name for given name. if use_instance is true then "Instance"
    /// structure is used while returning name
    std::string get_variable_name(std::string name, bool use_instance = true) override;


    /// name of the current variable used in the breakpoint bock
    std::string breakpoint_current(std::string current);


    /// process verbatim block for possible variable renaming
    std::string process_verbatim_text(std::string text);


    /// process token in verbatim block for possible variable renaming
    std::string process_verbatim_token(const std::string& token);


    /// rename function/procedure arguments that conflict with default arguments
    void rename_function_arguments();


    //// statements for reading ion values
    std::vector<std::string> ion_read_statements(BlockType type);


    //// minimal statements for reading ion values
    std::vector<std::string> ion_read_statements_optimal(BlockType type);


    //// statements for writing ion values
    std::vector<ShadowUseStatement> ion_write_statements(BlockType type);


    /// return variable name and corresponding ion read variable name
    std::pair<std::string, std::string> read_ion_variable_name(std::string name);


    /// return variable name and corresponding ion write variable name
    std::pair<std::string, std::string> write_ion_variable_name(std::string name);


    /// function call / statement for nrn_wrote_conc
    std::string conc_write_statement(std::string ion_name,
                                     const std::string& concentration,
                                     int index);


    /// arguments for internally defined functions
    std::string internal_method_arguments();


    /// parameters for internally defined functions
    std::string internal_method_parameters();


    /// arguments for external functions
    std::string external_method_arguments();


    /// parameters for external functions
    std::string external_method_parameters();


    /// arguments for register_mech or point_register_mech function
    std::string register_mechanism_arguments();


    /// arguments for "_threadargs_" macro in neuron implementation
    std::string nrn_thread_arguments();


    /// list of shadow statements in the current block
    std::vector<ShadowUseStatement> shadow_statements;


    /// return name of main compute kernels
    virtual std::string compute_method_name(BlockType type);


    /// restrict keyword
    virtual std::string k_restrict();

    /// const keyword
    virtual std::string k_const();


    /// start of coreneuron namespace
    void print_namespace_start();


    /// end of coreneuron namespace
    void print_namespace_end();


    /// start of backend namespace
    virtual void print_backend_namespace_start();


    /// end of backend namespace
    virtual void print_backend_namespace_end();


    /// top header printed in generated code
    void print_backend_info();


    /// memory allocation routine
    virtual void print_memory_allocation_routine();


    /// common includes : standard c/c++, coreneuron and backend specific
    void print_standard_includes();
    void print_coreneuron_includes();
    virtual void print_backend_includes();


    /// use of shadow updates at channel level required
    virtual bool block_require_shadow_update(BlockType type);


    /// channel execution with dependency (backend specific)
    virtual bool channel_task_dependency_enabled();


    /// check if shadow_vector_setup function is required
    bool shadow_vector_setup_required();


    /// ion variable copies are avoided
    bool optimize_ion_variable_copies();


    /// if reduction block in nrn_cur required
    virtual bool nrn_cur_reduction_loop_required();


    /// if variable is qualified as constant
    virtual bool is_constant_variable(std::string name);


    /// char array that has mechanism information (to be registered with coreneuron)
    void print_mechanism_info_array();


    /// structure that wraps all global variables in the mod file
    void print_mechanism_global_structure();


    /// structure that wraps all range and int variables required for mod file
    void print_mechanism_var_structure();


    /// structure of ion variables used for local copies
    void print_ion_variables_structure();


    /// return floating point type for given range variable symbol
    std::string get_range_var_float_type(const SymbolType& symbol);


    /// function that initialize range variable with different data type
    void print_setup_range_variable();


    /// function that initialize instance structure
    void print_instance_variable_setup();


    /// char arrays that registers scalar and vector variables for hoc interface
    void print_global_variables_for_hoc();


    /// getter method for thread variables and ids
    void print_thread_getters();


    /// getter method for memory layout
    void print_memory_layout_getter();


    /// getter method for index position of first pointer variable
    void print_first_pointer_var_index_getter();


    /// getter methods for float and integer variables count
    void print_num_variable_getter();


    /// getter method for getting number of arguments for net_receive
    void print_net_receive_arg_size_getter();


    /// getter method for returning membrane list from NrnThread
    void print_memb_list_getter();


    /// getter method for returning mechtype
    void print_mech_type_getter();


    /// setup method that initializes all global variables
    void print_global_variable_setup();


    /// setup method for allocation of shadow vectors
    void print_shadow_vector_setup();


    /// setup method for setting matrix shadow vectors
    virtual void print_rhs_d_shadow_variables();


    /// backend specific device method annotation
    virtual void print_device_method_annotation();


    /// backend specific global method annotation
    virtual void print_global_method_annotation();


    /// call to internal or external function
    void print_function_call(ast::FunctionCall* node);


    /// net_send call
    void print_net_send_call(ast::FunctionCall* node);


    /// net_event call
    void print_net_event_call(ast::FunctionCall* node);


    /// channel iterations from which task can be created
    virtual void print_channel_iteration_task_begin(BlockType type);


    /// end of task for channel iteration
    virtual void print_channel_iteration_task_end();


    /// backend specific block start for tiling on channel iteration
    virtual void print_channel_iteration_tiling_block_begin(BlockType type);


    /// backend specific block end for tiling on channel iteration
    virtual void print_channel_iteration_tiling_block_end();


    /// ivdep like annotation for channel iterations
    virtual void print_channel_iteration_block_parallel_hint();


    /// annotations like "acc enter data present(...)" for main kernel
    virtual void print_kernel_data_present_annotation_block_begin();


    /// end of annotation like "acc enter data"
    virtual void print_kernel_data_present_annotation_block_end();


    /// backend specific channel instance iteration block start
    virtual void print_channel_iteration_block_begin();


    /// backend specific channel instance iteration block end
    virtual void print_channel_iteration_block_end();


    /// common code post channel instance iteration
    void print_post_channel_iteration_common_code();


    /// function and procedures prototype declaration
    void print_function_prototypes();


    /// nmodl function definition
    void print_function(ast::FunctionBlock* node);


    /// nmodl procedure definition
    void print_procedure(ast::ProcedureBlock* node);


    /// thread related memory allocation and deallocation callbacks
    void print_thread_memory_callbacks();


    /// top level (global scope) verbatim blocks
    void print_top_verbatim_blocks();


    /// any statement block in nmodl with option to (not) print braces
    void print_statement_block(ast::StatementBlock* node,
                               bool open_brace = true,
                               bool close_brace = true);


    /// prototype declarations of functions and procedures
    template <typename T>
    void print_function_declaration(T& node);


    /// initial block
    void print_initial_block(ast::InitialBlock* node);


    /// initial block in the net receive block
    void print_net_init_kernel();


    /// common code section for net receive related methods
    void print_net_receive_common_code(ast::Block* node);


    /// kernel for buffering net_send events
    void print_net_send_buffer_kernel();


    /// kernel for buffering net_receive events
    void print_net_receive_buffer_kernel();


    /// net_receive function definition
    void print_net_receive();


    /// derivative kernel when euler method is used
    void print_derivative_kernel_for_euler();


    /// derivative kernel when derivimplicit method is used
    void print_derivative_kernel_for_derivimplicit();


    /// block / loop for statement requiring reduction
    void print_shadow_reduction_block_begin();


    /// end of block / loop for statement requiring reduction
    void print_shadow_reduction_block_end();


    /// atomic update pragma for reduction statements
    virtual void print_atomic_reduction_pragma();


    /// print all reduction statements
    void print_shadow_reduction_statements();


    /// process shadow update statement : if statement requires reduction then
    /// add it to vector of reduction statement and return statement using shadow update
    std::string process_shadow_update_statement(ShadowUseStatement& statement, BlockType type);


    /// main body of nrn_cur function
    void print_nrn_cur_kernel(ast::BreakpointBlock* node);


    /// nrn_cur_kernel will use this kernel if conductance keywords are specified
    void print_nrn_cur_conductance_kernel(ast::BreakpointBlock* node);


    /// nrn_cur_kernel will use this kernel if no conductance keywords are specified
    void print_nrn_cur_non_conductance_kernel();


    /// nrn_cur_kernel will have two calls to nrn_current if no conductance keywords specified
    void print_nrn_current(ast::BreakpointBlock* node);


    /// update to matrix elements with/without shadow vectors
    virtual void print_nrn_cur_matrix_shadow_update();


    /// reduction to matrix elements from shadow vectors
    virtual void print_nrn_cur_matrix_shadow_reduction();


    /// nrn_alloc function definition
    void print_nrn_alloc();


    /// common code for global functions like nrn_init, nrn_cur and nrn_state
    void print_global_function_common_code(BlockType type);

    /// nrn_init function definition
    void print_nrn_init();


    /// nrn_state / state update function definition
    void print_nrn_state();


    /// nrn_cur / current update function definition
    void print_nrn_cur();


    /// mechanism registration function
    void print_mechanism_register();

    /// all includes
    void codegen_includes();

    /// start of namespaces
    void codegen_namespace_begin();

    /// end of namespaces
    void codegen_namespace_end();

    /// common getter
    void codegen_common_getters();

    /// all classes
    void codegen_data_structures();

    /// all compute functions for every backend
    virtual void codegen_compute_functions();

    /// entry point to code generation
    virtual void codegen_all();

  public:
    CodegenCVisitor(std::string mod_file,
                    bool aos,
                    std::string float_type,
                    std::string extension = ".cpp")
        : CodegenBaseVisitor(mod_file, aos, float_type, extension) {
    }

    CodegenCVisitor(std::string mod_file,
                    std::stringstream& stream,
                    bool aos,
                    std::string float_type)
        : CodegenBaseVisitor(mod_file, stream, aos, float_type) {
    }

    virtual void visit_function_call(ast::FunctionCall* node) override;
    virtual void visit_verbatim(ast::Verbatim* node) override;
    virtual void visit_program(ast::Program* node) override;
};



/**
 * Print prototype declaration for function and procedures
 *
 * If there is an argument with name (say alpha) same as range variable (say alpha),
 * we want avoid it being printed as instance->alpha. And hence we disable variable
 * name lookup during prototype declaration.
 */
template <typename T>
void CodegenCVisitor::print_function_declaration(T& node) {
    enable_variable_name_lookup = false;

    /// name, internal and user provided arguments
    auto name = node->get_name();
    auto internal_params = internal_method_parameters();
    auto params = node->get_arguments();

    /// procedures have "int" return type by default
    std::string return_type = "int";
    if (node->is_function_block()) {
        return_type = default_float_data_type();
    }

    print_device_method_annotation();
    printer->add_indent();
    printer->add_text("inline {} {}({}"_format(return_type, method_name(name), internal_params));

    /// print remaining of arguments to the function
    if (!params.empty() && !internal_params.empty()) {
        printer->add_text(", ");
    }
    auto type = default_float_data_type() + " ";
    print_vector_elements(params, ", ", type);
    printer->add_text(")");

    enable_variable_name_lookup = true;
}

#endif
