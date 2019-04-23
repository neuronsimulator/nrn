/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::codegen::CodegenHelperVisitor
 */

#include <string>

#include "codegen/codegen_info.hpp"
#include "symtab/symbol_table.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace codegen {

/**
 * @addtogroup codegen_details
 * @{
 */

/**
 * \class CodegenHelperVisitor
 * \brief Helper visitor to gather AST information to help code generation
 *
 * Code generation pass needs various information from AST and symbol
 * table. Different code generation backends will need this information.
 * This helper pass visit ast and collect all information into single
 * class object of CodegenInfo.
 *
 * \todo
 *  - determine `vectorize` as part of the pass
 *  - determine `threadsafe` as part of the pass
 *  - global variable order is not preserved, for example, below gives different order:
 *      NEURON block:    GLOBAL gq, gp
 *      PARAMETER block: gp = 11, gq[2]
 *  - POINTER rng and if it's also assigned rng[4] then it is printed as one value.
 *    Need to check what is correct value.
 */
class CodegenHelperVisitor: public visitor::AstVisitor {
    using SymbolType = std::shared_ptr<symtab::Symbol>;

    /// holds all codegen related information
    codegen::CodegenInfo info;

    /// if visiting net receive block
    bool under_net_receive_block = false;

    /// if visiting derivative block
    bool under_derivative_block = false;

    /// if visiting breakpoint block
    bool under_breakpoint_block = false;

    /// table statement found
    bool table_statement_used = false;

    /// symbol table for the program
    symtab::SymbolTable* psymtab = nullptr;

    /// lhs of assignment in derivative block
    std::shared_ptr<ast::Expression> assign_lhs;

    void find_ion_variables();
    void find_table_variables();
    void find_range_variables();
    void find_non_range_variables();
    void sort_with_mod2c_symbol_order(std::vector<SymbolType>& symbols);

  public:
    CodegenHelperVisitor() = default;

    /// run visitor and return information for code generation
    codegen::CodegenInfo analyze(ast::Program* node);

    void visit_elctrode_current(ast::ElctrodeCurrent* node) override;
    void visit_suffix(ast::Suffix* node) override;
    void visit_function_call(ast::FunctionCall* node) override;
    void visit_binary_expression(ast::BinaryExpression* node) override;
    void visit_conductance_hint(ast::ConductanceHint* node) override;
    void visit_procedure_block(ast::ProcedureBlock* node) override;
    void visit_function_block(ast::FunctionBlock* node) override;
    void visit_eigen_newton_solver_block(ast::EigenNewtonSolverBlock* node) override;
    void visit_eigen_linear_solver_block(ast::EigenLinearSolverBlock* node) override;
    void visit_statement_block(ast::StatementBlock* node) override;
    void visit_initial_block(ast::InitialBlock* node) override;
    void visit_breakpoint_block(ast::BreakpointBlock* node) override;
    void visit_derivative_block(ast::DerivativeBlock* node) override;
    void visit_derivimplicit_callback(ast::DerivimplicitCallback* node) override;
    void visit_net_receive_block(ast::NetReceiveBlock* node) override;
    void visit_bbcore_ptr(ast::BbcorePtr* node) override;
    void visit_watch(ast::Watch* node) override;
    void visit_watch_statement(ast::WatchStatement* node) override;
    void visit_for_netcon(ast::ForNetcon* node) override;
    void visit_table_statement(ast::TableStatement* node) override;
    void visit_program(ast::Program* node) override;
    void visit_nrn_state_block(ast::NrnStateBlock* node) override;
    void visit_linear_block(ast::LinearBlock* node) override;
    void visit_non_linear_block(ast::NonLinearBlock* node) override;
    void visit_discrete_block(ast::DiscreteBlock* node) override;
    void visit_partial_block(ast::PartialBlock* node) override;
};

/** @} */  // end of codegen_details

}  // namespace codegen
}  // namespace nmodl
