#ifndef CODEGEN_HELPER_VISITOR_HPP
#define CODEGEN_HELPER_VISITOR_HPP

#include <string>

#include "symtab/symbol_table.hpp"
#include "visitors/ast_visitor.hpp"
#include "codegen/codegen_info.hpp"


/**
 * \class CodegenHelperVisitor
 * \brief Helper visitor to gather iformation for code generation
 *
 * Code generation pass needs various information from AST and symbol
 * table. Different code generation backends will need this information.
 * This helper pass visit ast and collect all information into single
 * class object of CodegenInfo.
 *
 * \todo:
 *  - determine vectorize as part of the pass
 *  - determine threadsafe as part of the pass
 *  - global variable order is not preserved, for example, below gives different order:
 *      NEURON block:    GLOBAL gq, gp
 *      PARAMETER block: gp = 11, gq[2]
 *  - POINTER rng and if it's also assigned rng[4] then it is printed as one value.
 *    Need to check what is correct value.
 */
class CodegenHelperVisitor : public AstVisitor {
    /// holds all codegen related information
    codegen::CodegenInfo info;

    /// if visiting net receive block
    bool under_net_receive_block = false;

    /// if visiting derivative block
    bool under_derivative_block = false;

    /// if visiting breakpoint block
    bool under_breakpoint_block = false;

    /// symbol table for the program
    symtab::SymbolTable* psymtab = nullptr;

    /// symbol table for the entire model
    symtab::ModelSymbolTable* model_symtab = nullptr;

    /// if we are visiting lhs of assignment statement
    bool visiting_lhs_assignment = false;

    using SymbolType = std::shared_ptr<symtab::Symbol>;

    /// lhs of assignment in derivative block
    std::shared_ptr<ast::Expression> assign_lhs = nullptr;

    /// name of the derivimplicit, euler and cnexp methods
    const std::string derivimplicit_method = "derivimplicit";
    const std::string euler_method = "euler";
    const std::string cnexp_method = "cnexp";
    const std::string net_send_method = "net_send";
    const std::string net_event_method = "net_event";
    const std::string artificial_cell = "ARTIFICIAL_CELL";
    const std::string point_process = "POINT_PROCESS";
    const std::string diam_variable = "diam";
    const std::string area_variable = "area";

    void find_solve_node();
    void find_ion_variables();
    void find_range_variables();
    void find_non_range_variables();
    void sort_with_mod2c_symbol_order(std::vector<SymbolType>& symbols);

  public:
    CodegenHelperVisitor() = default;
    codegen::CodegenInfo get_code_info(ast::Program* node);

    virtual void visit_elctrode_current(ast::ElctrodeCurrent* node) override;
    virtual void visit_suffix(ast::Suffix* node) override;
    virtual void visit_function_call(ast::FunctionCall* node) override;
    virtual void visit_binary_expression(ast::BinaryExpression* node) override;
    virtual void visit_conductance_hint(ast::ConductanceHint* node) override;
    virtual void visit_procedure_block(ast::ProcedureBlock* node) override;
    virtual void visit_function_block(ast::FunctionBlock* node) override;
    virtual void visit_solve_block(ast::SolveBlock* node) override;
    virtual void visit_statement_block(ast::StatementBlock* node) override;
    virtual void visit_initial_block(ast::InitialBlock* node) override;
    virtual void visit_breakpoint_block(ast::BreakpointBlock* node) override;
    virtual void visit_derivative_block(ast::DerivativeBlock* node) override;
    virtual void visit_net_receive_block(ast::NetReceiveBlock* node) override;
    virtual void visit_bbcore_ptr(ast::BbcorePtr* node) override;
    virtual void visit_watch(ast::Watch* node) override;
    virtual void visit_watch_statement(ast::WatchStatement* node) override;
    virtual void visit_for_netcon(ast::ForNetcon* node) override;
    virtual void visit_program(ast::Program* node) override;
};

#endif