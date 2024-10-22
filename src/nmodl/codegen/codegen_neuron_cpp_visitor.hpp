/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \dir
 * \brief Code generation backend implementations for NEURON
 *
 * \file
 * \brief \copybrief nmodl::codegen::CodegenNeuronCppVisitor
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


/// encapsulates code generation backend implementations
namespace nmodl {

namespace codegen {


using printer::CodePrinter;


/**
 * @brief Enum to switch between HOC and Python wrappers for functions and
 *        procedures defined in mechanisms
 *
 */
enum InterpreterWrapper { HOC, Python };


/**
 * \defgroup codegen_backends Codegen Backends
 * \ingroup codegen
 * \brief Code generation backends for NEURON
 * \{
 */

struct ThreadVariableInfo {
    const std::shared_ptr<symtab::Symbol> symbol;

    /** There `index` global variables ahead of this one. If one counts array
     *  global variables as one variable.
     */
    size_t index;

    /** The global variables ahead of this one require `offset` doubles to
     *  store.
     */
    size_t offset;
};

inline std::string get_name(const ThreadVariableInfo& var) {
    return var.symbol->get_name();
}


/**
 * \class CodegenNeuronCppVisitor
 * \brief %Visitor for printing C++ code compatible with legacy api of NEURON
 *
 * \todo
 *  - Handle define statement (i.e. macros)
 *  - If there is a return statement in the verbatim block
 *    of inlined function then it will be error. Need better
 *    error checking. For example, see netstim.mod where we
 *    have removed return from verbatim block.
 */
class CodegenNeuronCppVisitor: public CodegenCppVisitor {
  public:
    using CodegenCppVisitor::CodegenCppVisitor;

  protected:
    /****************************************************************************************/
    /*                                    Member variables                                  */
    /****************************************************************************************/

    /**
     * GLOBAL variables in THREADSAFE MOD files that are not read-only are
     * converted to thread variables. This is the list of all such variables.
     */
    std::vector<ThreadVariableInfo> codegen_thread_variables;


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
    std::string backend_name() const override;

    /**
     * Name of the threaded table checking function
     */
    std::string table_thread_function_name() const;


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


    /****************************************************************************************/
    /*                                Backend specific routines                             */
    /****************************************************************************************/


    /**
     * Print atomic update pragma for reduction statements
     */
    void print_atomic_reduction_pragma() override;


    /**
     * Check if ion variable copies should be avoided
     */
    bool optimize_ion_variable_copies() const override;

    /****************************************************************************************/
    /*                         Printing routines for code generation                        */
    /****************************************************************************************/

    /**
     * Print NET_RECEIVE{ INITIAL{ ... }} block.
     */
    void print_net_init();

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

    void print_function_table_call(const ast::FunctionCall& node) override;

    /**
     * Print `net_receive` call-back.
     */
    void print_net_receive();
    void print_net_receive_common_code();
    ParamVector net_receive_args();

    /**
     * Print code to register the call-back for the NET_RECEIVE block.
     */
    void print_net_receive_registration();

    /**
     * Print POINT_PROCESS related functions
     * Wrap external NEURON functions related to POINT_PROCESS mechanisms
     *
     */
    void print_point_process_function_definitions();

    /**
     * Print NEURON functions related to setting global variables of the mechanism
     *
     */
    void print_setdata_functions();


    /**
     * Print function and procedures prototype declaration
     */
    void print_function_prototypes() override;


    /**
     * Print function and procedures prototype definitions.
     *
     * This includes the HOC/Python wrappers.
     */
    void print_function_definitions();


    /**
     * Print all `check_*` function declarations
     */
    void print_check_table_entrypoint();


    void print_function_or_procedure(const ast::Block& node,
                                     const std::string& name,
                                     const std::unordered_set<CppObjectSpecifier>& specifiers = {
                                         CppObjectSpecifier::Inline}) override;


    /**
     * Common helper function to help printing function or procedure blocks
     * \param node the AST node representing the function or procedure in NMODL
     */
    void print_function_procedure_helper(const ast::Block& node) override;


    /** Print the wrapper for calling FUNCION/PROCEDURES from HOC/Py.
     *
     *  Usually the function is made up of the following parts:
     *    * Print setup code `inst`, etc.
     *    * Print code to call the function and return.
     */
    void print_hoc_py_wrapper(const ast::Block* function_or_procedure_block,
                              InterpreterWrapper wrapper_type);

    /** Print the setup code for HOC/Py wrapper.
     */
    void print_hoc_py_wrapper_setup(const ast::Block* function_or_procedure_block,
                                    InterpreterWrapper wrapper_type);


    /** Print the code that calls the impl from the HOC/Py wrapper.
     */
    void print_hoc_py_wrapper_call_impl(const ast::Block* function_or_procedure_block,
                                        InterpreterWrapper wrapper_type);

    /** Return the wrapper signature.
     *
     * Everything without the `{` or `;`. Roughly, as an example:
     *      <return_type> <function_name>(<internal_args>, <args>)
     *
     * were `<internal_args> is the list of arguments required by the
     * codegen to be passed along, while <args> are the arguments of
     * of the function as they appear in the MOD file.
     */
    std::string hoc_py_wrapper_signature(const ast::Block* function_or_procedure_block,
                                         InterpreterWrapper wrapper_type);

    void print_hoc_py_wrapper_function_definitions();

    /**
     * Prints the callbacks required for LONGITUDINAL_DIFFUSION.
     */
    void print_longitudinal_diffusion_callbacks();

    /**
     * Parameters for what NEURON calls `ldifusfunc1_t`.
     */
    ParamVector ldifusfunc1_parameters() const;

    /**
     * Parameters for what NEURON calls `ldifusfunc3_t`.
     */
    ParamVector ldifusfunc3_parameters() const;


    /****************************************************************************************/
    /*                             Code-specific helper routines                            */
    /****************************************************************************************/

    void add_variable_tqitem(std::vector<IndexVariableInfo>& variables) override;
    void add_variable_point_process(std::vector<IndexVariableInfo>& variables) override;

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
    const std::string external_method_arguments() noexcept override;


    /**
     * Parameters for functions in generated code that are called back from external code
     *
     * Functions registered in NEURON during initialization for callback must adhere to a prescribed
     * calling convention. This method generates the string representing the function parameters for
     * these externally called functions.
     * \param table
     * \return      A string representing the parameters of the function
     */
    const ParamVector external_method_parameters(bool table = false) noexcept override;


    /**
     * Arguments for "_threadargs_" macro in neuron implementation
     */
    std::string nrn_thread_arguments() const override;


    /**
     * Arguments for "_threadargs_" macro in neuron implementation
     */
    std::string nrn_thread_internal_arguments() override;

    std::pair<ParamVector, ParamVector> function_table_parameters(
        const ast::FunctionTableBlock& /* node */) override;


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


    void append_conc_write_statements(std::vector<ShadowUseStatement>& statements,
                                      const Ion& ion,
                                      const std::string& concentration) override;

    /**
     * All functions and procedures need a \c _hoc_<func_or_proc_name> to be available to the HOC
     * interpreter
     */
    std::string hoc_function_name(const std::string& function_or_procedure_name) const;


    /**
     * Get the signature of the \c _hoc_<func_or_proc_name> function
     */
    std::string hoc_function_signature(const std::string& function_or_procedure_name) const;


    /**
     * In non POINT_PROCESS mechanisms all functions and procedures need a \c
     * _py_<func_or_proc_name> to be available to the HOC interpreter
     */
    std::string py_function_name(const std::string& function_or_procedure_name) const;

    /**
     * Get the signature of the \c _npy_<func_or_proc_name> function
     */
    std::string py_function_signature(const std::string& function_or_procedure_name) const;


    /****************************************************************************************/
    /*                  Code-specific printing routines for code generations                */
    /****************************************************************************************/


    std::string namespace_name() override;


    /****************************************************************************************/
    /*                         Routines for returning variable name                         */
    /****************************************************************************************/


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
     * Determine the C++ string to print for thread variables.
     *
     * \param var_info Identifies the thread variable, typically an instance of
     *                 `codegen_thread_variables`.
     * \param use_instance Should the variable be accessed via instance or data array
     */
    std::string thread_variable_name(const ThreadVariableInfo& var_info,
                                     bool use_instance = true) const;


    /**
     * Determine variable name in the structure of mechanism properties
     *
     * \param name         Variable name that is being printed
     * \param use_instance Should the variable be accessed via instance or data array
     * \return             The C++ string representing the variable.
     * thread structure
     */
    std::string get_variable_name(const std::string& name, bool use_instance = true) const override;


    /****************************************************************************************/
    /*                      Main printing routines for code generation                      */
    /****************************************************************************************/


    /**
     * Print standard C/C++ includes
     */
    void print_standard_includes() override;


    /**
     * Print includes from NEURON
     */
    void print_neuron_includes();


    void print_sdlists_init(bool print_initializers) override;


    /**
     * Print the structure that wraps all global variables used in the NMODL
     *
     * \param print_initializers Whether to include default values in the struct
     *                           definition (true: int foo{42}; false: int foo;)
     */
    void print_mechanism_global_var_structure(bool print_initializers) override;


    /**
     * Print byte arrays that register scalar and vector variables for hoc interface
     *
     */
    void print_global_variables_for_hoc() override;

    /**
     * Print functions for EXTERNAL use.
     *
     */
    void print_global_var_external_access();

    /** Print global struct with default value of RANGE PARAMETERs.
     */
    void print_global_param_default_values();

    /**
     * Print the mechanism registration function
     *
     */
    void print_mechanism_register() override;

    /** Function body for anything not SUFFIX nothing. */
    void print_mechanism_register_regular();

    /** Function body for SUFFIX nothing. */
    void print_mechanism_register_nothing();

    /**
     * Print thread variable (de-)initialization functions.
     */
    void print_thread_memory_callbacks();

    /**
     * Print common code for global functions like nrn_init, nrn_cur and nrn_state
     * \param type The target backend code block type
     */
    void print_global_function_common_code(BlockType type,
                                           const std::string& function_name = "") override;

    /**
     * Prints setup code for entrypoints from NEURON.
     *
     * The entrypoints typically receive a `sorted_token` and a bunch of other things, which then
     * need to be converted into the default arguments for functions called (recursively) from the
     * entrypoint.
     *
     * This variation prints the fast entrypoint, where NEURON is fully initialized and setup.
     */
    void print_entrypoint_setup_code_from_memb_list();


    /**
     * Prints setup code for entrypoints NEURON.
     *
     * See `print_entrypoint_setup_code_from_memb_list`. This variation should be used when one only
     * has access to a `Prop`, but not the full `Memb_list`.
     */
    void print_entrypoint_setup_code_from_prop();


    /**
     * Print the \c nrn\_init function definition
     * \param skip_init_check \c true to generate code executing the initialization conditionally
     */
    void print_nrn_init(bool skip_init_check = true);

    /** Print the initial block. */
    void print_initial_block(const ast::InitialBlock* node);

    /**
     * Print nrn_constructor function definition
     *
     */
    void print_nrn_constructor() override;
    void print_nrn_constructor_declaration();

    /**
     * Print nrn_destructor function definition
     *
     */
    void print_nrn_destructor() override;
    void print_nrn_destructor_declaration();


    /**
     * Print nrn_alloc function definition
     *
     */
    void print_nrn_alloc() override;


    /**
     * Print nrn_jacob function definition
     *
     */
    void print_nrn_jacob();


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

    std::string nrn_current_arguments();
    ParamVector nrn_current_parameters();

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
    void print_fast_imem_calculation() override;


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
     * Print all NEURON macros
     *
     */
    void print_macro_definitions();


    /**
     * Print extern declarations for neuron global variables.
     *
     * Examples include `celsius`.
     */
    void print_neuron_global_variable_declarations();


    /**
     * Print NEURON global variable macros
     *
     */
    void print_global_macros();


    /**
     * Print mechanism variables' related macros
     *
     */
    void print_mechanism_variables_macros();


    /**
     * Print all classes
     * \param print_initializers Whether to include default values.
     */
    void print_data_structures(bool print_initializers) override;

    /** Print `make_*_instance`.
     */
    void print_make_instance() const;

    /** Print `make_*_node_data`.
     */
    void print_make_node_data() const;

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
    void print_compute_functions() override;


    /**
     * Print entry point to code generation
     *
     */
    void print_codegen_routines() override;

    /** Anything not SUFFIX nothing. */
    void print_codegen_routines_regular();

    /** SUFFIX nothing is special. */
    void print_codegen_routines_nothing();


    void print_ion_variable() override;


    /****************************************************************************************/
    /*                            Overloaded visitor routines                               */
    /****************************************************************************************/


    void visit_watch_statement(const ast::WatchStatement& node) override;
    void visit_for_netcon(const ast::ForNetcon& node) override;
    void visit_longitudinal_diffusion_block(const ast::LongitudinalDiffusionBlock& node) override;
    void visit_lon_diffuse(const ast::LonDiffuse& node) override;


  public:
    /****************************************************************************************/
    /*          Public printing routines for code generation for use in unit tests          */
    /****************************************************************************************/
    ParamVector functor_params() override;

    /**
     * Print the structure that wraps all range and int variables required for the NMODL
     *
     * \param print_initializers Whether or not default values for variables
     *                           be included in the struct declaration.
     */
    void print_mechanism_range_var_structure(bool print_initializers) override;

    /**
     * Print the structure that wraps all node variables required for the NMODL.
     *
     * \param print_initializers Whether or not default values for variables
     *                           be included in the struct declaration.
     */
    void print_node_data_structure(bool print_initializers);

    /**
     * Print the data structure used to access thread variables.
     */
    void print_thread_variables_structure(bool print_initializers);
};


/** \} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl
