/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::PerfVisitor
 */

#include <set>
#include <stack>

#include "printer/json_printer.hpp"
#include "symtab/symbol_table.hpp"
#include "utils/perf_stat.hpp"
#include "visitors/ast_visitor.hpp"


namespace nmodl {
namespace visitor {

/**
 * @addtogroup visitor_classes
 * @{
 */

/**
 * \class PerfVisitor
 * \brief %Visitor for measuring performance related information
 *
 * This visitor used to visit the ast and associated symbol tables
 * to measure the performance of every block in nmodl file. For
 * every symbol in associated symbol table, read/write count
 * is updated which will be used during code generation (to select
 * memory types). Certain statements like useion, valence etc.
 * are not executed in the translated C code and hence need to
 * be skipped (i.e. without visiting children). Note that this
 * pass must be run after symbol table generation pass.
 *
 * \todo
 *     - To measure the performance of statements like if, elseif
 *       and else, we have to find maximum performance from if,elseif,else
 *       and then use it to calculate total performance. In the current
 *       implementation we are doing sum of all blocks. We need to override
 *       IfStatement (which has all sub-blocks) and get maximum performance
 *       of all statements recursively.
 *
 *     - In order to avoid empty implementations and checking
 *       start_measurement, there should be "empty" ast visitor from
 *       which PerfVisitor should be inherited.
 */
class PerfVisitor: public AstVisitor {
  private:
    /// symbol table of current block being visited
    symtab::SymbolTable* current_symtab = nullptr;

    /// performance stats of all blocks being visited
    /// in recursive chain
    std::stack<utils::PerfStat> blocks_perf;

    /// total performance of mod file
    utils::PerfStat total_perf;

    /// performance of current block
    utils::PerfStat current_block_perf;

    /// performance of current all childrens
    std::stack<utils::PerfStat> children_blocks_perf;

    /// whether to measure performance for current block
    bool start_measurement = false;

    /// true while visiting lhs of binary expression
    /// (to count write operations)
    bool visiting_lhs_expression = false;

    /// whether function call is being visited
    bool under_function_call = false;

    /// whether solve block is being visited
    bool under_solve_block = false;

    /// whether net receive block is being visited
    bool under_net_receive_block = false;

    /// to print to json file
    std::unique_ptr<printer::JSONPrinter> printer;

    /// if not json, all goes to string
    std::stringstream stream;

    /// count of per channel instance variables
    int num_instance_variables = 0;

    /// subset of instance variables which are constant
    int num_constant_instance_variables = 0;

    /// subset of instance variables which are localized
    int num_localized_instance_variables = 0;

    /// count of global variables
    int num_global_variables = 0;

    /// subset of global variables which are constant
    int num_constant_global_variables = 0;

    /// subset of global variables which are localized
    int num_localized_global_variables = 0;

    /// count of state variables
    int num_state_variables = 0;

    /// count of pointer / bbcorepointer variables
    int num_pointer_variables = 0;

    /// keys used in map to track var usage
    std::string const_memr_key = "cm_r_u";
    std::string const_memw_key = "cm_w_u";
    std::string global_memr_key = "gm_r_u";
    std::string global_memw_key = "gm_w_u";

    /// map of variables to count unique read-writes
    std::map<std::string, std::set<std::string>> var_usage = {{const_memr_key, {}},
                                                              {const_memw_key, {}},
                                                              {global_memr_key, {}},
                                                              {global_memw_key, {}}};

    void update_memory_ops(const std::string& name);

    bool symbol_to_skip(const std::shared_ptr<symtab::Symbol>& symbol);

    bool is_local_variable(const std::shared_ptr<symtab::Symbol>& symbol);

    bool is_constant_variable(const std::shared_ptr<symtab::Symbol>& symbol);

    void count_variables();

    void measure_performance(ast::Ast* node);

    void print_memory_usage();

    void add_perf_to_printer(utils::PerfStat& perf);

  public:
    PerfVisitor() = default;

    explicit PerfVisitor(const std::string& filename);

    void compact_json(bool flag) {
        printer->compact_json(flag);
    }

    utils::PerfStat get_total_perfstat() {
        return total_perf;
    }

    int get_instance_variable_count() {
        return num_instance_variables;
    }

    int get_const_instance_variable_count() {
        return num_constant_instance_variables;
    }

    int get_const_global_variable_count() {
        return num_constant_global_variables;
    }

    int get_global_variable_count() {
        return num_global_variables;
    }

    int get_state_variable_count() {
        return num_state_variables;
    }

    void visit_binary_expression(ast::BinaryExpression* node) override;

    void visit_function_call(ast::FunctionCall* node) override;

    void visit_name(ast::Name* node) override;

    void visit_prime_name(ast::PrimeName* node) override;

    void visit_solve_block(ast::SolveBlock* node) override;

    void visit_statement_block(ast::StatementBlock* node) override;

    void visit_unary_expression(ast::UnaryExpression* node) override;

    void visit_if_statement(ast::IfStatement* node) override;

    void visit_else_if_statement(ast::ElseIfStatement* node) override;

    void visit_program(ast::Program* node) override;

    void visit_plot_block(ast::PlotBlock* node) override {
        measure_performance(node);
    }

    /// skip initial block under net_receive block
    void visit_initial_block(ast::InitialBlock* node) override {
        if (!under_net_receive_block) {
            measure_performance(node);
        }
    }

    void visit_constructor_block(ast::ConstructorBlock* node) override {
        measure_performance(node);
    }

    void visit_destructor_block(ast::DestructorBlock* node) override {
        measure_performance(node);
    }

    void visit_derivative_block(ast::DerivativeBlock* node) override {
        measure_performance(node);
    }

    void visit_linear_block(ast::LinearBlock* node) override {
        measure_performance(node);
    }

    void visit_non_linear_block(ast::NonLinearBlock* node) override {
        measure_performance(node);
    }

    void visit_discrete_block(ast::DiscreteBlock* node) override {
        measure_performance(node);
    }

    void visit_partial_block(ast::PartialBlock* node) override {
        measure_performance(node);
    }

    void visit_function_table_block(ast::FunctionTableBlock* node) override {
        measure_performance(node);
    }

    void visit_function_block(ast::FunctionBlock* node) override {
        measure_performance(node);
    }

    void visit_procedure_block(ast::ProcedureBlock* node) override {
        measure_performance(node);
    }

    void visit_net_receive_block(ast::NetReceiveBlock* node) override {
        under_net_receive_block = true;
        measure_performance(node);
        under_net_receive_block = false;
    }

    void visit_breakpoint_block(ast::BreakpointBlock* node) override {
        measure_performance(node);
    }

    void visit_terminal_block(ast::TerminalBlock* node) override {
        measure_performance(node);
    }

    void visit_before_block(ast::BeforeBlock* node) override {
        measure_performance(node);
    }

    void visit_after_block(ast::AfterBlock* node) override {
        measure_performance(node);
    }

    void visit_ba_block(ast::BABlock* node) override {
        measure_performance(node);
    }

    void visit_for_netcon(ast::ForNetcon* node) override {
        measure_performance(node);
    }

    void visit_kinetic_block(ast::KineticBlock* node) override {
        measure_performance(node);
    }

    void visit_match_block(ast::MatchBlock* node) override {
        measure_performance(node);
    }

    /// certain constructs needs to be excluded from usage counting
    /// and hence need to provide empty implementations

    void visit_conductance_hint(ast::ConductanceHint* /*node*/) override {}

    void visit_local_list_statement(ast::LocalListStatement* /*node*/) override {}

    void visit_suffix(ast::Suffix* /*node*/) override {}

    void visit_useion(ast::Useion* /*node*/) override {}

    void visit_valence(ast::Valence* /*node*/) override {}

    void print(std::stringstream& ss) {
        ss << stream.str();
    }
};

/** @} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
