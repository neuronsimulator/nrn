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
 * \brief \copybrief nmodl::codegen::CodegenCppVisitor
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

/// encapsulates code generation backend implementations
namespace nmodl {

namespace codegen {

/**
 * \defgroup codegen Code Generation Implementation
 * \brief Implementations of code generation backends
 *
 * \defgroup codegen_details Codegen Helpers
 * \ingroup codegen
 * \brief Helper routines/types for code generation
 * \{
 */

/**
 * \enum BlockType
 * \brief Helper to represent various block types
 *
 * Note: do not assign integers to these enums
 *
 */
enum class BlockType {
    /// initial block
    Initial,

    /// constructor block
    Constructor,

    /// destructor block
    Destructor,

    /// breakpoint block
    Equation,

    /// derivative block
    State,

    /// watch block
    Watch,

    /// net_receive block
    NetReceive,

    /// before / after block
    BeforeAfter,

    /// fake ending block type for loops on the enums. Keep it at the end
    BlockTypeEnd
};


/**
 * \enum MemberType
 * \brief Helper to represent various variables types
 *
 */
enum class MemberType {
    /// index / int variables
    index,

    /// range / double variables
    range,

    /// global variables
    global,

    /// thread variables
    thread
};


/**
 * \class IndexVariableInfo
 * \brief Helper to represent information about index/int variables
 *
 */
struct IndexVariableInfo {
    /// symbol for the variable
    const std::shared_ptr<symtab::Symbol> symbol;

    /// if variable resides in vdata field of NrnThread
    /// typically true for bbcore pointer
    bool is_vdata = false;

    /// if this is pure index (e.g. style_ion) variables is directly
    /// index and shouldn't be printed with data/vdata
    bool is_index = false;

    /// if this is an integer (e.g. tqitem, point_process) variable which
    /// is printed as array accesses
    bool is_integer = false;

    /// if the variable is qualified as constant (this is property of IndexVariable)
    bool is_constant = false;

    explicit IndexVariableInfo(std::shared_ptr<symtab::Symbol> symbol,
                               bool is_vdata = false,
                               bool is_index = false,
                               bool is_integer = false)
        : symbol(std::move(symbol))
        , is_vdata(is_vdata)
        , is_index(is_index)
        , is_integer(is_integer) {}
};


/**
 * \class ShadowUseStatement
 * \brief Represents ion write statement during code generation
 *
 * Ion update statement needs use of shadow vectors for certain backends
 * as atomics operations are not supported on cpu backend.
 *
 * \todo If shadow_lhs is empty then we assume shadow statement not required
 */
struct ShadowUseStatement {
    std::string lhs;
    std::string op;
    std::string rhs;
};

/** \} */  // end of codegen_details


using printer::CodePrinter;


/**
 * \defgroup codegen_backends Codegen Backends
 * \ingroup codegen
 * \brief Code generation backends for CoreNEURON
 * \{
 */

/**
 * \class CodegenCppVisitor
 * \brief %Visitor for printing C++ code compatible with legacy api of CoreNEURON
 *
 * \todo
 *  - Handle define statement (i.e. macros)
 *  - If there is a return statement in the verbatim block
 *    of inlined function then it will be error. Need better
 *    error checking. For example, see netstim.mod where we
 *    have removed return from verbatim block.
 */
class CodegenCppVisitor: public visitor::ConstAstVisitor {
  protected:
    using SymbolType = std::shared_ptr<symtab::Symbol>;


    /**
     * A vector of parameters represented by a 4-tuple of strings:
     *
     * - type qualifier (e.g. \c const)
     * - type (e.g. \c double)
     * - pointer qualifier (e.g. \c \_\_restrict\_\_)
     * - parameter name (e.g. \c data)
     *
     */
    using ParamVector = std::vector<std::tuple<std::string, std::string, std::string, std::string>>;


    /****************************************************************************************/
    /*                                    Member variables                                  */
    /****************************************************************************************/

    /**
     * Code printer object for target (C++)
     */
    std::unique_ptr<CodePrinter> printer;


    /**
     * Name of mod file (without .mod suffix)
     */
    std::string mod_filename;


    /**
     * Data type of floating point variables
     */
    std::string float_type = codegen::naming::DEFAULT_FLOAT_TYPE;


    /**
     * Flag to indicate if visitor should avoid ion variable copies
     */
    bool optimize_ionvar_copies = true;


    /**
     * All ast information for code generation
     */
    codegen::CodegenInfo info;


    /**
     * Symbol table for the program
     */
    symtab::SymbolTable* program_symtab = nullptr;


    /**
     * All float variables for the model
     */
    std::vector<SymbolType> codegen_float_variables;


    /**
     * All int variables for the model
     */
    std::vector<IndexVariableInfo> codegen_int_variables;


    /**
     * All global variables for the model
     * \todo: this has become different than CodegenInfo
     */
    std::vector<SymbolType> codegen_global_variables;


    /**
     * Variable name should be converted to instance name (but not for function arguments)
     */
    bool enable_variable_name_lookup = true;


    /**
     * \c true if currently net_receive block being printed
     */
    bool printing_net_receive = false;


    /**
     * \c true if currently initial block of net_receive being printed
     */
    bool printing_net_init = false;


    /**
     * \c true if currently printing top level verbatim blocks
     */
    bool printing_top_verbatim_blocks = false;


    /**
     * \c true if internal method call was encountered while processing verbatim block
     */
    bool internal_method_call_encountered = false;


    /**
     * Index of watch statement being printed
     */
    int current_watch_statement = 0;


    /****************************************************************************************/
    /*                              Generic information getters                             */
    /****************************************************************************************/

    /**
     * Return Nmodl language version
     * \return A version
     */
    std::string nmodl_version() const noexcept {
        return codegen::naming::NMODL_VERSION;
    }


    /**
     * Name of the simulator the code was generated for
     */
    virtual std::string simulator_name() = 0;


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
     * Name of the code generation backend
     */
    virtual std::string backend_name() const = 0;


    /**
     * Data type for the local variables
     */
    const char* local_var_type() const noexcept {
        return codegen::naming::DEFAULT_LOCAL_VAR_TYPE;
    }


    /**
     * Check if a semicolon is required at the end of given statement
     * \param node The AST Statement node to check
     * \return     \c true if this Statement requires a semicolon
     */
    static bool need_semicolon(const ast::Statement& node);


    /**
     * Default data type for floating point elements
     */
    const char* default_float_data_type() const noexcept {
        return codegen::naming::DEFAULT_FLOAT_TYPE;
    }


    /**
     * Data type for floating point elements specified on command line
     */
    const std::string& float_data_type() const noexcept {
        return float_type;
    }


    /**
     * Default data type for integer (offset) elements
     */
    const char* default_int_data_type() const noexcept {
        return codegen::naming::DEFAULT_INTEGER_TYPE;
    }


    /**
     * Operator for rhs vector update (matrix update)
     */
    const char* operator_for_rhs() const noexcept {
        return info.electrode_current ? "+=" : "-=";
    }


    /**
     * Operator for diagonal vector update (matrix update)
     */
    const char* operator_for_d() const noexcept {
        return info.electrode_current ? "-=" : "+=";
    }

    /**
     * Name of channel info variable
     *
     */
    std::string get_channel_info_var_name() const noexcept {
        return std::string("mechanism_info");
    }


    /****************************************************************************************/
    /*                     Common helper routines accross codegen functions                 */
    /****************************************************************************************/


    /**
     * Generate the string representing the procedure parameter declaration
     *
     * The procedure parameters are stored in a vector of 4-tuples each representing a parameter.
     *
     * \param params The parameters that should be concatenated into the function parameter
     * declaration
     * \return The string representing the declaration of function parameters
     */
    static std::string get_parameter_str(const ParamVector& params);


