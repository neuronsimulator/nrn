/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \dir
 * \brief Code generation backend implementations for CoreNEURON
 *
 * \file
 * \brief \copybrief nmodl::codegen::CodegenCVisitor
 */

#include <algorithm>
#include <cmath>
#include <ctime>
#include <numeric>
#include <ostream>
#include <string>
#include <utility>

#include "codegen/codegen_info.hpp"
#include "codegen/codegen_naming.hpp"
#include "printer/code_printer.hpp"
#include "symtab/symbol_table.hpp"
#include "utils/logger.hpp"
#include "visitors/ast_visitor.hpp"


namespace nmodl {
/// encapsulates code generation backend implementations
namespace codegen {

/**
 * @defgroup codegen Code Generation Implementation
 * @brief Implementations of code generation backends
 *
 * @defgroup codegen_details Codegen Helpers
 * @ingroup codegen
 * @brief Helper routines/types for code generation
 * @{
 */

/**
 * \enum BlockType
 * \brief Helper to represent various block types
 *
 * Note: do not assign integers to these enums
 *
 */
enum BlockType {
    /// initial block
    Initial,

    /// constructor block
    Constructor,

    /// destructor block
    Destructor,

    /// breakpoint block
    Equation,

    /// ode_* routines block (not used)
    Ode,

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

    /// if variable reside in vdata field of NrnThread
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

    IndexVariableInfo(std::shared_ptr<symtab::Symbol> symbol,
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

/** @} */  // end of codegen_details


using printer::CodePrinter;


/**
 * \defgroup codegen_backends Codegen Backends
 * \ingroup codegen
 * \brief Code generation backends for CoreNEURON
 * \{
 */

/**
 * \class CodegenCVisitor
 * \brief %Visitor for printing C code compatible with legacy api of CoreNEURON
 *
 * \todo
 *  - Handle define statement (i.e. macros)
 *  - If there is a return statement in the verbatim block
 *    of inlined function then it will be error. Need better
 *    error checking. For example, see netstim.mod where we
 *    have removed return from verbatim block.
 */
class CodegenCVisitor: public visitor::ConstAstVisitor {
  protected:
    using SymbolType = std::shared_ptr<symtab::Symbol>;

    /**
     * A vector of parameters represented by a 4-tuple of strings:
     *
     * - type qualifier (e.g. \c const)
     * - type (e.g. \c double)
     * - pointer qualifier (e.g. \c \_\_restrict\_\_)
     * - parameter name (e.g. \c data)
     */
    using ParamVector = std::vector<std::tuple<std::string, std::string, std::string, std::string>>;

    /**
     * Name of mod file (without .mod suffix)
     */
    std::string mod_filename;

    /**
     * Flag to indicate if visitor should print the visited nodes
     */
    bool codegen = false;

    /**
     * Flag to indicate if visitor should avoid ion variable copies
     */
    bool optimize_ionvar_copies = true;

    /**
     * Variable name should be converted to instance name (but not for function arguments)
     */
    bool enable_variable_name_lookup = true;

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
     * All ion variables that could be possibly written
     */
    std::vector<SymbolType> codegen_shadow_variables;

    /**
     * \c true if currently net_receive block being printed
     */
    bool printing_net_receive = false;

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

    /**
     * Data type of floating point variables
     */
    std::string float_type = codegen::naming::DEFAULT_FLOAT_TYPE;

    /**
     * All ast information for code generation
     */
    codegen::CodegenInfo info;

    /**
     * Code printer object for target (C, CUDA, ispc, ...)
     */
    std::shared_ptr<CodePrinter> target_printer;

    /**
     * Code printer object for wrappers
     */
    std::shared_ptr<CodePrinter> wrapper_printer;

    /**
     * Pointer to active code printer
     */
    std::shared_ptr<CodePrinter> printer;

    /**
     * List of shadow statements in the current block
     */
    std::vector<ShadowUseStatement> shadow_statements;


    /**
     * Return Nmodl language version
     * \return A version
     */
    std::string nmodl_version() const noexcept {
        return codegen::naming::NMODL_VERSION;
    }

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
     * Operator for rhs vector update (matrix update)
     */
    std::string operator_for_rhs() const noexcept {
        return info.electrode_current ? "+=" : "-=";
    }


    /**
     * Operator for diagonal vector update (matrix update)
     */
    std::string operator_for_d() const noexcept {
        return info.electrode_current ? "-=" : "+=";
    }


    /**
     * Data type for the local variables
     */
    std::string local_var_type() const noexcept {
        return codegen::naming::DEFAULT_LOCAL_VAR_TYPE;
    }


    /**
     * Default data type for floating point elements
     */
    std::string default_float_data_type() const noexcept {
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
    std::string default_int_data_type() const noexcept {
        return codegen::naming::DEFAULT_INTEGER_TYPE;
    }


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
     * Name of structure that wraps range variables
     */
    std::string instance_struct() const {
        return fmt::format("{}_Instance", info.mod_suffix);
    }


    /**
     * Name of structure that wraps range variables
     */
    std::string global_struct() const {
        return fmt::format("{}_Store", info.mod_suffix);
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
     * Constructs a shadow variable name
     * \param name The name of the variable
     * \return     The name of the variable prefixed with \c shadow_
     */
    std::string shadow_varname(const std::string& name) const {
        return "shadow_" + name;
    }


    /**
     * Creates a temporary symbol
     * \param name The name of the symbol
     * \return     A symbol based on the given name
     */
    SymbolType make_symbol(const std::string& name) const {
        return std::make_shared<symtab::Symbol>(name, ModToken());
    }


    /**
     * Checks if the given variable name belongs to a state variable
     * \param name The variable name
     * \return     \c true if the variable is a state variable
     */
    bool state_variable(const std::string& name) const;


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
     * Check if net_send_buffer is required
     */
    bool net_send_buffer_required() const noexcept;


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
     * Check if given statement should be skipped during code generation
     * \param node The AST Statement node to check
     * \return     \c true if this Statement is to be skipped
     */
    bool statement_to_skip(const ast::Statement& node) const;


    /**
     * Check if a semicolon is required at the end of given statement
     * \param node The AST Statement node to check
     * \return     \c true if this Statement requires a semicolon
     */
    bool need_semicolon(ast::Statement* node) const;


    /**
     * Determine the number of threads to allocate
     */
    int num_thread_objects() const noexcept {
        return info.vectorize ? (info.thread_data_index + 1) : 0;
    }


    /**
     * Number of float variables in the model
     */
    int float_variables_size() const;


    /**
     * Number of integer variables in the model
     */
    int int_variables_size() const;


    /**
     * Determine the position in the data array for a given float variable
     * \param name The name of a float variable
     * \return     The position index in the data array
     */
    int position_of_float_var(const std::string& name) const;


    /**
     * Determine the position in the data array for a given int variable
     * \param name The name of an int variable
     * \return     The position index in the data array
     */
    int position_of_int_var(const std::string& name) const;


    /**
     * Determine the updated name if the ion variable has been optimized
     * \param name The ion variable name
     * \return     The updated name of the variable has been optimized (e.g. \c ena --> \c ion_ena)
     */
    std::string update_if_ion_variable_name(const std::string& name) const;


    /**
     * Name of the code generation backend
     */
    virtual std::string backend_name() const;


    /**
     * Convert a given \c double value to its string representation
     * \param value The number to convert given as string as it is parsed by the modfile
     * \return      Its string representation
     */
    virtual std::string format_double_string(const std::string& value);


    /**
     * Convert a given \c float value to its string representation
     * \param value The number to convert given as string as it is parsed by the modfile
     * \return      Its string representation
     */
    virtual std::string format_float_string(const std::string& value);


    /**
     * Determine the name of a \c float variable given its symbol
     *
     * This function typically returns the accessor expression in backend code for the given symbol.
     * Since the model variables are stored in data arrays and accessed by offset, this function
     * will return the C string representing the array access at the correct offset
     *
     * \param symbol       The symbol of a variable for which we want to obtain its name
     * \param use_instance Should the variable be accessed via instance or data array
     * \return             The backend code string representing the access to the given variable
     * symbol
     */
    std::string float_variable_name(const SymbolType& symbol, bool use_instance) const;


    /**
     * Determine the name of an \c int variable given its symbol
     *
     * This function typically returns the accessor expression in backend code for the given symbol.
     * Since the model variables are stored in data arrays and accessed by offset, this function
     * will return the C string representing the array access at the correct offset
     *
     * \param symbol       The symbol of a variable for which we want to obtain its name
     * \param name         The name of the index variable
     * \param use_instance Should the variable be accessed via instance or data array
     * \return             The backend code string representing the access to the given variable
     * symbol
     */
    std::string int_variable_name(const IndexVariableInfo& symbol,
                                  const std::string& name,
                                  bool use_instance) const;


    /**
     * Determine the variable name for a global variable given its symbol
     * \param symbol The symbol of a variable for which we want to obtain its name
     * \return       The C string representing the access to the global variable
     */
    std::string global_variable_name(const SymbolType& symbol) const;


    /**
     * Determine the variable name for a shadow variable given its symbol
     * \param symbol The symbol of a variable for which we want to obtain its name
     * \return       The C string representing the access to the shadow variable
     */
    std::string ion_shadow_variable_name(const SymbolType& symbol) const;


    /**
     * Determine variable name in the structure of mechanism properties
     *
     * \param name         Variable name that is being printed
     * \param use_instance Should the variable be accessed via instance or data array
     * \return             The C string representing the access to the variable in the neuron thread
     * structure
     */
    std::string get_variable_name(const std::string& name, bool use_instance = true) const;


    /**
     * Determine the variable name for the "current" used in breakpoint block taking into account
     * intermediate code transformations.
     * \param current The variable name for the current used in the model
     * \return        The name for the current to be printed in C
     */
    std::string breakpoint_current(std::string current) const;


    /**
     * populate all index semantics needed for registration with coreneuron
     */
    void update_index_semantics();


    /**
     * Determine all \c float variables required during code generation
     * \return A \c vector of \c float variables
     */
    std::vector<SymbolType> get_float_variables();


    /**
     * Determine all \c int variables required during code generation
     * \return A \c vector of \c int variables
     */
    std::vector<IndexVariableInfo> get_int_variables();


    /**
     * Determine all ion write variables that require shadow vectors during code generation
     * \return A \c vector of ion variables
     */
    std::vector<SymbolType> get_shadow_variables();


    /**
     * Print the items in a vector as a list
     *
     * This function prints a given vector of elements as a list with given separator onto the
     * current printer. Elements are expected to be of type nmodl::ast::Ast and are printed by being
     * visited. Care is taken to omit the separator after the the last element.
     *
     * \tparam The element type in the vector, which must be of type nmodl::ast::Ast
     * \param  elements The vector of elements to be printed
     * \param  prefix A prefix string to printed before each element
     * \param  separator The seperator string to be printed between all elements
     */
    template <typename T>
    void print_vector_elements(const std::vector<T>& elements,
                               const std::string& separator,
                               const std::string& prefix = "");

    /**
     * Generate the string representing the procedure parameter declaration
     *
     * The procedure parameters are stored in a vector of 4-tuples each representing a parameter.
     *
     * \param params The parameters that should be concatenated into the function parameter
     * declaration \return The string representing the declaration of function parameters
     */
    std::string get_parameter_str(const ParamVector& params);


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
     * Check if a structure for ion variables is required
     * \return \c true if a structure fot ion variables must be generated
     */
    bool ion_variable_struct_required() const;


    /**
     * Process a verbatim block for possible variable renaming
     * \param text The verbatim code to be processed
     * \return     The code with all variables renamed as needed
     */
    std::string process_verbatim_text(std::string text);


    /**
     * Process a token in a verbatim block for possible variable renaming
     * \param token The verbatim token to be processed
     * \return      The code after variable renaming
     */
    std::string process_verbatim_token(const std::string& token);


    /**
     * Rename function/procedure arguments that conflict with default arguments
     */
    void rename_function_arguments();


    /**
     * For a given output block type, return statements for all read ion variables
     *
     * \param type The type of code block being generated
     * \return     A \c vector of strings representing the reading of ion variables
     */
    std::vector<std::string> ion_read_statements(BlockType type);


    /**
     * For a given output block type, return minimal statements for all read ion variables
     *
     * \param type The type of code block being generated
     * \return     A \c vector of strings representing the reading of ion variables
     */
    std::vector<std::string> ion_read_statements_optimized(BlockType type);


    /**
     * For a given output block type, return statements for writing back ion variables
     *
     * \param type The type of code block being generated
     * \return     A \c vector of strings representing the write-back of ion variables
     */
    std::vector<ShadowUseStatement> ion_write_statements(BlockType type);


    /**
     * Return ion variable name and corresponding ion read variable name
     * \param name The ion variable name
     * \return     The ion read variable name
     */
    std::pair<std::string, std::string> read_ion_variable_name(const std::string& name) const;


    /**
     * Return ion variable name and corresponding ion write variable name
     * \param name The ion variable name
     * \return     The ion write variable name
     */
    std::pair<std::string, std::string> write_ion_variable_name(const std::string& name) const;


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
     * Arguments for functions that are defined and used internally.
     * \return the method arguments
     */
    std::string internal_method_arguments();


    /**
     * Parameters for internally defined functions
     * \return the method parameters
     */
    ParamVector internal_method_parameters();


    /**
     * Arguments for external functions called from generated code
     * \return A string representing the arguments passed to an external function
     */
    std::string external_method_arguments() const;


    /**
     * Parameters for functions in generated code that are called back from external code
     *
     * Functions registered in NEURON during initialization for callback must adhere to a prescribed
     * calling convention. This method generates the string representing the function parameters for
     * these externally called functions.
     * \param table
     * \return      A string representing the parameters of the function
     */
    std::string external_method_parameters(bool table = false) const;


    /**
     * Arguments for register_mech or point_register_mech function
     */
    std::string register_mechanism_arguments() const;


    /**
     * Arguments for "_threadargs_" macro in neuron implementation
     */
    std::string nrn_thread_arguments();


    /**
     * Arguments for "_threadargs_" macro in neuron implementation
     */
    std::string nrn_thread_internal_arguments();


    /**
     * Replace commonly used verbatim variables
     * \param name A variable name to be checked and possibly updated
     * \return     The possibly replace variable name
     */
    std::string replace_if_verbatim_variable(std::string name);


    /**
     * Return the name of main compute kernels
     * \param type A block type
     */
    virtual std::string compute_method_name(BlockType type) const;


    /**
     * The used pointer qualifier.
     *
     * For C code generation this is \c \_\_restrict\_\_ to ensure that the compiler is aware of
     * the fact that we are working only with non-overlapping memory regions
     * \return \c \_\_restrict\_\_
     */
    virtual std::string ptr_type_qualifier();

    /**
     * The used global type qualifier
     *
     * For C code generation this is empty
     * \return ""
     *
     * For ispc
     * \return "uniform "
     */
    virtual std::string global_var_struct_type_qualifier();

    /**
     * Instantiate global var instance
     *
     * For C code generation this is empty
     * \return ""
     */
    virtual void print_global_var_struct_decl();

    /**
     * The used parameter type qualifier
     * \return an empty string
     */
    virtual std::string param_type_qualifier();


    /**
     * The used parameter pointer type qualifier
     * \return an empty string
     */
    virtual std::string param_ptr_qualifier();


    /**
     * Returns the \c const keyword
     * \return  \c const
     */
    virtual std::string k_const();


    /**
     * Prints the start of the \c coreneuron namespace
     */
    void print_namespace_start();


    /**
     * Prints the end of the \c coreneuron namespace
     */
    void print_namespace_stop();


    /**
     * Prints the start of namespace for the backend-specific code
     *
     * For the C backend no additional namespace is required
     */
    virtual void print_backend_namespace_start();


    /**
     * Prints the end of namespace for the backend-specific code
     *
     * For the C backend no additional namespace is required
     */
    virtual void print_backend_namespace_stop();


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
    virtual void print_nmodl_constants();


    /**
     * Print top file header printed in generated code
     */
    void print_backend_info();


    /**
     * Print memory allocation routine
     */
    virtual void print_memory_allocation_routine() const;


    /**
     * Print backend specific abort routine
     */
    virtual void print_abort_routine() const;


    /**
     * Print standard C/C++ includes
     */
    void print_standard_includes();


    /**
     * Print includes from coreneuron
     */
    void print_coreneuron_includes();


    /**
     * Print backend specific includes (none needed for C backend)
     */
    virtual void print_backend_includes();


    /**
     * Determine whether use of shadow updates at channel level required
     *
     *
     * \param type The backend block type
     * \return  \c true if shadow updates are needed
     */
    virtual bool block_require_shadow_update(BlockType type);


    /**
     * Determine whether this backend is performing channel execution with dependency
     * \return \c true if task dependency is enabled
     */
    virtual bool channel_task_dependency_enabled();


    /**
     * Check if \c shadow\_vector\_setup function is required
     */
    bool shadow_vector_setup_required();


    /**
     * Check if ion variable copies should be avoided
     */
    bool optimize_ion_variable_copies() const;


    /**
     * Check if reduction block in \c nrn\_cur required
     */
    virtual bool nrn_cur_reduction_loop_required();


    /**
     * Check if variable is qualified as constant
     * \param name The name of variable
     * \return \c true if it is constant
     */
    virtual bool is_constant_variable(const std::string& name) const;


    /**
     * Print declaration of macro NRN_PRCELLSTATE for debugging
     */
    void print_prcellstate_macros() const;

    /**
     * Print backend code for byte array that has mechanism information (to be registered
     * with coreneuron)
     */
    void print_mechanism_info();


    /**
     * Print the structure that wraps all global variables used in the NMODL
     */
    void print_mechanism_global_var_structure();


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
     * Returns floating point type for given range variable symbol
     * \param symbol A range variable symbol
     */
    std::string get_range_var_float_type(const SymbolType& symbol);


    /**
     * Print the function that initialize range variable with different data type
     */
    void print_setup_range_variable();


    /**
     * Print the code to copy instance variable to device
     */
    virtual void print_instance_variable_transfer_to_device() const;


    /**
     * Print the code to copy derivative advance flag to device
     */
    virtual void print_deriv_advance_flag_transfer_to_device() const;


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
     * Print byte arrays that register scalar and vector variables for hoc interface
     *
     */
    void print_global_variables_for_hoc();


    /**
     * Print the getter method for thread variables and ids
     *
     */
    void print_thread_getters();


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
     * Print the getter method for returning membrane list from NrnThread
     *
     */
    void print_memb_list_getter();


    /**
     * Print the getter method for returning mechtype
     *
     */
    void print_mech_type_getter();


    /**
     * Print the setup method that initializes all global variables
     *
     */
    void print_global_variable_setup();


    /**
     * Print the pragma annotation needed before a global variable that must be
     * created on the device. This always comes before a matching call to
     * print_global_variable_device_create_annotation_post.
     *
     * \note This is not used for the C backend
     */
    virtual void print_global_variable_device_create_annotation_pre();

    /**
     * Print the pragma annotation needed after a global variables that must be
     * created on the device. This always comes after a matching call to
     * print_global_variable_device_create_annotation_pre.
     *
     * \note This is not used for the C backend
     */
    virtual void print_global_variable_device_create_annotation_post();

    /**
     * Print the pragma annotation to update global variables from host to the device
     *
     * \note This is not used for the C backend
     */
    virtual void print_global_variable_device_update_annotation();


    /**
     * Print the setup method for allocation of shadow vectors
     *
     */
    void print_shadow_vector_setup();


    /**
     * Print the setup method for setting matrix shadow vectors
     *
     */
    virtual void print_rhs_d_shadow_variables();


    /**
     * Print the backend specific device method annotation
     *
     * \note This is not used for the C backend
     */
    virtual void print_device_method_annotation();


    /**
     * Print backend specific global method annotation
     *
     * \note This is not used for the C backend
     */
    virtual void print_global_method_annotation();


    /**
     * Print call to internal or external function
     * \param node The AST node representing a function call
     */
    void print_function_call(const ast::FunctionCall& node);


    /**
     * Print call to \c net\_send
     * \param node The AST node representing the function call
     */
    void print_net_send_call(const ast::FunctionCall& node);


    /**
     * Print call to net\_move
     * \param node The AST node representing the function call
     */
    void print_net_move_call(const ast::FunctionCall& node);


    /**
     * Print call to net\_event
     * \param node The AST node representing the function call
     */
    void print_net_event_call(const ast::FunctionCall& node);


    /**
     * Print channel iterations from which tasks are created
     *
     * \note This is not used for the C backend
     * \param type
     */
    virtual void print_channel_iteration_task_begin(BlockType type);


    /**
     * Print end of channel iteration for task
     *
     * \note This is not used for the C backend
     */
    virtual void print_channel_iteration_task_end();


    /**
     * Print block start for tiling on channel iteration
     */
    virtual void print_channel_iteration_tiling_block_begin(BlockType type);


    /**
     * Print block end for tiling on channel iteration
     */
    virtual void print_channel_iteration_tiling_block_end();


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
    virtual void print_channel_iteration_block_parallel_hint(BlockType type);


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
     * Print backend specific channel instance iteration block start
     * \param type The block type in which we currently are
     */
    virtual void print_channel_iteration_block_begin(BlockType type);


    /**
     * Print backend specific channel instance iteration block end
     */
    virtual void print_channel_iteration_block_end();


    /**
     * Print function and procedures prototype declaration
     */
    void print_function_prototypes();


    /**
     * Print check_table functions
     */
    void print_check_table_thread_function();


    /**
     * Print nmodl function or procedure (common code)
     * \param node the AST node representing the function or procedure in NMODL
     * \param name the name of the function or procedure
     */
    void print_function_or_procedure(const ast::Block& node, const std::string& name);


    /**
     * Common helper function to help printing function or procedure blocks
     * \param node the AST node representing the function or procedure in NMODL
     */
    void print_function_procedure_helper(const ast::Block& node);

    /**
     * Print thread related memory allocation and deallocation callbacks
     */
    void print_thread_memory_callbacks();


    /**
     * Print top level (global scope) verbatim blocks
     */
    void print_top_verbatim_blocks();


    /**
     * Print prototype declarations of functions or procedures
     * \tparam T   The AST node type of the node (must be of nmodl::ast::Ast or subclass)
     * \param node The AST node representing the function or procedure block
     * \param name A user defined name for the function
     */
    template <typename T>
    void print_function_declaration(const T& node, const std::string& name);


    /**
     * Print initial block statements
     *
     * Generate the target backend code corresponding to the NMODL initial block statements
     *
     * \param node The AST Node representing a NMODL initial block
     */
    void print_initial_block(const ast::InitialBlock* node);


    /**
     * Print initial block in the net receive block
     */
    void print_net_init();


    /**
     * Print the common code section for net receive related methods
     *
     * \param node The AST node representing the corresponding NMODL block
     * \param need_mech_inst \c true if a local \c inst variable needs to be defined in generated
     * code
     */
    void print_net_receive_common_code(const ast::Block& node, bool need_mech_inst = true);


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
     * Print \c net\_receive function definition
     */
    void print_net_receive();


    /**
     * Print derivative kernel when \c derivimplicit method is used
     *
     * \param block The corresponding AST node represening an NMODL \c derivimplicit block
     */
    void print_derivimplicit_kernel(ast::Block* block);


    /**
     * Print code block to transfer newtonspace structure to device
     */
    virtual void print_newtonspace_transfer_to_device() const;


    /**
     * Print pragma annotation for increase and capture of variable in automatic way
     */
    virtual void print_device_atomic_capture_annotation() const;

    /**
     * Print block / loop for statement requiring reduction
     *
     */
    virtual void print_shadow_reduction_block_begin();


    /**
     * Print end of block / loop for statement requiring reduction
     *
     */
    virtual void print_shadow_reduction_block_end();


    /**
     * Print atomic update pragma for reduction statements
     *
     */
    virtual void print_atomic_reduction_pragma();


    /**
     * Print all reduction statements
     *
     */
    void print_shadow_reduction_statements();


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


    /**
     * Print main body of nrn_cur function
     * \param node the AST node representing the NMODL breakpoint block
     */
    void print_nrn_cur_kernel(const ast::BreakpointBlock& node);


    /**
     * Print the \c nrn\_cur kernel with NMODL \c conductance keyword provisions
     *
     * If the NMODL \c conductance keyword is used in the \c breakpoint block, then
     * CodegenCVisitor::print_nrn_cur_kernel will use this printer
     *
     * \param node the AST node representing the NMODL breakpoint block
     */
    void print_nrn_cur_conductance_kernel(const ast::BreakpointBlock& node);


    /**
     * Print the \c nrn\_cur kernel without NMODL \c conductance keyword provisions
     *
     * If the NMODL \c conductance keyword is \b not used in the \c breakpoint block, then
     * CodegenCVisitor::print_nrn_cur_kernel will use this printer
     */
    void print_nrn_cur_non_conductance_kernel();


    /**
     * Print the \c nrn_current kernel
     *
     * \note nrn_cur_kernel will have two calls to nrn_current if no conductance keywords specified
     * \param node the AST node representing the NMODL breakpoint block
     */
    void print_nrn_current(const ast::BreakpointBlock& node);


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
     * Print nrn_constructor function definition
     *
     */
    void print_nrn_constructor();


    /**
     * Print nrn_destructor function definition
     *
     */
    void print_nrn_destructor();


    /**
     * Print nrn_alloc function definition
     *
     */
    void print_nrn_alloc();


    /**
     * Print common code for global functions like nrn_init, nrn_cur and nrn_state
     * \param type The target backend code block type
     */
    virtual void print_global_function_common_code(BlockType type,
                                                   const std::string& function_name = "");


    /**
     * Print the mechanism registration function
     *
     */
    void print_mechanism_register();


    /**
     * Print watch activate function
     *
     */
    void print_watch_activate();


    /**
     * Print all includes
     *
     */
    virtual void print_headers_include();


    /**
     * Print start of namespaces
     *
     */
    void print_namespace_begin();


    /**
     * Print end of namespaces
     *
     */
    void print_namespace_end();


    /**
     * Print common getters
     *
     */
    void print_common_getters();


    /**
     * Print all classes
     *
     */
    void print_data_structures();


    /**
     * Set v_unused (voltage) for NRN_PRCELLSTATE feature
     */
    void print_v_unused() const;


    /**
     * Set g_unused (conductance) for NRN_PRCELLSTATE feature
     */
    void print_g_unused() const;


    /**
     * Print all compute functions for every backend
     *
     */
    virtual void print_compute_functions();


    /**
     * Print entry point to code generation
     *
     */
    virtual void print_codegen_routines();


    /**
     * Print entry point to code generation for wrappers
     */
    virtual void print_wrapper_routines();


    /**
     * Get device variable pointer for corresponding host variable
     */
    virtual std::string get_variable_device_pointer(const std::string& variable,
                                                    const std::string& type) const;


    CodegenCVisitor(const std::string& mod_filename,
                    const std::string& output_dir,
                    const std::string& float_type,
                    const bool optimize_ionvar_copies,
                    const std::string& extension,
                    const std::string& wrapper_ext)
        : target_printer(new CodePrinter(output_dir + "/" + mod_filename + extension))
        , wrapper_printer(new CodePrinter(output_dir + "/" + mod_filename + wrapper_ext))
        , printer(target_printer)
        , mod_filename(mod_filename)
        , float_type(float_type)
        , optimize_ionvar_copies(optimize_ionvar_copies) {}

    CodegenCVisitor(const std::string& mod_filename,
                    std::ostream& stream,
                    const std::string& float_type,
                    const bool optimize_ionvar_copies,
                    const std::string& extension,
                    const std::string& wrapper_ext)
        : target_printer(new CodePrinter(stream))
        , wrapper_printer(new CodePrinter(stream))
        , printer(target_printer)
        , mod_filename(mod_filename)
        , float_type(float_type)
        , optimize_ionvar_copies(optimize_ionvar_copies) {}


  public:
    /**
     * \brief Constructs the C code generator visitor
     *
     * This constructor instantiates an NMODL C code generator and allows writing generated code
     * directly to a file in \c [output_dir]/[mod_filename].[extension].
     *
     * \note No code generation is performed at this stage. Since the code
     * generator classes are all based on \c AstVisitor the AST must be visited using e.g. \c
     * visit_program in order to generate the C code corresponding to the AST.
     *
     * \param mod_filename The name of the model for which code should be generated.
     *                     It is used for constructing an output filename.
     * \param output_dir   The directory where target C file should be generated.
     * \param float_type   The float type to use in the generated code. The string will be used
     *                     as-is in the target code. This defaults to \c double.
     * \param extension    The file extension to use. This defaults to \c .cpp .
     */
    CodegenCVisitor(const std::string& mod_filename,
                    const std::string& output_dir,
                    std::string float_type,
                    const bool optimize_ionvar_copies,
                    const std::string& extension = ".cpp")
        : target_printer(new CodePrinter(output_dir + "/" + mod_filename + extension))
        , printer(target_printer)
        , mod_filename(mod_filename)
        , float_type(std::move(float_type))
        , optimize_ionvar_copies(optimize_ionvar_copies) {}

    /**
     * \copybrief nmodl::codegen::CodegenCVisitor
     *
     * This constructor instantiates an NMODL C code generator and allows writing generated code
     * into an output stream.
     *
     * \note No code generation is performed at this stage. Since the code
     * generator classes are all based on \c AstVisitor the AST must be visited using e.g. \c
     * visit_program in order to generate the C code corresponding to the AST.
     *
     * \param mod_filename The name of the model for which code should be generated.
     *                     It is used for constructing an output filename.
     * \param stream       The output stream onto which to write the generated code
     * \param float_type   The float type to use in the generated code. The string will be used
     *                     as-is in the target code. This defaults to \c double.
     */
    CodegenCVisitor(const std::string& mod_filename,
                    std::ostream& stream,
                    const std::string& float_type,
                    const bool optimize_ionvar_copies)
        : target_printer(new CodePrinter(stream))
        , printer(target_printer)
        , mod_filename(mod_filename)
        , float_type(float_type)
        , optimize_ionvar_copies(optimize_ionvar_copies) {}


    /**
     * \copybrief nmodl::codegen::CodegenCVisitor
     *
     * This constructor instantiates an NMODL C code generator and allows writing generated code
     * using an nmodl::printer::CodePrinter defined elsewhere.
     *
     * \note No code generation is performed at this stage. Since the code
     * generator classes are all based on \c AstVisitor the AST must be visited using e.g. \c
     * visit_program in order to generate the C code corresponding to the AST.
     *
     * \param mod_filename   The name of the model for which code should be generated.
     *                       It is used for constructing an output filename.
     * \param float_type     The float type to use in the generated code. The string will be used
     *                       as-is in the target code. This defaults to \c double.
     * \param target_printer A printer defined outside this visitor to be used for the code
     *                       generation
     */
    CodegenCVisitor(std::string mod_filename,
                    std::string float_type,
                    const bool optimize_ionvar_copies,
                    std::shared_ptr<CodePrinter>& target_printer)
        : target_printer(target_printer)
        , printer(target_printer)
        , mod_filename(mod_filename)
        , float_type(float_type)
        , optimize_ionvar_copies(optimize_ionvar_copies) {}


    /**
     * Print the \c nrn\_init function definition
     * \param skip_init_check \c true if we want the generated code to execute the initialization
     *                        conditionally
     */
    void print_nrn_init(bool skip_init_check = true);


    /**
     * Print nrn_state / state update function definition
     */
    void print_nrn_state();


    /**
     * Print nrn_cur / current update function definition
     */
    void print_nrn_cur();

    /**
     * Print fast membrane current calculation code
     */
    virtual void print_fast_imem_calculation();


    /**
     * Print kernel for buffering net_receive events
     *
     * This kernel is only needed for accelerator backends where \c net\_receive needs to be
     * executed in two stages as the actual communication must be done in the host code. \param
     * need_mech_inst \c true if the generated code needs a local inst variable to be defined
     */
    void print_net_receive_buffering(bool need_mech_inst = true);


    /**
     * Print \c net\_receive kernel function definition
     */
    void print_net_receive_kernel();


    /**
     * Print watch activate function
     */
    void print_watch_check();


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
     * Print NMODL function in target backend code
     * \param node
     */
    void print_function(const ast::FunctionBlock& node);


    /**
     * Print NMODL procedure in target backend code
     * \param node
     */
    virtual void print_procedure(const ast::ProcedureBlock& node);

    /**
     * Print NMODL before / after block in target backend code
     * @param node AST node of type before/after type being printed
     * @param block_id Index of the before/after block
     */
    virtual void print_before_after_block(const ast::Block* node, size_t block_id);

    /** Setup the target backend code generator
     *
     * Typically called from within \c visit\_program but may be called from
     * specialized targets to setup this Code generator as fallback.
     */
    void setup(const ast::Program& node);


    /**
     * Set the global variables to be generated in target backend code
     * \param global_vars
     */
    void set_codegen_global_variables(std::vector<SymbolType>& global_vars);

    /**
     * Find unique variable name defined in nmodl::utils::SingletonRandomString by the
     * nmodl::visitor::SympySolverVisitor
     * @param original_name Original name of variable to change
     * @return std::string Unique name produced as [original_name]_[random_string]
     */
    std::string find_var_unique_name(const std::string& original_name) const;

    /**
     * Print the structure that wraps all range and int variables required for the NMODL
     */
    void print_mechanism_range_var_structure();

    /**
     * Print the function that initialize instance structure
     */
    void print_instance_variable_setup();

    void visit_binary_expression(const ast::BinaryExpression& node) override;
    void visit_binary_operator(const ast::BinaryOperator& node) override;
    void visit_boolean(const ast::Boolean& node) override;
    void visit_double(const ast::Double& node) override;
    void visit_else_if_statement(const ast::ElseIfStatement& node) override;
    void visit_else_statement(const ast::ElseStatement& node) override;
    void visit_float(const ast::Float& node) override;
    void visit_from_statement(const ast::FromStatement& node) override;
    void visit_function_call(const ast::FunctionCall& node) override;
    void visit_eigen_newton_solver_block(const ast::EigenNewtonSolverBlock& node) override;
    void visit_eigen_linear_solver_block(const ast::EigenLinearSolverBlock& node) override;
    virtual void print_eigen_linear_solver(const std::string& float_type, int N);
    void visit_if_statement(const ast::IfStatement& node) override;
    void visit_indexed_name(const ast::IndexedName& node) override;
    void visit_integer(const ast::Integer& node) override;
    void visit_local_list_statement(const ast::LocalListStatement& node) override;
    void visit_name(const ast::Name& node) override;
    void visit_paren_expression(const ast::ParenExpression& node) override;
    void visit_prime_name(const ast::PrimeName& node) override;
    void visit_program(const ast::Program& node) override;
    void visit_statement_block(const ast::StatementBlock& node) override;
    void visit_string(const ast::String& node) override;
    void visit_solution_expression(const ast::SolutionExpression& node) override;
    void visit_unary_operator(const ast::UnaryOperator& node) override;
    void visit_unit(const ast::Unit& node) override;
    void visit_var_name(const ast::VarName& node) override;
    void visit_verbatim(const ast::Verbatim& node) override;
    void visit_watch_statement(const ast::WatchStatement& node) override;
    void visit_while_statement(const ast::WhileStatement& node) override;
    void visit_derivimplicit_callback(const ast::DerivimplicitCallback& node) override;
    void visit_for_netcon(const ast::ForNetcon& node) override;
    void visit_update_dt(const ast::UpdateDt& node) override;
};


template <typename T>
void CodegenCVisitor::print_vector_elements(const std::vector<T>& elements,
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


/**
 * Check if function or procedure node has parameter with given name
 *
 * \tparam T Node type (either procedure or function)
 * \param node AST node (either procedure or function)
 * \param name Name of parameter
 * \return True if argument with name exist
 */
template <typename T>
bool has_parameter_of_name(const T& node, const std::string& name) {
    auto parameters = node->get_parameters();
    for (const auto& parameter: parameters) {
        if (parameter->get_node_name() == name) {
            return true;
        }
    }
    return false;
}


/**
 * \details If there is an argument with name (say alpha) same as range variable (say alpha),
 * we want to avoid it being printed as instance->alpha. And hence we disable variable
 * name lookup during prototype declaration. Note that the name of procedure can be
 * different in case of table statement.
 */
template <typename T>
void CodegenCVisitor::print_function_declaration(const T& node, const std::string& name) {
    enable_variable_name_lookup = false;
    auto type = default_float_data_type();

    // internal and user provided arguments
    auto internal_params = internal_method_parameters();
    const auto& params = node.get_parameters();
    for (const auto& param: params) {
        internal_params.emplace_back("", type, "", param.get()->get_node_name());
    }

    // procedures have "int" return type by default
    std::string return_type = "int";
    if (node.is_function_block()) {
        return_type = default_float_data_type();
    }

    print_device_method_annotation();
    printer->add_indent();
    printer->add_text(fmt::format(
        "inline {} {}({})", return_type, method_name(name), get_parameter_str(internal_params)));

    enable_variable_name_lookup = true;
}

/** \} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl
