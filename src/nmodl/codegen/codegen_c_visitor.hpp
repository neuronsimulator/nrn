/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <algorithm>
#include <cmath>
#include <ctime>
#include <string>
#include <utility>

#include "fmt/format.h"

#include "codegen/codegen_info.hpp"
#include "codegen/codegen_naming.hpp"
#include "printer/code_printer.hpp"
#include "symtab/symbol_table.hpp"
#include "visitors/ast_visitor.hpp"


using namespace fmt::literals;


namespace nmodl {
namespace codegen {

/**
 * \enum BlockType
 * \brief Helper to represent various block types
 *
 */
enum class BlockType {
    /// initial block
    Initial,

    /// breakpoint block
    Equation,

    /// ode_* routines block (not used)
    Ode,

    /// derivative block
    State,

    /// watch block
    Watch
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
    std::shared_ptr<symtab::Symbol> symbol;

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
        : symbol(symbol)
        , is_vdata(is_vdata)
        , is_index(is_index)
        , is_integer(is_integer) {}
};


/**
 * \enum LayoutType
 * \brief Represents memory layout to use for code generation
 *
 */
enum class LayoutType {
    /// array of structure
    aos,

    /// structure of array
    soa
};


/**
 * \class ShadowUseStatement
 * \brief Represents ion write statement during code generation
 *
 * Ion update statement needs use of shadow vectors for certain backends
 * as atomics operations are not supported on cpu backend.
 *
 * \todo : if shadow_lhs is empty then we assume shadow statement not required
 */
struct ShadowUseStatement {
    std::string lhs;
    std::string op;
    std::string rhs;
};


/**
 * \class CodegenCVisitor
 * \brief Visitor for printing c code compatible with legacy api
 *
 * \todo :
 *      - handle define statement (i.e. macro statement printing)
 *      - return statement in the verbatim block of inlined function not handled
 *        for example, see netstim.mod where we removed return from verbatim block
 */
class CodegenCVisitor: public AstVisitor {
  protected:
    using SymbolType = std::shared_ptr<symtab::Symbol>;

    using ParamVector = std::vector<std::tuple<std::string, std::string, std::string, std::string>>;

    /// name of mod file (without .mod suffix)
    std::string mod_filename;

    /// flag to indicate if visitor should print the visited nodes
    bool codegen = false;

    /// variable name should be converted to instance name (but not for function arguments)
    bool enable_variable_name_lookup = true;

    /// symbol table for the program
    symtab::SymbolTable* program_symtab = nullptr;

    /// all float variables for the model
    std::vector<SymbolType> codegen_float_variables;

    /// all int variables for the model
    std::vector<IndexVariableInfo> codegen_int_variables;

    /// all global variables for the model
    /// @todo: this has become different than CodegenInfo
    std::vector<SymbolType> codegen_global_variables;

    /// all ion variables that could be possibly written
    std::vector<SymbolType> codegen_shadow_variables;

    /// true if currently net_receive block being printed
    bool printing_net_receive = false;

    /// true if currently printing top level verbatim blocks
    bool printing_top_verbatim_blocks = false;

    /// true if internal method call was encountered while processing verbatim block
    bool internal_method_call_encountered = false;

    /// index of watch statement being printed
    int current_watch_statement = 0;

    /// data type of floating point variables
    std::string float_type = codegen::naming::DEFAULT_FLOAT_TYPE;

    /// memory layout for code generation
    LayoutType layout;

    /// all ast information for code generation
    codegen::CodegenInfo info;

    /// code printer object for target (C, CUDA, ispc, ...)
    std::shared_ptr<CodePrinter> target_printer;

    /// code printer object for wrappers
    std::shared_ptr<CodePrinter> wrapper_printer;

    /// pointer to active code printer
    std::shared_ptr<CodePrinter> printer;

    /// list of shadow statements in the current block
    std::vector<ShadowUseStatement> shadow_statements;


    /// nmodl language version
    std::string nmodl_version() {
        return codegen::naming::NMODL_VERSION;
    }


    std::string add_escape_quote(const std::string& text) {
        return "\"" + text + "\"";
    }


    /// operator for rhs vector update (matrix update)
    std::string operator_for_rhs() {
        return info.electorde_current ? "+=" : "-=";
    }


    /// operator for diagonal vector update (matrix update)
    std::string operator_for_d() {
        return info.electorde_current ? "-=" : "+=";
    }


    /// data type for the local variables
    std::string local_var_type() {
        return codegen::naming::DEFAULT_LOCAL_VAR_TYPE;
    }


    /// default data type for floating point elements
    std::string default_float_data_type() {
        return codegen::naming::DEFAULT_FLOAT_TYPE;
    }


    /// data type for floating point elements specified on command line
    std::string float_data_type() {
        return float_type;
    }


    /// default data type for integer (offset) elements
    std::string default_int_data_type() {
        return codegen::naming::DEFAULT_INTEGER_TYPE;
    }


    /// function name for net send
    bool is_net_send(const std::string& name) {
        return name == codegen::naming::NET_SEND_METHOD;
    }

    /// function name for net move
    bool is_net_move(const std::string& name) {
        return name == codegen::naming::NET_MOVE_METHOD;
    }

    /// function name for net event
    bool is_net_event(const std::string& name) {
        return name == codegen::naming::NET_EVENT_METHOD;
    }


    /// name of structure that wraps range variables
    std::string instance_struct() {
        return "{}_Instance"_format(info.mod_suffix);
    }


    /// name of structure that wraps range variables
    std::string global_struct() {
        return "{}_Store"_format(info.mod_suffix);
    }


    /// name of function or procedure
    std::string method_name(const std::string& name) {
        auto suffix = info.mod_suffix;
        return name + "_" + suffix;
    }


    /// name for shadow variable
    std::string shadow_varname(const std::string& name) {
        return "shadow_" + name;
    }


    /// create temporary symbol
    SymbolType make_symbol(std::string name) {
        return std::make_shared<symtab::Symbol>(name, ModToken());
    }


    /// check if given variable is state variable
    bool state_variable(std::string name);


    /// check if net receive/send buffering kernels required
    bool net_receive_buffering_required();


    /// check if nrn_state function is required
    bool nrn_state_required();


    /// check if nrn_cur function is required
    bool nrn_cur_required();


    /// check if net_receive function is required
    bool net_receive_required();


    /// check if net_send_buffer is required
    bool net_send_buffer_required();


    /// check if setup_range_variable function is required
    bool range_variable_setup_required();


    /// check if net_receive node exist
    bool net_receive_exist();


    /// check if breakpoint node exist
    bool breakpoint_exist();


    /// if method is defined the mod file
    bool defined_method(const std::string& name);


    /// check if give statement should be skipped during code generation
    bool statement_to_skip(ast::Statement* node);


    /// check if semicolon required at the end of given statement
    bool need_semicolon(ast::Statement* node);


    /// number of threads to allocate
    int num_thread_objects() {
        return info.vectorize ? (info.thread_data_index + 1) : 0;
    }


    /// num of float variables in the model
    int float_variables_size();


    /// num of integer variables in the model
    int int_variables_size();


    /// for given float variable name, index position in the data array
    int position_of_float_var(const std::string& name);


    /// for given int variable name, index position in the data array
    int position_of_int_var(const std::string& name);


    /// when ion variable copies optimized then change name (e.g. ena to ion_ena)
    std::string update_if_ion_variable_name(const std::string& name);


    /// name of the code generation backend
    virtual std::string backend_name();


    /// convert given double value to string (for printing)
    virtual std::string double_to_string(double value);


    /// convert given float value to string (for printing)
    virtual std::string float_to_string(float value);


    /// get variable name for float variable
    std::string float_variable_name(SymbolType& symbol, bool use_instance);


    /// get variable name for int variable
    std::string int_variable_name(IndexVariableInfo& symbol,
                                  const std::string& name,
                                  bool use_instance);


    /// get variable name for global variable
    std::string global_variable_name(SymbolType& symbol);


    /// get ion shadow variable name
    std::string ion_shadow_variable_name(SymbolType& symbol);


    /// get variable name for given name. if use_instance is true then "Instance"
    /// structure is used while returning name (implemented by derived classes)
    virtual std::string get_variable_name(const std::string& name, bool use_instance = true);


    /// name of the current variable used in the breakpoint bock
    std::string breakpoint_current(std::string current);


    /// populate all index semantics needed for registration with coreneuron
    void update_index_semantics();


    /// return all float variables required during code generation
    std::vector<SymbolType> get_float_variables();


    /// return all int variables required during code generation
    std::vector<IndexVariableInfo> get_int_variables();


    /// return all ion write variables that require shadow vectors during code generation
    std::vector<SymbolType> get_shadow_variables();


    /// print vector elements (all types)
    template <typename T>
    void print_vector_elements(const std::vector<T>& elements,
                               const std::string& separator,
                               const std::string& prefix = "");

    std::string get_parameter_str(const ParamVector& params);

    /// check if function or procedure has argument with same name
    template <typename T>
    bool has_argument_with_name(const T& node, std::string name);


    /// any statement block in nmodl with option to (not) print braces
    void print_statement_block(ast::StatementBlock* node,
                               bool open_brace = true,
                               bool close_brace = true);


    /// check if structure for ion variables required
    bool ion_variable_struct_required();


    /// process verbatim block for possible variable renaming
    std::string process_verbatim_text(std::string text);


    /// process token in verbatim block for possible variable renaming
    std::string process_verbatim_token(const std::string& token);


    /// rename function/procedure arguments that conflict with default arguments
    void rename_function_arguments();


    //// statements for reading ion values
    std::vector<std::string> ion_read_statements(BlockType type);


    //// minimal statements for reading ion values
    std::vector<std::string> ion_read_statements_optimized(BlockType type);


    //// statements for writing ion values
    std::vector<ShadowUseStatement> ion_write_statements(BlockType type);


    /// return variable name and corresponding ion read variable name
    std::pair<std::string, std::string> read_ion_variable_name(std::string name);


    /// return variable name and corresponding ion write variable name
    std::pair<std::string, std::string> write_ion_variable_name(std::string name);


    /// function call / statement for nrn_wrote_conc
    std::string conc_write_statement(const std::string& ion_name,
                                     const std::string& concentration,
                                     int index);


    /// arguments for internally defined functions
    std::string internal_method_arguments();


    /// parameters for internally defined functions
    ParamVector internal_method_parameters();


    /// arguments for external functions
    std::string external_method_arguments();


    /// parameters for external functions
    std::string external_method_parameters(bool table = false);


    /// arguments for register_mech or point_register_mech function
    std::string register_mechanism_arguments();


    /// arguments for "_threadargs_" macro in neuron implementation
    std::string nrn_thread_arguments();


    /// arguments for "_threadargs_" macro in neuron implementation
    std::string nrn_thread_internal_arguments();


    /// replace commonly used verbatim variables
    std::string replace_if_verbatim_variable(std::string name);


    /// return name of main compute kernels
    virtual std::string compute_method_name(BlockType type);


    /// restrict keyword
    virtual std::string ptr_type_qualifier();


    virtual std::string param_type_qualifier();


    virtual std::string param_ptr_qualifier();


    /// const keyword
    virtual std::string k_const();


    /// start of coreneuron namespace
    void print_namespace_start();


    /// end of coreneuron namespace
    void print_namespace_stop();


    /// start of backend namespace
    virtual void print_backend_namespace_start();


    /// end of backend namespace
    virtual void print_backend_namespace_stop();


    /// nmodl constants
    void print_nmodl_constant();


    /// top header printed in generated code
    void print_backend_info();


    /// memory allocation routine
    virtual void print_memory_allocation_routine();


    /// standard c/c++ includes
    void print_standard_includes();


    /// includes from coreneuron
    void print_coreneuron_includes();


    /// backend specific includes
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
    void print_mechanism_info();


    /// structure that wraps all global variables in the mod file
    virtual void print_mechanism_global_var_structure();


    /// structure that wraps all range and int variables required for mod file
    void print_mechanism_range_var_structure();


    /// structure of ion variables used for local copies
    void print_ion_var_structure();


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


    /// net_move call
    void print_net_move_call(ast::FunctionCall* node);


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


    /// print check_function() for function/procedure using table
    void print_table_check_function(ast::Block* node);


    /// print replacement function for function/procedure using table
    void print_table_replacement_function(ast::Block* node);


    /// print check_table functions
    void print_check_table_thread_function();


    /// nmodl function definition
    void print_function(ast::FunctionBlock* node);


    /// nmodl procedure definition
    void print_procedure(ast::ProcedureBlock* node);


    /// print nmodl function or procedure (common code)
    void print_function_or_procedure(ast::Block* node, std::string& name);


    /// thread related memory allocation and deallocation callbacks
    void print_thread_memory_callbacks();


    /// top level (global scope) verbatim blocks
    void print_top_verbatim_blocks();


    /// prototype declarations of functions and procedures
    template <typename T>
    void print_function_declaration(const T& node, const std::string& name);


    /// initial block
    void print_initial_block(ast::InitialBlock* node);


    /// initial block in the net receive block
    void print_net_init();


    /// common code section for net receive related methods
    void print_net_receive_common_code(ast::Block* node, bool need_mech_inst = true);


    /// kernel for buffering net_send events
    void print_net_send_buffering();


    /// send event move block used in net receive as well as watch
    void print_send_event_move();


    virtual std::string net_receive_buffering_declaration();


    virtual void print_get_memb_list();

    virtual void print_net_receive_loop_begin();


    virtual void print_net_receive_loop_end();


    /// kernel for buffering net_receive events
    void print_net_receive_buffering(bool need_mech_inst = true);


    /// net_receive kernel function definition
    void print_net_receive_kernel();


    /// net_receive function definition
    void print_net_receive();


    /// derivative kernel when derivimplicit method is used
    void print_derivimplicit_kernel(ast::Block* block);


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
    virtual void print_global_function_common_code(BlockType type);


    /// nrn_init function definition
    void print_nrn_init(bool skip_init_check = true);


    /// nrn_state / state update function definition
    void print_nrn_state();


    /// nrn_cur / current update function definition
    void print_nrn_cur();


    /// mechanism registration function
    void print_mechanism_register();


    /// print watch activate function
    void print_watch_activate();


    /// print watch activate function
    void print_watch_check();


    /// all includes
    virtual void print_headers_include();


    /// start of namespaces
    void print_namespace_begin();


    /// end of namespaces
    void print_namespace_end();


    /// common getters
    void print_common_getters();


    /// all classes
    virtual void print_data_structures();


    /// all compute functions for every backend
    virtual void print_compute_functions();


    /// entry point to code generation
    virtual void print_codegen_routines();


    /// entry point to code generation for wrappers
    virtual void print_wrapper_routines();


    CodegenCVisitor(std::string mod_filename,
                    std::string output_dir,
                    LayoutType layout,
                    std::string float_type,
                    std::string extension,
                    std::string wrapper_ext)
        : target_printer(new CodePrinter(output_dir + "/" + mod_filename + extension))
        , wrapper_printer(new CodePrinter(output_dir + "/" + mod_filename + wrapper_ext))
        , printer(target_printer)
        , mod_filename(mod_filename)
        , layout(layout)
        , float_type(float_type) {}


  public:
    CodegenCVisitor(std::string mod_filename,
                    std::string output_dir,
                    LayoutType layout,
                    std::string float_type,
                    std::string extension = ".cpp")
        : target_printer(new CodePrinter(output_dir + "/" + mod_filename + extension))
        , printer(target_printer)
        , mod_filename(mod_filename)
        , layout(layout)
        , float_type(float_type) {}


    CodegenCVisitor(std::string mod_filename,
                    std::stringstream& stream,
                    LayoutType layout,
                    std::string float_type)
        : target_printer(new CodePrinter(stream))
        , printer(target_printer)
        , mod_filename(mod_filename)
        , layout(layout)
        , float_type(float_type) {}

    virtual void visit_binary_expression(ast::BinaryExpression* node) override;
    virtual void visit_binary_operator(ast::BinaryOperator* node) override;
    virtual void visit_boolean(ast::Boolean* node) override;
    virtual void visit_double(ast::Double* node) override;
    virtual void visit_else_if_statement(ast::ElseIfStatement* node) override;
    virtual void visit_else_statement(ast::ElseStatement* node) override;
    virtual void visit_float(ast::Float* node) override;
    virtual void visit_from_statement(ast::FromStatement* node) override;
    virtual void visit_function_call(ast::FunctionCall* node) override;
    virtual void visit_eigen_newton_solver_block(ast::EigenNewtonSolverBlock* node) override;
    virtual void visit_if_statement(ast::IfStatement* node) override;
    virtual void visit_indexed_name(ast::IndexedName* node) override;
    virtual void visit_integer(ast::Integer* node) override;
    virtual void visit_local_list_statement(ast::LocalListStatement* node) override;
    virtual void visit_name(ast::Name* node) override;
    virtual void visit_paren_expression(ast::ParenExpression* node) override;
    virtual void visit_prime_name(ast::PrimeName* node) override;
    virtual void visit_program(ast::Program* node) override;
    virtual void visit_statement_block(ast::StatementBlock* node) override;
    virtual void visit_string(ast::String* node) override;
    virtual void visit_solution_expression(ast::SolutionExpression* node) override;
    virtual void visit_unary_operator(ast::UnaryOperator* node) override;
    virtual void visit_unit(ast::Unit* node) override;
    virtual void visit_var_name(ast::VarName* node) override;
    virtual void visit_verbatim(ast::Verbatim* node) override;
    virtual void visit_watch_statement(ast::WatchStatement* node) override;
    virtual void visit_while_statement(ast::WhileStatement* node) override;
    virtual void visit_derivimplicit_callback(ast::DerivimplicitCallback* node) override;
};


/**
 * Print elements of vector with given separator and prefix string
 */
template <typename T>
void CodegenCVisitor::print_vector_elements(const std::vector<T>& elements,
                                            const std::string& separator,
                                            const std::string& prefix) {
    for (auto iter = elements.begin(); iter != elements.end(); iter++) {
        printer->add_text(prefix);
        (*iter)->accept(this);
        if (!separator.empty() && !is_last(iter, elements)) {
            printer->add_text(separator);
        }
    }
}


/**
 * Check if function or procedure node has parameter with given name
 *
 * @tparam T Node type (either procedure or function)
 * @param node AST node (either procedure or function)
 * @param name Name of parameter
 * @return True if argument with name exist
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
 * Print prototype declaration for function and procedures
 *
 * If there is an argument with name (say alpha) same as range variable (say alpha),
 * we want avoid it being printed as instance->alpha. And hence we disable variable
 * name lookup during prototype declaration. Note that the name of procedure can be
 * different in case of table statement.
 */
template <typename T>
void CodegenCVisitor::print_function_declaration(const T& node, const std::string& name) {
    enable_variable_name_lookup = false;
    auto type = default_float_data_type();

    /// internal and user provided arguments
    auto internal_params = internal_method_parameters();
    auto params = node->get_parameters();
    for (const auto& param: params) {
        internal_params.emplace_back("", type, "", param.get()->get_node_name());
    }

    /// procedures have "int" return type by default
    std::string return_type = "int";
    if (node->is_function_block()) {
        return_type = default_float_data_type();
    }

    print_device_method_annotation();
    printer->add_indent();
    printer->add_text("inline {} {}({})"_format(return_type, method_name(name),
                                                get_parameter_str(internal_params)));

    enable_variable_name_lookup = true;
}

}  // namespace codegen
}  // namespace nmodl