    /**
     * Check if function or procedure node has parameter with given name
     *
     * \tparam T Node type (either procedure or function)
     * \param node AST node (either procedure or function)
     * \param name Name of parameter
     * \return True if argument with name exist
     */
    template <typename T>
    bool has_parameter_of_name(const T& node, const std::string& name);


    /**
     * Check if given statement should be skipped during code generation
     * \param node The AST Statement node to check
     * \return     \c true if this Statement is to be skipped
     */
    static bool statement_to_skip(const ast::Statement& node);


    /**
     * Check if net_send_buffer is required
     */
    bool net_send_buffer_required() const noexcept;


    /**
     * Check if net receive/send buffering kernels required
     */
    bool net_receive_buffering_required() const noexcept;


    /**
     * Check if nrn_state function is required
     */
    bool nrn_state_required() const noexcept;


    /**
     * Check if nrn_cur function is required
     */
    bool nrn_cur_required() const noexcept;


    /**
     * Check if net_receive function is required
     */
    bool net_receive_required() const noexcept;


    /**
     * Check if setup_range_variable function is required
     * \return
     */
    bool range_variable_setup_required() const noexcept;


    /**
     * Check if net_receive node exist
     */
    bool net_receive_exist() const noexcept;


    /**
     * Check if breakpoint node exist
     */
    bool breakpoint_exist() const noexcept;


    /**
     * Check if given method is defined in this model
     * \param name The name of the method to check
     * \return     \c true if the method is defined
     */
    bool defined_method(const std::string& name) const;


    /**
     * Checks if given function name is \c net_send
     * \param name The function name to check
     * \return     \c true if the function is net_send
     */
    bool is_net_send(const std::string& name) const noexcept {
        return name == codegen::naming::NET_SEND_METHOD;
    }


    /**
     * Checks if given function name is \c net_move
     * \param name The function name to check
     * \return     \c true if the function is net_move
     */
    bool is_net_move(const std::string& name) const noexcept {
        return name == codegen::naming::NET_MOVE_METHOD;
    }


    /**
     * Checks if given function name is \c net_event
     * \param name The function name to check
     * \return     \c true if the function is net_event
     */
    bool is_net_event(const std::string& name) const noexcept {
        return name == codegen::naming::NET_EVENT_METHOD;
    }


    /**
     * Determine the position in the data array for a given float variable
     * \param name The name of a float variable
     * \return     The position index in the data array
     */
    virtual int position_of_float_var(const std::string& name) const = 0;


    /**
     * Determine the position in the data array for a given int variable
     * \param name The name of an int variable
     * \return     The position index in the data array
     */
    virtual int position_of_int_var(const std::string& name) const = 0;


    /**
     * Number of float variables in the model
     */
    int float_variables_size() const;


    /**
     * Number of integer variables in the model
     */
    int int_variables_size() const;


    /**
     * Convert a given \c double value to its string representation
     * \param value The number to convert given as string as it is parsed by the modfile
     * \return      Its string representation
     */
    std::string format_double_string(const std::string& value);


    /**
     * Convert a given \c float value to its string representation
     * \param value The number to convert given as string as it is parsed by the modfile
     * \return      Its string representation
     */
    std::string format_float_string(const std::string& value);


    /**
     * populate all index semantics needed for registration with coreneuron
     */
    void update_index_semantics();


    /**
     * Determine all \c float variables required during code generation
     * \return A \c vector of \c float variables
     */
    std::vector<SymbolType> get_float_variables() const;


    /**
     * Determine all \c int variables required during code generation
     * \return A \c vector of \c int variables
     */
    std::vector<IndexVariableInfo> get_int_variables();


    /****************************************************************************************/
    /*                                Backend specific routines                             */
    /****************************************************************************************/


    /**
     * Print atomic update pragma for reduction statements
     */
    virtual void print_atomic_reduction_pragma() = 0;


    /**
     * Instantiate global var instance
     *
     * For C++ code generation this is empty
     * \return ""
     */
    virtual void print_global_var_struct_decl();


    /****************************************************************************************/
    /*                         Printing routines for code generation                        */
    /****************************************************************************************/


    /**
     * Print any statement block in nmodl with option to (not) print braces
     *
     * The individual statements (of type nmodl::ast::Statement) in the StatementBlock are printed
     * by accepting \c this visistor.
     *
     * \param node        A (possibly empty) statement block AST node
     * \param open_brace  Print an opening brace if \c false
     * \param close_brace Print a closing brace if \c true
     */
    void print_statement_block(const ast::StatementBlock& node,
                               bool open_brace = true,
                               bool close_brace = true);


    /**
     * Print call to internal or external function
     * \param node The AST node representing a function call
     */
    virtual void print_function_call(const ast::FunctionCall& node);

    /**
     * Print call to \c net\_send
     * \param node The AST node representing the function call
     */
    virtual void print_net_send_call(const ast::FunctionCall& node) = 0;


    /**
     * Print call to net\_move
     * \param node The AST node representing the function call
     */
    virtual void print_net_move_call(const ast::FunctionCall& node) = 0;


    /**
     * Print call to net\_event
     * \param node The AST node representing the function call
     */
    virtual void print_net_event_call(const ast::FunctionCall& node) = 0;

    /**
     * Print function and procedures prototype declaration
     */
    virtual void print_function_prototypes() = 0;


    /**
     * Print nmodl function or procedure (common code)
     * \param node the AST node representing the function or procedure in NMODL
     * \param name the name of the function or procedure
     */
    virtual void print_function_or_procedure(const ast::Block& node, const std::string& name) = 0;


    /**
     * Common helper function to help printing function or procedure blocks
     * \param node the AST node representing the function or procedure in NMODL
     */
    virtual void print_function_procedure_helper(const ast::Block& node) = 0;


    /**
     * Print NMODL procedure in target backend code
     * \param node
     */
    virtual void print_procedure(const ast::ProcedureBlock& node) = 0;


    /**
     * Print NMODL function in target backend code
     * \param node
     */
    virtual void print_function(const ast::FunctionBlock& node) = 0;


    /**
     * Rename function/procedure arguments that conflict with default arguments
     */
    void rename_function_arguments();


    /**
     * Print the items in a vector as a list
     *
     * This function prints a given vector of elements as a list with given separator onto the
     * current printer. Elements are expected to be of type nmodl::ast::Ast and are printed by being
     * visited. Care is taken to omit the separator after the the last element.
     *
     * \tparam T The element type in the vector, which must be of type nmodl::ast::Ast
     * \param  elements The vector of elements to be printed
     * \param  separator The separator string to print between all elements
     * \param  prefix A prefix string to print before each element
     */
    template <typename T>
    void print_vector_elements(const std::vector<T>& elements,
                               const std::string& separator,
                               const std::string& prefix = "");


    /****************************************************************************************/
    /*                             Code-specific helper routines                            */
    /****************************************************************************************/


    /**
     * Arguments for functions that are defined and used internally.
     * \return the method arguments
     */
    virtual std::string internal_method_arguments() = 0;


    /**
     * Parameters for internally defined functions
     * \return the method parameters
     */
    virtual ParamVector internal_method_parameters() = 0;


    /**
     * Arguments for external functions called from generated code
     * \return A string representing the arguments passed to an external function
     */
    virtual const char* external_method_arguments() noexcept = 0;


    /**
     * Parameters for functions in generated code that are called back from external code
     *
     * Functions registered in NEURON during initialization for callback must adhere to a prescribed
     * calling convention. This method generates the string representing the function parameters for
     * these externally called functions.
     * \param table
     * \return      A string representing the parameters of the function
     */
    virtual const char* external_method_parameters(bool table = false) noexcept = 0;


    /**
     * Arguments for "_threadargs_" macro in neuron implementation
     */
    virtual std::string nrn_thread_arguments() const = 0;


    /**
     * Arguments for "_threadargs_" macro in neuron implementation
     */
    virtual std::string nrn_thread_internal_arguments() = 0;

    /**
     * Process a verbatim block for possible variable renaming
     * \param text The verbatim code to be processed
     * \return     The code with all variables renamed as needed
     */
    virtual std::string process_verbatim_text(std::string const& text) = 0;


    /**
     * Arguments for register_mech or point_register_mech function
     */
    virtual std::string register_mechanism_arguments() const = 0;


    /**
     * Add quotes to string to be output
     *
     * \param text The string to be quoted
     * \return     The same string with double-quotes pre- and postfixed
     */
    std::string add_escape_quote(const std::string& text) const {
        return "\"" + text + "\"";
    }


    /**
     * Constructs the name of a function or procedure
     * \param name The name of the function or procedure
     * \return     The name of the function or procedure postfixed with the model name
     */
    std::string method_name(const std::string& name) const {
        return name + "_" + info.mod_suffix;
    }


    /**
     * Creates a temporary symbol
     * \param name The name of the symbol
     * \return     A symbol based on the given name
     */
    SymbolType make_symbol(const std::string& name) const {
        return std::make_shared<symtab::Symbol>(name, ModToken());
    }


    /****************************************************************************************/
    /*                  Code-specific printing routines for code generations                */
    /****************************************************************************************/


    /**
     * Prints the start of the simulator namespace
     */
    virtual void print_namespace_start() = 0;


    /**
     * Prints the end of the simulator namespace
     */
    virtual void print_namespace_stop() = 0;


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
    virtual std::string float_variable_name(const SymbolType& symbol, bool use_instance) const = 0;


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
    virtual std::string int_variable_name(const IndexVariableInfo& symbol,
                                          const std::string& name,
                                          bool use_instance) const = 0;


    /**
     * Determine the variable name for a global variable given its symbol
     * \param symbol The symbol of a variable for which we want to obtain its name
     * \param use_instance Should the variable be accessed via the (host-only)
     * global variable or the instance-specific copy (also available on GPU).
     * \return       The C++ string representing the access to the global variable
     */
    virtual std::string global_variable_name(const SymbolType& symbol,
                                             bool use_instance = true) const = 0;


    /**
     * Determine variable name in the structure of mechanism properties
     *
     * \param name         Variable name that is being printed
     * \param use_instance Should the variable be accessed via instance or data array
     * \return             The C++ string representing the access to the variable in the neuron
     * thread structure
     */
    virtual std::string get_variable_name(const std::string& name,
                                          bool use_instance = true) const = 0;


    /****************************************************************************************/
    /*                      Main printing routines for code generation                      */
    /****************************************************************************************/


    /**
     * Print top file header printed in generated code
     */
    virtual void print_backend_info() = 0;


    /**
     * Print standard C/C++ includes
     */
    virtual void print_standard_includes() = 0;


    virtual void print_sdlists_init(bool print_initializers) = 0;


    /**
     * Print the structure that wraps all global variables used in the NMODL
     *
     * \param print_initializers Whether to include default values in the struct
     *                           definition (true: int foo{42}; false: int foo;)
     */
    virtual void print_mechanism_global_var_structure(bool print_initializers) = 0;


    /**
     * Print static assertions about the global variable struct.
     */
    virtual void print_global_var_struct_assertions() const;


    /**
     * Print declaration of macro NRN_PRCELLSTATE for debugging
     */
    void print_prcellstate_macros() const;


    /**
     * Print backend code for byte array that has mechanism information (to be registered
     * with NEURON/CoreNEURON)
     */
    void print_mechanism_info();


    /**
     * Print byte arrays that register scalar and vector variables for hoc interface
     *
     */
    virtual void print_global_variables_for_hoc() = 0;


    /**
     * Print the mechanism registration function
     *
     */
    virtual void print_mechanism_register() = 0;


    /**
     * Print common code for global functions like nrn_init, nrn_cur and nrn_state
     * \param type The target backend code block type
     */
    virtual void print_global_function_common_code(BlockType type,
                                                   const std::string& function_name = "") = 0;


    /**
     * Print nrn_constructor function definition
     *
     */
    virtual void print_nrn_constructor() = 0;


    /**
     * Print nrn_destructor function definition
     *
     */
    virtual void print_nrn_destructor() = 0;


    /**
     * Print nrn_alloc function definition
     *
     */
    virtual void print_nrn_alloc() = 0;


    /****************************************************************************************/
    /*                                 Print nrn_state routine                              */
    /****************************************************************************************/


    /**
     * Print nrn_state / state update function definition
     */
    virtual void print_nrn_state() = 0;


    /****************************************************************************************/
    /*                              Print nrn_cur related routines                          */
    /****************************************************************************************/


    /**
     * Print the \c nrn_current kernel
     *
     * \note nrn_cur_kernel will have two calls to nrn_current if no conductance keywords specified
     * \param node the AST node representing the NMODL breakpoint block
     */
    virtual void print_nrn_current(const ast::BreakpointBlock& node) = 0;


    /**
     * Print the \c nrn\_cur kernel with NMODL \c conductance keyword provisions
     *
     * If the NMODL \c conductance keyword is used in the \c breakpoint block, then
     * CodegenCoreneuronCppVisitor::print_nrn_cur_kernel will use this printer
     *
     * \param node the AST node representing the NMODL breakpoint block
     */
    virtual void print_nrn_cur_conductance_kernel(const ast::BreakpointBlock& node) = 0;


    /**
     * Print the \c nrn\_cur kernel without NMODL \c conductance keyword provisions
     *
     * If the NMODL \c conductance keyword is \b not used in the \c breakpoint block, then
     * CodegenCoreneuronCppVisitor::print_nrn_cur_kernel will use this printer
     */
    virtual void print_nrn_cur_non_conductance_kernel() = 0;


    /**
     * Print main body of nrn_cur function
     * \param node the AST node representing the NMODL breakpoint block
     */
    virtual void print_nrn_cur_kernel(const ast::BreakpointBlock& node) = 0;


    /**
     * Print fast membrane current calculation code
     */
    virtual void print_fast_imem_calculation() = 0;


    /**
     * Print nrn_cur / current update function definition
     */
    virtual void print_nrn_cur() = 0;


    /****************************************************************************************/
    /*                              Main code printing entry points                         */
    /****************************************************************************************/


    /**
     * Print all includes
     *
     */
    virtual void print_headers_include() = 0;


    /**
     * Print start of namespaces
     *
     */
    virtual void print_namespace_begin() = 0;


    /**
     * Print end of namespaces
     *
     */
    virtual void print_namespace_end() = 0;


    /**
     * Print all classes
     * \param print_initializers Whether to include default values.
     */
    virtual void print_data_structures(bool print_initializers) = 0;


    /**
     * Set v_unused (voltage) for NRN_PRCELLSTATE feature
     */
    virtual void print_v_unused() const = 0;


    /**
     * Set g_unused (conductance) for NRN_PRCELLSTATE feature
     */
    virtual void print_g_unused() const = 0;


    /**
     * Print all compute functions for every backend
     *
     */
    virtual void print_compute_functions() = 0;


    /**
     * Print entry point to code generation
     *
     */
    virtual void print_codegen_routines() = 0;


    /**
     * Print the nmodl constants used in backend code
     *
     * Currently we define three basic constants, which are assumed to be present in NMODL, directly
     * in the backend code:
     *
     * \code
     * static const double FARADAY = 96485.3;
     * static const double PI = 3.14159;
     * static const double R = 8.3145;
     * \endcode
     */
    void print_nmodl_constants();


    /****************************************************************************************/
    /*                                  Protected constructors                              */
    /****************************************************************************************/


    /// This constructor is private, only the derived classes' public constructors are public
    CodegenCppVisitor(std::string mod_filename,
                      const std::string& output_dir,
                      std::string float_type,
                      const bool optimize_ionvar_copies)
        : printer(std::make_unique<CodePrinter>(output_dir + "/" + mod_filename + ".cpp"))
        , mod_filename(std::move(mod_filename))
        , float_type(std::move(float_type))
        , optimize_ionvar_copies(optimize_ionvar_copies) {}


    /// This constructor is private, only the derived classes' public constructors are public
    CodegenCppVisitor(std::string mod_filename,
                      std::ostream& stream,
                      std::string float_type,
                      const bool optimize_ionvar_copies)
        : printer(std::make_unique<CodePrinter>(stream))
        , mod_filename(std::move(mod_filename))
        , float_type(std::move(float_type))
        , optimize_ionvar_copies(optimize_ionvar_copies) {}


    /****************************************************************************************/
    /*                            Overloaded visitor routines                               */
    /****************************************************************************************/

    void visit_binary_expression(const ast::BinaryExpression& node) override;
    void visit_binary_operator(const ast::BinaryOperator& node) override;
    void visit_boolean(const ast::Boolean& node) override;
    void visit_double(const ast::Double& node) override;
    void visit_else_if_statement(const ast::ElseIfStatement& node) override;
    void visit_else_statement(const ast::ElseStatement& node) override;
    void visit_float(const ast::Float& node) override;
    void visit_from_statement(const ast::FromStatement& node) override;
    void visit_function_call(const ast::FunctionCall& node) override;
    void visit_if_statement(const ast::IfStatement& node) override;
    void visit_indexed_name(const ast::IndexedName& node) override;
    void visit_integer(const ast::Integer& node) override;
    void visit_local_list_statement(const ast::LocalListStatement& node) override;
    void visit_name(const ast::Name& node) override;
    void visit_paren_expression(const ast::ParenExpression& node) override;
    void visit_prime_name(const ast::PrimeName& node) override;
    void visit_statement_block(const ast::StatementBlock& node) override;
    void visit_string(const ast::String& node) override;
    void visit_unary_operator(const ast::UnaryOperator& node) override;
    void visit_unit(const ast::Unit& node) override;
    void visit_var_name(const ast::VarName& node) override;
    void visit_verbatim(const ast::Verbatim& node) override;
    void visit_while_statement(const ast::WhileStatement& node) override;
    void visit_update_dt(const ast::UpdateDt& node) override;
    void visit_protect_statement(const ast::ProtectStatement& node) override;
    void visit_mutex_lock(const ast::MutexLock& node) override;
    void visit_mutex_unlock(const ast::MutexUnlock& node) override;
    void visit_solution_expression(const ast::SolutionExpression& node) override;

    std::string compute_method_name(BlockType type) const;

  public:
    /** Setup the target backend code generator
     *
     * Typically called from within \c visit\_program but may be called from
     * specialized targets to setup this Code generator as fallback.
     */
    void setup(const ast::Program& node);


    /**
     * Main and only member function to call after creating an instance of this class.
     * \param program the AST to translate to C++ code
     */
    void visit_program(const ast::Program& program) override;


    /****************************************************************************************/
    /*          Public printing routines for code generation for use in unit tests          */
    /****************************************************************************************/


    /**
     * Print the structure that wraps all range and int variables required for the NMODL
     *
     * \param print_initializers Whether or not default values for variables
     *                           be included in the struct declaration.
     */
    virtual void print_mechanism_range_var_structure(bool print_initializers) = 0;
};

/* Templated functions need to be defined in header file */
template <typename T>
void CodegenCppVisitor::print_vector_elements(const std::vector<T>& elements,
                                              const std::string& separator,
                                              const std::string& prefix) {
    for (auto iter = elements.begin(); iter != elements.end(); iter++) {
        printer->add_text(prefix);
        (*iter)->accept(*this);
        if (!separator.empty() && !nmodl::utils::is_last(iter, elements)) {
            printer->add_text(separator);
        }
    }
}


/** \} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl
