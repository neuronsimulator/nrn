/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \dir
 * \brief Code generation backend implementations for CoreNEURON
 *
 * \file
 * \brief \copybrief nmodl::codegen::CodegenCoreneuronCppVisitor
 */

#include <algorithm>
#include <cmath>
#include <ctime>
#include <numeric>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "codegen/codegen_cpp_visitor.hpp"
#include "codegen/codegen_info.hpp"
#include "codegen/codegen_naming.hpp"
#include "printer/code_printer.hpp"
#include "symtab/symbol_table.hpp"
#include "utils/logger.hpp"
#include "visitors/ast_visitor.hpp"


namespace nmodl {

namespace codegen {


using printer::CodePrinter;


/**
 * \defgroup codegen_backends Codegen Backends
 * \ingroup codegen
 * \brief Code generation backends for CoreNEURON
 * \{
 */

/**
 * \class CodegenCoreneuronCppVisitor
 * \brief %Visitor for printing C++ code compatible with legacy api of CoreNEURON
 *
 * \todo
 *  - Handle define statement (i.e. macros)
 *  - If there is a return statement in the verbatim block
 *    of inlined function then it will be error. Need better
 *    error checking. For example, see netstim.mod where we
 *    have removed return from verbatim block.
 */
class CodegenCoreneuronCppVisitor: public CodegenCppVisitor {
  protected:
    /****************************************************************************************/
    /*                                    Member variables                                  */
    /****************************************************************************************/


    /****************************************************************************************/
    /*                              Generic information getters                             */
    /****************************************************************************************/


    /**
     * Name of the simulator the code was generated for
     */
    std::string simulator_name() override;


    /**
     * Name of the code generation backend
     */
    virtual std::string backend_name() const override;


    /**
     * Name of structure that wraps range variables
     */
    std::string instance_struct() const {
        return fmt::format("{}_Instance", info.mod_suffix);
    }


    /**
     * Name of structure that wraps global variables
     */
    std::string global_struct() const {
        return fmt::format("{}_Store", info.mod_suffix);
    }


    /**
     * Name of the (host-only) global instance of `global_struct`
     */
    std::string global_struct_instance() const {
        return info.mod_suffix + "_global";
    }


    /**
     * Determine the number of threads to allocate
     */
    int num_thread_objects() const noexcept {
        return info.vectorize ? (info.thread_data_index + 1) : 0;
    }


    /****************************************************************************************/
    /*                     Common helper routines accross codegen functions                 */
    /****************************************************************************************/


    /**
     * Determine the position in the data array for a given float variable
     * \param name The name of a float variable
     * \return     The position index in the data array
     */
    int position_of_float_var(const std::string& name) const override;


    /**
     * Determine the position in the data array for a given int variable
     * \param name The name of an int variable
     * \return     The position index in the data array
     */
    int position_of_int_var(const std::string& name) const override;


    /**
     * Determine the variable name for the "current" used in breakpoint block taking into account
     * intermediate code transformations.
     * \param current The variable name for the current used in the model
     * \return        The name for the current to be printed in C++
     */
    std::string breakpoint_current(std::string current) const;


    /**
     * For a given output block type, return statements for all read ion variables
     *
     * \param type The type of code block being generated
     * \return     A \c vector of strings representing the reading of ion variables
     */
    std::vector<std::string> ion_read_statements(BlockType type) const;


    /**
     * For a given output block type, return minimal statements for all read ion variables
     *
     * \param type The type of code block being generated
     * \return     A \c vector of strings representing the reading of ion variables
     */
    std::vector<std::string> ion_read_statements_optimized(BlockType type) const;


    /**
     * For a given output block type, return statements for writing back ion variables
     *
     * \param type The type of code block being generated
     * \return     A \c vector of strings representing the write-back of ion variables
     */
    std::vector<ShadowUseStatement> ion_write_statements(BlockType type);


    /**
     * Process a token in a verbatim block for possible variable renaming
     * \param token The verbatim token to be processed
     * \return      The code after variable renaming
     */
    std::string process_verbatim_token(const std::string& token);


    /**
     * Check if a structure for ion variables is required
     * \return \c true if a structure fot ion variables must be generated
     */
    bool ion_variable_struct_required() const;


    /**
     * Check if variable is qualified as constant
     * \param name The name of variable
     * \return \c true if it is constant
     */
    virtual bool is_constant_variable(const std::string& name) const;


    /****************************************************************************************/
    /*                                Backend specific routines                             */
    /****************************************************************************************/


    /**
     * Print the code to copy derivative advance flag to device
     */
    virtual void print_deriv_advance_flag_transfer_to_device() const;


    /**
     * Print pragma annotation for increase and capture of variable in automatic way
     */
    virtual void print_device_atomic_capture_annotation() const;


    /**
     * Print the code to update NetSendBuffer_t count from device to host
     */
    virtual void print_net_send_buf_count_update_to_host() const;


    /**
     * Print the code to update NetSendBuffer_t from device to host
     */
    virtual void print_net_send_buf_update_to_host() const;


    /**
     * Print the code to update NetSendBuffer_t count from host to device
     */
    virtual void print_net_send_buf_count_update_to_device() const;

    /**
     * Print the code to update dt from host to device
     */
    virtual void print_dt_update_to_device() const;

    /**
     * Print the code to synchronise/wait on stream specific to NrnThread
     */
    virtual void print_device_stream_wait() const;


    /**
     * Print accelerator annotations indicating data presence on device
     */
    virtual void print_kernel_data_present_annotation_block_begin();


    /**
     * Print matching block end of accelerator annotations for data presence on device
     */
    virtual void print_kernel_data_present_annotation_block_end();


    /**
     * Print accelerator kernels begin annotation for net_init kernel
     */
    virtual void print_net_init_acc_serial_annotation_block_begin();


    /**
     * Print accelerator kernels end annotation for net_init kernel
     */
    virtual void print_net_init_acc_serial_annotation_block_end();


    /**
     * Print pragma annotations for channel iterations
     *
     * This can be overriden by backends to provide additonal annotations or pragmas to enable
     * for example SIMD code generation (e.g. through \c ivdep)
     * The default implementation prints
     *
     * \code
     * #pragma ivdep
     * \endcode
     *
     * \param type The block type
     */
    virtual void print_channel_iteration_block_parallel_hint(BlockType type,
                                                             const ast::Block* block);


    /**
     * Check if reduction block in \c nrn\_cur required
     */
    virtual bool nrn_cur_reduction_loop_required();


    /**
     * Print the setup method for setting matrix shadow vectors
     *
     */
    virtual void print_rhs_d_shadow_variables();


    /**
     * Print the update to matrix elements with/without shadow vectors
     *
     */
    virtual void print_nrn_cur_matrix_shadow_update();


    /**
     * Print the reduction to matrix elements from shadow vectors
     *
     */
    virtual void print_nrn_cur_matrix_shadow_reduction();


    /**
     * Print atomic update pragma for reduction statements
     */
    virtual void print_atomic_reduction_pragma() override;


    /**
     * Print the backend specific device method annotation
     *
     * \note This is not used for the C++ backend
     */
    virtual void print_device_method_annotation();


    /**
     * Print backend specific global method annotation
     *
     * \note This is not used for the C++ backend
     */
    virtual void print_global_method_annotation();


    /**
     * Prints the start of namespace for the backend-specific code
     *
     * For the C++ backend no additional namespace is required
     */
    virtual void print_backend_namespace_start();


    /**
     * Prints the end of namespace for the backend-specific code
     *
     * For the C++ backend no additional namespace is required
     */
    virtual void print_backend_namespace_stop();


    /**
     * Print backend specific includes (none needed for C++ backend)
     */
    virtual void print_backend_includes();


    /**
     * Check if ion variable copies should be avoided
     */
    bool optimize_ion_variable_copies() const;


    /**
     * Print memory allocation routine
     */
    virtual void print_memory_allocation_routine() const;


    /**
     * Print backend specific abort routine
     */
    virtual void print_abort_routine() const;


    /**
     * Instantiate global var instance
     *
     * For C++ code generation this is empty
     * \return ""
     */
    virtual void print_global_var_struct_decl();


    /**
     * Print declarations of the functions used by \ref
     * print_instance_struct_copy_to_device and \ref
     * print_instance_struct_delete_from_device.
     */
    virtual void print_instance_struct_transfer_routine_declarations() {}

    /**
     * Print the definitions of the functions used by \ref
     * print_instance_struct_copy_to_device and \ref
     * print_instance_struct_delete_from_device. Declarations of these functions
     * are printed by \ref print_instance_struct_transfer_routine_declarations.
     *
     * This updates the (pointer) member variables in the device copy of the
     * instance struct to contain device pointers, which is why you must pass a
     * list of names of those member variables.
     *
     * \param ptr_members List of instance struct member names.
     */
    virtual void print_instance_struct_transfer_routines(
        std::vector<std::string> const& /* ptr_members */) {}


    /**
     * Transfer the instance struct to the device. This calls a function
     * declared by \ref print_instance_struct_transfer_routine_declarations.
     */
    virtual void print_instance_struct_copy_to_device() {}


    /**
     * Delete the instance struct from the device. This calls a function
     * declared by \ref print_instance_struct_transfer_routine_declarations.
     */
    virtual void print_instance_struct_delete_from_device() {}


    /****************************************************************************************/
    /*                         Printing routines for code generation                        */
    /****************************************************************************************/


    /**
     * Print top level (global scope) verbatim blocks
     */
    void print_top_verbatim_blocks();


    /**
     * Print function and procedures prototype declaration
     */
    void print_function_prototypes() override;


    /**
     * Check if the given name exist in the symbol
     * \return \c return a tuple <true, array_length> if variable
     *            is an array otherwise <false, 0>
     */
    std::tuple<bool, int> check_if_var_is_array(const std::string& name);


    /**
     * Print \c check\_function() for functions or procedure using table
     * \param node The AST node representing a function or procedure block
     */
    void print_table_check_function(const ast::Block& node);


    /**
     * Print replacement function for function or procedure using table
     * \param node The AST node representing a function or procedure block
     */
    void print_table_replacement_function(const ast::Block& node);


    /**
     * Print check_table functions
     */
    void print_check_table_thread_function();


    /**
     * Print nmodl function or procedure (common code)
     * \param node the AST node representing the function or procedure in NMODL
     * \param name the name of the function or procedure
     */
    void print_function_or_procedure(const ast::Block& node, const std::string& name) override;


    /**
     * Common helper function to help printing function or procedure blocks
     * \param node the AST node representing the function or procedure in NMODL
     */
    void print_function_procedure_helper(const ast::Block& node) override;


    /**
     * Print NMODL procedure in target backend code
     * \param node
     */
    virtual void print_procedure(const ast::ProcedureBlock& node) override;


    /**
     * Print NMODL function in target backend code
     * \param node
     */
    void print_function(const ast::FunctionBlock& node) override;


    /**
     * Print NMODL function_table in target backend code
     * \param node
     */
    void print_function_tables(const ast::FunctionTableBlock& node);


    bool is_functor_const(const ast::StatementBlock& variable_block,
                          const ast::StatementBlock& functor_block);


    /**
     * \brief Based on the \c EigenNewtonSolverBlock passed print the definition needed for its
     * functor
     *
     * \param node \c EigenNewtonSolverBlock for which to print the functor
     */
    void print_functor_definition(const ast::EigenNewtonSolverBlock& node);


    virtual void print_eigen_linear_solver(const std::string& float_type, int N);


    /****************************************************************************************/
    /*                             Code-specific helper routines                            */
    /****************************************************************************************/


    /**
     * Arguments for functions that are defined and used internally.
     * \return the method arguments
     */
    std::string internal_method_arguments() override;


    /**
     * Parameters for internally defined functions
     * \return the method parameters
     */
    ParamVector internal_method_parameters() override;


    /**
     * Arguments for external functions called from generated code
     * \return A string representing the arguments passed to an external function
     */
    const char* external_method_arguments() noexcept override;


    /**
     * Parameters for functions in generated code that are called back from external code
     *
     * Functions registered in NEURON during initialization for callback must adhere to a prescribed
     * calling convention. This method generates the string representing the function parameters for
     * these externally called functions.
     * \param table
     * \return      A string representing the parameters of the function
     */
    const char* external_method_parameters(bool table = false) noexcept override;


    /**
     * Arguments for "_threadargs_" macro in neuron implementation
     */
    std::string nrn_thread_arguments() const override;


    /**
     * Arguments for "_threadargs_" macro in neuron implementation
     */
    std::string nrn_thread_internal_arguments() override;


    /**
     * Replace commonly used verbatim variables
     * \param name A variable name to be checked and possibly updated
     * \return     The possibly replace variable name
     */
    std::string replace_if_verbatim_variable(std::string name);


    /**
     * Process a verbatim block for possible variable renaming
     * \param text The verbatim code to be processed
     * \return     The code with all variables renamed as needed
     */
    std::string process_verbatim_text(std::string const& text) override;


    /**
     * Arguments for register_mech or point_register_mech function
     */
    std::string register_mechanism_arguments() const override;


    /**
     * Return ion variable name and corresponding ion read variable name
     * \param name The ion variable name
     * \return     The ion read variable name
     */
    static std::pair<std::string, std::string> read_ion_variable_name(const std::string& name);


    /**
     * Return ion variable name and corresponding ion write variable name
     * \param name The ion variable name
     * \return     The ion write variable name
     */
    static std::pair<std::string, std::string> write_ion_variable_name(const std::string& name);


    /**
     * Generate Function call statement for nrn_wrote_conc
     * \param ion_name      The name of the ion variable
     * \param concentration The name of the concentration variable
     * \param index
     * \return              The string representing the function call
     */
    std::string conc_write_statement(const std::string& ion_name,
                                     const std::string& concentration,
                                     int index);

    /**
     * Process shadow update statement
     *
     * If the statement requires reduction then add it to vector of reduction statement and return
     * statement using shadow update
     *
     * \param statement The statement that might require shadow updates
     * \param type      The target backend code block type
     * \return          The generated target backend code
     */
    std::string process_shadow_update_statement(const ShadowUseStatement& statement,
                                                BlockType type);


    /****************************************************************************************/
    /*                  Code-specific printing routines for code generations                */
    /****************************************************************************************/


    /**
     * Print the getter method for index position of first pointer variable
     *
     */
    void print_first_pointer_var_index_getter();


    /**
     * Print the getter methods for float and integer variables count
     *
     */
    void print_num_variable_getter();


    /**
     * Print the getter method for getting number of arguments for net_receive
     *
     */
    void print_net_receive_arg_size_getter();


    /**
     * Print the getter method for returning mechtype
     *
     */
    void print_mech_type_getter();


    /**
     * Print the getter method for returning membrane list from NrnThread
     *
     */
    void print_memb_list_getter();


    /**
     * Prints the start of the \c coreneuron namespace
     */
    void print_namespace_start() override;


    /**
     * Prints the end of the \c coreneuron namespace
     */
    void print_namespace_stop() override;


    /**
     * Print the getter method for thread variables and ids
     *
     */
    void print_thread_getters();


    /****************************************************************************************/
    /*                         Routines for returning variable name                         */
    /****************************************************************************************/


    /**
     * Determine the updated name if the ion variable has been optimized
     * \param name The ion variable name
     * \return     The updated name of the variable has been optimized (e.g. \c ena --> \c ion_ena)
     */
    std::string update_if_ion_variable_name(const std::string& name) const;


    /**
     * Determine the name of a \c float variable given its symbol
     *
     * This function typically returns the accessor expression in backend code for the given symbol.
     * Since the model variables are stored in data arrays and accessed by offset, this function
     * will return the C++ string representing the array access at the correct offset
     *
     * \param symbol       The symbol of a variable for which we want to obtain its name
     * \param use_instance Should the variable be accessed via instance or data array
     * \return             The backend code string representing the access to the given variable
     * symbol
     */
    std::string float_variable_name(const SymbolType& symbol, bool use_instance) const override;


    /**
     * Determine the name of an \c int variable given its symbol
     *
     * This function typically returns the accessor expression in backend code for the given symbol.
     * Since the model variables are stored in data arrays and accessed by offset, this function
     * will return the C++ string representing the array access at the correct offset
     *
     * \param symbol       The symbol of a variable for which we want to obtain its name
     * \param name         The name of the index variable
     * \param use_instance Should the variable be accessed via instance or data array
     * \return             The backend code string representing the access to the given variable
     * symbol
     */
    std::string int_variable_name(const IndexVariableInfo& symbol,
                                  const std::string& name,
                                  bool use_instance) const override;


    /**
     * Determine the variable name for a global variable given its symbol
     * \param symbol The symbol of a variable for which we want to obtain its name
     * \param use_instance Should the variable be accessed via the (host-only)
     * global variable or the instance-specific copy (also available on GPU).
     * \return       The C++ string representing the access to the global variable
     */
    std::string global_variable_name(const SymbolType& symbol,
                                     bool use_instance = true) const override;


    /**
     * Determine variable name in the structure of mechanism properties
     *
     * \param name         Variable name that is being printed
     * \param use_instance Should the variable be accessed via instance or data array
     * \return             The C++ string representing the access to the variable in the neuron
     * thread structure
     */
    std::string get_variable_name(const std::string& name, bool use_instance = true) const override;


    /****************************************************************************************/
    /*                      Main printing routines for code generation                      */
    /****************************************************************************************/


    /**
     * Print top file header printed in generated code
     */
    void print_backend_info() override;


    /**
     * Print standard C/C++ includes
     */
    void print_standard_includes() override;


    /**
     * Print includes from coreneuron
     */
    void print_coreneuron_includes();


    void print_sdlists_init(bool print_initializers) override;


    /**
     * Print the structure that wraps all global variables used in the NMODL
     *
     * \param print_initializers Whether to include default values in the struct
     *                           definition (true: int foo{42}; false: int foo;)
     */
    void print_mechanism_global_var_structure(bool print_initializers) override;


    /**
     * Print static assertions about the global variable struct.
     */
    virtual void print_global_var_struct_assertions() const;


    /**
     * Print byte arrays that register scalar and vector variables for hoc interface
     *
     */
    void print_global_variables_for_hoc() override;


    /**
     * Print the mechanism registration function
     *
     */
    void print_mechanism_register() override;


    /**
     * Print thread related memory allocation and deallocation callbacks
     */
    void print_thread_memory_callbacks();


    /**
     * Print structure of ion variables used for local copies
     */
    void print_ion_var_structure();


    /**
     * Print constructor of ion variables
     * \param members The ion variable names
     */
    virtual void print_ion_var_constructor(const std::vector<std::string>& members);


    /**
     * Print the ion variable struct
     */
    virtual void print_ion_variable();


    /**
     * Print the pragma annotation to update global variables from host to the device
     *
     * \note This is not used for the C++ backend
     */
    virtual void print_global_variable_device_update_annotation();


    /**
     * Print the function that initialize range variable with different data type
     */
    void print_setup_range_variable();


    /**
     * Returns floating point type for given range variable symbol
     * \param symbol A range variable symbol
     */
    std::string get_range_var_float_type(const SymbolType& symbol);


    /**
     * Print initial block statements
     *
     * Generate the target backend code corresponding to the NMODL initial block statements
     *
     * \param node The AST Node representing a NMODL initial block
     */
    void print_initial_block(const ast::InitialBlock* node);


    /**
     * Print common code for global functions like nrn_init, nrn_cur and nrn_state
     * \param type The target backend code block type
     */
    virtual void print_global_function_common_code(BlockType type,
                                                   const std::string& function_name = "") override;


    /**
     * Print the \c nrn\_init function definition
     * \param skip_init_check \c true to generate code executing the initialization conditionally
     */
    void print_nrn_init(bool skip_init_check = true);


    /**
     * Print NMODL before / after block in target backend code
     * \param node AST node of type before/after type being printed
     * \param block_id Index of the before/after block
     */
    virtual void print_before_after_block(const ast::Block* node, size_t block_id);


    /**
     * Print nrn_constructor function definition
     *
     */
    void print_nrn_constructor() override;


    /**
     * Print nrn_destructor function definition
     *
     */
    void print_nrn_destructor() override;


    /**
     * Go through the map of \c EigenNewtonSolverBlock s and their corresponding functor names
     * and print the functor definitions before the definitions of the functions of the generated
     * file
     *
     */
    void print_functors_definitions();


    /**
     * Print nrn_alloc function definition
     *
     */
    void print_nrn_alloc() override;


    /**
     * Print watch activate function
     *
     */
    void print_watch_activate();


    /**
     * Print watch activate function
     */
    void print_watch_check();


    /**
     * Print the common code section for net receive related methods
     *
     * \param node The AST node representing the corresponding NMODL block
     * \param need_mech_inst \c true if a local \c inst variable needs to be defined in generated
     * code
     */
    void print_net_receive_common_code(const ast::Block& node, bool need_mech_inst = true);


    /**
     * Print call to \c net\_send
     * \param node The AST node representing the function call
     */
    void print_net_send_call(const ast::FunctionCall& node) override;


    /**
     * Print call to net\_move
     * \param node The AST node representing the function call
     */
    void print_net_move_call(const ast::FunctionCall& node) override;


    /**
     * Print call to net\_event
     * \param node The AST node representing the function call
     */
    void print_net_event_call(const ast::FunctionCall& node) override;


    /**
     * Print initial block in the net receive block
     */
    void print_net_init();


    /**
     * Print send event move block used in net receive as well as watch
     */
    void print_send_event_move();


    /**
     * Generate the target backend code for the \c net\_receive\_buffering function delcaration
     * \return The target code string
     */
    virtual std::string net_receive_buffering_declaration();


    /**
     * Print the target backend code for defining and checking a local \c Memb\_list variable
     */
    virtual void print_get_memb_list();


    /**
     * Print the code for the main \c net\_receive loop
     */
    virtual void print_net_receive_loop_begin();


    /**
     * Print the code for closing the main \c net\_receive loop
     */
    virtual void print_net_receive_loop_end();


    /**
     * Print kernel for buffering net_receive events
     *
     * This kernel is only needed for accelerator backends where \c net\_receive needs to be
     * executed in two stages as the actual communication must be done in the host code. \param
     * need_mech_inst \c true if the generated code needs a local inst variable to be defined
     */
    void print_net_receive_buffering(bool need_mech_inst = true);


    /**
     * Print the code related to the update of NetSendBuffer_t cnt. For GPU this needs to be done
     * with atomic operation, on CPU it's not needed.
     *
     */
    virtual void print_net_send_buffering_cnt_update() const;


    /**
     * Print statement that grows NetSendBuffering_t structure if needed.
     * This function should be overridden for backends that cannot dynamically reallocate the buffer
     */
    virtual void print_net_send_buffering_grow();


    /**
     * Print kernel for buffering net_send events
     *
     * This kernel is only needed for accelerator backends where \c net\_send needs to be executed
     * in two stages as the actual communication must be done in the host code.
     */
    void print_net_send_buffering();


    /**
     * Print \c net\_receive kernel function definition
     */
    void print_net_receive_kernel();


    /**
     * Print \c net\_receive function definition
     */
    void print_net_receive();


    /**
     * Print derivative kernel when \c derivimplicit method is used
     *
     * \param block The corresponding AST node representing an NMODL \c derivimplicit block
     */
    void print_derivimplicit_kernel(const ast::Block& block);


    /**
     * Print code block to transfer newtonspace structure to device
     */
    virtual void print_newtonspace_transfer_to_device() const;


    /****************************************************************************************/
    /*                                 Print nrn_state routine                              */
    /****************************************************************************************/


    /**
     * Print nrn_state / state update function definition
     */
    void print_nrn_state() override;


    /****************************************************************************************/
    /*                              Print nrn_cur related routines                          */
    /****************************************************************************************/


    /**
     * Print the \c nrn_current kernel
     *
     * \note nrn_cur_kernel will have two calls to nrn_current if no conductance keywords specified
     * \param node the AST node representing the NMODL breakpoint block
     */
    void print_nrn_current(const ast::BreakpointBlock& node) override;


    /**
     * Print the \c nrn\_cur kernel with NMODL \c conductance keyword provisions
     *
     * If the NMODL \c conductance keyword is used in the \c breakpoint block, then
     * CodegenCoreneuronCppVisitor::print_nrn_cur_kernel will use this printer
     *
     * \param node the AST node representing the NMODL breakpoint block
     */
    void print_nrn_cur_conductance_kernel(const ast::BreakpointBlock& node) override;


    /**
     * Print the \c nrn\_cur kernel without NMODL \c conductance keyword provisions
     *
     * If the NMODL \c conductance keyword is \b not used in the \c breakpoint block, then
     * CodegenCoreneuronCppVisitor::print_nrn_cur_kernel will use this printer
     */
    void print_nrn_cur_non_conductance_kernel() override;


    /**
     * Print main body of nrn_cur function
     * \param node the AST node representing the NMODL breakpoint block
     */
    void print_nrn_cur_kernel(const ast::BreakpointBlock& node) override;


    /**
     * Print fast membrane current calculation code
     */
    virtual void print_fast_imem_calculation() override;


    /**
     * Print nrn_cur / current update function definition
     */
    void print_nrn_cur() override;


    /****************************************************************************************/
    /*                              Main code printing entry points                         */
    /****************************************************************************************/


    /**
     * Print all includes
     *
     */
    void print_headers_include() override;


    /**
     * Print start of namespaces
     *
     */
    void print_namespace_begin() override;


    /**
     * Print end of namespaces
     *
     */
    void print_namespace_end() override;


    /**
     * Print common getters
     *
     */
    void print_common_getters();


    /**
     * Print all classes
     * \param print_initializers Whether to include default values.
     */
    void print_data_structures(bool print_initializers) override;


    /**
     * Set v_unused (voltage) for NRN_PRCELLSTATE feature
     */
    void print_v_unused() const override;


    /**
     * Set g_unused (conductance) for NRN_PRCELLSTATE feature
     */
    void print_g_unused() const override;


    /**
     * Print all compute functions for every backend
     *
     */
    virtual void print_compute_functions() override;


    /**
     * Print entry point to code generation
     *
     */
    virtual void print_codegen_routines() override;


    /****************************************************************************************/
    /*                            Overloaded visitor routines                               */
    /****************************************************************************************/


    void visit_derivimplicit_callback(const ast::DerivimplicitCallback& node) override;
    void visit_eigen_newton_solver_block(const ast::EigenNewtonSolverBlock& node) override;
    void visit_eigen_linear_solver_block(const ast::EigenLinearSolverBlock& node) override;
    void visit_for_netcon(const ast::ForNetcon& node) override;
    virtual void visit_watch_statement(const ast::WatchStatement& node) override;


    /**
     * Print prototype declarations of functions or procedures
     * \tparam T   The AST node type of the node (must be of nmodl::ast::Ast or subclass)
     * \param node The AST node representing the function or procedure block
     * \param name A user defined name for the function
     */
    template <typename T>
    void print_function_declaration(const T& node, const std::string& name);


  public:
    /**
     * \brief Constructs the C++ code generator visitor
     *
     * This constructor instantiates an NMODL C++ code generator and allows writing generated code
     * directly to a file in \c [output_dir]/[mod_filename].cpp.
     *
     * \note No code generation is performed at this stage. Since the code
     * generator classes are all based on \c AstVisitor the AST must be visited using e.g. \c
     * visit_program in order to generate the C++ code corresponding to the AST.
     *
     * \param mod_filename The name of the model for which code should be generated.
     *                     It is used for constructing an output filename.
     * \param output_dir   The directory where target C++ file should be generated.
     * \param float_type   The float type to use in the generated code. The string will be used
     *                     as-is in the target code. This defaults to \c double.
     */
    CodegenCoreneuronCppVisitor(std::string mod_filename,
                                const std::string& output_dir,
                                std::string float_type,
                                const bool optimize_ionvar_copies)
        : CodegenCppVisitor(mod_filename, output_dir, float_type, optimize_ionvar_copies) {}

    /**
     * \copybrief nmodl::codegen::CodegenCoreneuronCppVisitor
     *
     * This constructor instantiates an NMODL C++ code generator and allows writing generated code
     * into an output stream.
     *
     * \note No code generation is performed at this stage. Since the code
     * generator classes are all based on \c AstVisitor the AST must be visited using e.g. \c
     * visit_program in order to generate the C++ code corresponding to the AST.
     *
     * \param mod_filename The name of the model for which code should be generated.
     *                     It is used for constructing an output filename.
     * \param stream       The output stream onto which to write the generated code
     * \param float_type   The float type to use in the generated code. The string will be used
     *                     as-is in the target code. This defaults to \c double.
     */
    CodegenCoreneuronCppVisitor(std::string mod_filename,
                                std::ostream& stream,
                                std::string float_type,
                                const bool optimize_ionvar_copies)
        : CodegenCppVisitor(mod_filename, stream, float_type, optimize_ionvar_copies) {}


    /****************************************************************************************/
    /*          Public printing routines for code generation for use in unit tests          */
    /****************************************************************************************/


    /**
     * Print the function that initialize instance structure
     */
    void print_instance_variable_setup();


    /**
     * Print the structure that wraps all range and int variables required for the NMODL
     *
     * \param print_initializers Whether or not default values for variables
     *                           be included in the struct declaration.
     */
    void print_mechanism_range_var_structure(bool print_initializers) override;
};


/**
 * \details If there is an argument with name (say alpha) same as range variable (say alpha),
 * we want to avoid it being printed as instance->alpha. And hence we disable variable
 * name lookup during prototype declaration. Note that the name of procedure can be
 * different in case of table statement.
 */
template <typename T>
void CodegenCoreneuronCppVisitor::print_function_declaration(const T& node,
                                                             const std::string& name) {
    enable_variable_name_lookup = false;
    auto type = default_float_data_type();

    // internal and user provided arguments
    auto internal_params = internal_method_parameters();
    const auto& params = node.get_parameters();
    for (const auto& param: params) {
        internal_params.emplace_back("", type, "", param.get()->get_node_name());
    }

    // procedures have "int" return type by default
    const char* return_type = "int";
    if (node.is_function_block()) {
        return_type = default_float_data_type();
    }

    print_device_method_annotation();
    printer->add_indent();
    printer->fmt_text("inline {} {}({})",
                      return_type,
                      method_name(name),
                      get_parameter_str(internal_params));

    enable_variable_name_lookup = true;
}

/** \} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl
