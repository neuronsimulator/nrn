#ifndef _NMODL_PERF_VISITOR_HPP_
#define _NMODL_PERF_VISITOR_HPP_

#include <stack>

#include "printer/json_printer.hpp"
#include "symtab/symbol_table.hpp"
#include "utils/perf_stat.hpp"
#include "visitors/ast_visitor.hpp"

/**
 * \class PerfVisitor
 * \brief Visitor for measuring performance related information
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
 * \todo : To measure the performance of statements like if, elseif
 * and else, we have to find maximum performance from if,elseif,else
 * and then use it to calculate total performance. In the current
 * implementation we are doing sum of all blocks. We need to override
 * IfStatement (which has all sub-blocks) and get maximum performance
 * of all statements recursively.
 *
 * \todo : In order to avoid empty implementations and checking
 * start_measurement, there should be "empty" ast visitor from
 * which PerfVisitor should be inherited.
 */

class PerfVisitor : public AstVisitor {
  private:
    /// symbol table of current block being visited
    symtab::SymbolTable* current_symtab = nullptr;

    /// performance stats of all blocks being visited
    /// in recursive chain
    std::stack<PerfStat> blocks_perf;

    /// total performance of mod file
    PerfStat total_perf;

    /// performance of current block
    PerfStat current_block_perf;

    /// performance of current all childrens
    std::stack<PerfStat> children_blocks_perf;

    /// whether to measure performance for current block
    bool start_measurement = false;

    /// true while visiting lhs of binary expression
    /// (to count write operations)
    bool visiting_lhs_expression = false;

    /// whether function call is being visited
    bool under_function_call = false;

    /// whether solve block is being visited
    bool under_solve_block = false;

    /// to print to json file
    std::unique_ptr<JSONPrinter> printer;

    /// if not json, all goes to string
    std::stringstream stream;

    /// count of per channel instance variables
    int num_instance_variables = 0;

    /// count of global variables
    int num_global_variables = 0;

    /// count of state variables
    int num_state_variables = 0;

    void update_memory_ops(const std::string& name);

    bool symbol_to_skip(const std::shared_ptr<symtab::Symbol>& symbol);

    bool is_local_variable(const std::shared_ptr<symtab::Symbol>& symbol);

    bool is_constant_variable(const std::shared_ptr<symtab::Symbol>& symbol);

    void count_variables();

    void measure_performance(AST* node);

    void print_memory_usage();

    void add_perf_to_printer(PerfStat& perf);

  public:
    PerfVisitor() = default;

    explicit PerfVisitor(const std::string& filename);

    void compact_json(bool flag) {
        printer->compact_json(flag);
    }

    PerfStat get_total_perfstat() {
        return total_perf;
    }

    int get_instance_variable_count() {
        return num_instance_variables;
    }

    int get_global_variable_count() {
        return num_global_variables;
    }

    int get_state_variable_count() {
        return num_state_variables;
    }

    void visit_binary_expression(BinaryExpression* node) override;

    void visit_function_call(FunctionCall* node) override;

    void visit_name(Name* node) override;

    void visit_prime_name(PrimeName* node) override;

    void visit_solve_block(SolveBlock* node) override;

    void visit_statement_block(StatementBlock* node) override;

    void visit_unary_expression(UnaryExpression* node) override;

    void visit_if_statement(IfStatement* node) override;

    void visit_else_if_statement(ElseIfStatement* node) override;

    void visit_program(Program* node) override;

    void visit_plot_block(PlotBlock* node) override {
        measure_performance(node);
    }

    void visit_initial_block(InitialBlock* node) override {
        measure_performance(node);
    }

    void visit_constructor_block(ConstructorBlock* node) override {
        measure_performance(node);
    }

    void visit_destructor_block(DestructorBlock* node) override {
        measure_performance(node);
    }

    void visit_derivative_block(DerivativeBlock* node) override {
        measure_performance(node);
    }

    void visit_linear_block(LinearBlock* node) override {
        measure_performance(node);
    }

    void visit_non_linear_block(NonLinearBlock* node) override {
        measure_performance(node);
    }

    void visit_discrete_block(DiscreteBlock* node) override {
        measure_performance(node);
    }

    void visit_partial_block(PartialBlock* node) override {
        measure_performance(node);
    }

    void visit_function_table_block(FunctionTableBlock* node) override {
        measure_performance(node);
    }

    void visit_function_block(FunctionBlock* node) override {
        measure_performance(node);
    }

    void visit_procedure_block(ProcedureBlock* node) override {
        measure_performance(node);
    }

    void visit_net_receive_block(NetReceiveBlock* node) override {
        measure_performance(node);
    }

    void visit_breakpoint_block(BreakpointBlock* node) override {
        measure_performance(node);
    }

    void visit_terminal_block(TerminalBlock* node) override {
        measure_performance(node);
    }

    void visit_before_block(BeforeBlock* node) override {
        measure_performance(node);
    }

    void visit_after_block(AfterBlock* node) override {
        measure_performance(node);
    }

    void visit_ba_block(BABlock* node) override {
        measure_performance(node);
    }

    void visit_for_netcon(ForNetcon* node) override {
        measure_performance(node);
    }

    void visit_kinetic_block(KineticBlock* node) override {
        measure_performance(node);
    }

    void visit_match_block(MatchBlock* node) override {
        measure_performance(node);
    }

    /// certain constructs needs to be excluded from usage counting
    /// and hence need to provide empty implementations

    void visit_conductance_hint(ConductanceHint* /*node*/) override {
    }

    void visit_local_list_statement(LocalListStatement* /*node*/) override {
    }

    void visit_suffix(Suffix* /*node*/) override {
    }

    void visit_useion(Useion* /*node*/) override {
    }

    void visit_valence(Valence* /*node*/) override {
    }

    void print(std::stringstream& ss) {
        ss << stream.str();
    }
};

#endif
