#ifndef NMODL_CODEGEN_BASE_VISITOR_HPP
#define NMODL_CODEGEN_BASE_VISITOR_HPP


#include <string>
#include <algorithm>

#include "fmt/format.h"
#include "codegen/codegen_info.hpp"
#include "printer/code_printer.hpp"
#include "symtab/symbol_table.hpp"
#include "visitors/ast_visitor.hpp"


using namespace fmt::literals;


/**
 * \enum BlockType
 * \brief Helper to represent various block types (similar to NEURON)
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
    State
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

    IndexVariableInfo(std::shared_ptr<symtab::Symbol> symbol,
                      bool is_vdata = false,
                      bool is_index = false,
                      bool is_integer = false)
        : symbol(symbol), is_vdata(is_vdata), is_index(is_index), is_integer(is_integer) {
    }
};


/**
 * \enum LayoutType
 * \brief Represent memory layout to use for code generation
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
 * \brief Represent ion write statement during code generation
 *
 * Ion update statement need use of shadow vectors for certain backends
 * as atomics can be done in vector loops on cpu.
 *
 * \todo : if shadow_lhs is empty then we assume shadow statement not required.
 */
struct ShadowUseStatement {
    std::string lhs;
    std::string op;
    std::string rhs;
};


/**
 * \class CodegenBaseVisitor
 * \brief Visitor for printing c code compatible with legacy api
 *
 * \todo :
 *      - handle define i.e. macro statement printing
 *      - return statement in the verbatim block of inlined function not handled (e.g. netstim.mod)
 */
class CodegenBaseVisitor : public AstVisitor {
  protected:
    using SymbolType = std::shared_ptr<symtab::Symbol>;

    /// memory layout for code generation
    LayoutType layout;

    /// name of mod file (without .mod suffix)
    std::string mod_file_suffix;

    /// flag to indicate is visitor should print the visited nodes
    bool codegen = false;

    /// variable name should be converted to instance name (but not for function arguments)
    bool enable_variable_name_lookup = true;

    /// symbol table for the program
    symtab::SymbolTable* program_symtab = nullptr;

    /// all float variables for the model
    std::vector<SymbolType> float_variables;

    /// all int variables for the model
    std::vector<IndexVariableInfo> int_variables;

    /// all global variables for the model
    std::vector<SymbolType> global_variables;

    /// all ion variables that could be possibly written
    std::vector<SymbolType> shadow_variables;

    /// all ast information for code generation
    codegen::CodegenInfo info;

    /// code printer object
    std::unique_ptr<CodePrinter> printer;

    /// list of shadow statements in the current block
    std::vector<ShadowUseStatement> shadow_statements;

    /// node area variable
    const std::string node_area = "node_area";


    /// nmodl language version
    std::string nmodl_version() {
        return "6.2.0";
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
        return "double";
    }


    /// data type for floating point elements
    std::string float_data_type() {
        return "double";
    }


    /// data type for ineteger (offset) elemenets
    std::string int_data_type() {
        return "int";
    }


    /// function name for net send
    bool is_net_send(const std::string& name) {
        return name == "net_send";
    }


    /// function name for net event
    bool is_net_event(const std::string& name) {
        return name == "net_event";
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


    /// check if net_receive node exist
    bool net_receive_exist();


    /// check if breakpoint node exist
    bool breakpoint_exist();


    /// if method is defined the mod file
    bool defined_method(std::string name);


    /// check if give statement should be skipped during code generation
    bool skip_statement(ast::Statement* node);


    /// check if semicolon required at the end of given statement
    bool need_semicolon(ast::Statement* node);


    /// number of threads to allocate
    int num_thread_objects() {
        return info.vectorize ? (info.thread_data_index + 1) : 0;
    }


    /// num of float variables in the model
    int num_float_variable();


    /// num of integer variables in the model
    int num_int_variable();


    /// for given float variable name, index position in the data array
    int position_of_float_var(const std::string& name);


    /// for given int variable name, index position in the data array
    int position_of_int_var(const std::string& name);


    /// when ion variable copies optimized then change name (e.g. ena to ion_ena)
    std::string update_if_ion_variable_name(std::string name);


    /// name of the code generation backend
    std::string backend_name();


    /// convert given double value to string (for printing)
    std::string double_to_string(double value);


    /// get variable name for float variable
    std::string float_variable_name(SymbolType& symbol, bool use_instance);


    /// get variable name for int variable
    std::string int_variable_name(IndexVariableInfo& symbol, std::string name, bool use_instance);


    /// get variable name for global variable
    std::string global_variable_name(SymbolType& symbol);


    /// get ion shadow variable name
    std::string ion_shadow_variable_name(SymbolType& symbol);


    /// get variable name for given name. if use_instance is true then "Instance"
    /// structure is used while returning name (implemented by derived classes)
    virtual std::string get_variable_name(std::string name, bool use_instance = true) = 0;


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


    /// vector elements from all types
    template <typename T>
    void print_vector_elements(const std::vector<T>& elements,
                               std::string separator,
                               std::string prefix = "");


    /// any statement block in nmodl with option to (not) print braces
    void print_statement_block(ast::StatementBlock* node,
                               bool open_brace = true,
                               bool close_brace = true);


    /// common init for constructors
    void init(bool aos, std::string filename) {
        layout = aos ? LayoutType::aos : LayoutType::soa;
        mod_file_suffix = filename;
    }

  public:
    CodegenBaseVisitor(std::string mod_file, bool aos, std::string extension = ".cpp")
        : printer(new CodePrinter(mod_file + extension)) {
        init(aos, mod_file);
    }

    CodegenBaseVisitor(std::string mod_file, std::stringstream& stream, bool aos)
        : printer(new CodePrinter(stream)) {
        init(aos, mod_file);
    }

    virtual void visit_unit(ast::Unit* node) override;
    virtual void visit_string(ast::String* node) override;
    virtual void visit_integer(ast::Integer* node) override;
    virtual void visit_float(ast::Float* node) override;
    virtual void visit_double(ast::Double* node) override;
    virtual void visit_boolean(ast::Boolean* node) override;
    virtual void visit_name(ast::Name* node) override;
    virtual void visit_prime_name(ast::PrimeName* node) override;
    virtual void visit_var_name(ast::VarName* node) override;
    virtual void visit_indexed_name(ast::IndexedName* node) override;
    virtual void visit_local_list_statement(ast::LocalListStatement* node) override;
    virtual void visit_if_statement(ast::IfStatement* node) override;
    virtual void visit_else_if_statement(ast::ElseIfStatement* node) override;
    virtual void visit_else_statement(ast::ElseStatement* node) override;
    virtual void visit_from_statement(ast::FromStatement* node) override;
    virtual void visit_paren_expression(ast::ParenExpression* node) override;
    virtual void visit_binary_expression(ast::BinaryExpression* node) override;
    virtual void visit_binary_operator(ast::BinaryOperator* node) override;
    virtual void visit_unary_operator(ast::UnaryOperator* node) override;
    virtual void visit_statement_block(ast::StatementBlock* node) override;
    virtual void visit_program(ast::Program* node) override;
};


/**
 * Print elements of vector with given separator and prefix string
 */
template <typename T>
void CodegenBaseVisitor::print_vector_elements(const std::vector<T>& elements,
                                               std::string separator,
                                               std::string prefix) {
    for (auto iter = elements.begin(); iter != elements.end(); iter++) {
        printer->add_text(prefix);
        (*iter)->accept(this);
        if (!separator.empty() && !is_last(iter, elements)) {
            printer->add_text(separator);
        }
    }
}


#endif
