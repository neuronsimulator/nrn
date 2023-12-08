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

#include "codegen/codegen_info.hpp"
#include "codegen/codegen_naming.hpp"
#include "printer/code_printer.hpp"
#include "symtab/symbol_table.hpp"
#include "utils/logger.hpp"
#include "visitors/ast_visitor.hpp"
#include <codegen/codegen_cpp_visitor.hpp>


/// encapsulates code generation backend implementations
namespace nmodl {

namespace codegen {


using printer::CodePrinter;


/**
 * \defgroup codegen_backends Codegen Backends
 * \ingroup codegen
 * \brief Code generation backends for NEURON
 * \{
 */

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
    virtual void print_atomic_reduction_pragma() override;


    /****************************************************************************************/
    /*                         Printing routines for code generation                        */
    /****************************************************************************************/


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
     * Print function and procedures prototype declaration
     */
    void print_function_prototypes() override;


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
     * Process a verbatim block for possible variable renaming
     * \param text The verbatim code to be processed
     * \return     The code with all variables renamed as needed
     */
    std::string process_verbatim_text(std::string const& text) override;


    /**
     * Arguments for register_mech or point_register_mech function
     */
    std::string register_mechanism_arguments() const override;


    /****************************************************************************************/
    /*                  Code-specific printing routines for code generations                */
    /****************************************************************************************/


    /**
     * Prints the start of the \c neuron namespace
     */
    void print_namespace_start() override;


    /**
     * Prints the end of the \c neuron namespace
     */
    void print_namespace_stop() override;


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
     * Print the mechanism registration function
     *
     */
    void print_mechanism_register() override;


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
     * Print all NEURON macros
     *
     */
    void print_macro_definitions();


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
    void print_codegen_routines() override;


    /****************************************************************************************/
    /*                            Overloaded visitor routines                               */
    /****************************************************************************************/


    virtual void visit_solution_expression(const ast::SolutionExpression& node) override;
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
    CodegenNeuronCppVisitor(std::string mod_filename,
                            const std::string& output_dir,
                            std::string float_type,
                            const bool optimize_ionvar_copies)
        : CodegenCppVisitor(mod_filename, output_dir, float_type, optimize_ionvar_copies) {}

    /**
     * \copybrief nmodl::codegen::CodegenNeuronCppVisitor
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
    CodegenNeuronCppVisitor(std::string mod_filename,
                            std::ostream& stream,
                            std::string float_type,
                            const bool optimize_ionvar_copies)
        : CodegenCppVisitor(mod_filename, stream, float_type, optimize_ionvar_copies) {}


    /****************************************************************************************/
    /*          Public printing routines for code generation for use in unit tests          */
    /****************************************************************************************/


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
void CodegenNeuronCppVisitor::print_function_declaration(const T& node, const std::string& name) {
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

    /// TODO: Edit for NEURON
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
