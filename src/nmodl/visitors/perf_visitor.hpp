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
    std::shared_ptr<symtab::SymbolTable> current_symtab;

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

    void update_memory_ops(std::string name);

    bool symbol_to_skip(std::shared_ptr<symtab::Symbol> symbol);

    bool is_local_variable(std::shared_ptr<symtab::Symbol> symbol);

    void measure_performance(AST* node);

    void add_perf_to_printer(PerfStat& perf);

  public:
    PerfVisitor() = default;

    explicit PerfVisitor(std::string filename);

    void compact_json(bool flag) {
        printer->compact_json(flag);
    }

    PerfStat& get_total_perfstat() {
        return total_perf;
    }

    void visitBinaryExpression(BinaryExpression* node) override;

    void visitFunctionCall(FunctionCall* node) override;

    void visitName(Name* node) override;

    void visitPrimeName(PrimeName* node) override;

    void visitSolveBlock(SolveBlock* node) override;

    void visitStatementBlock(StatementBlock* node) override;

    void visitUnaryExpression(UnaryExpression* node) override;

    void visitIfStatement(IfStatement* node) override;

    void visitElseIfStatement(ElseIfStatement* node) override;

    void visitProgram(Program* node) override;

    void visitPlotBlock(PlotBlock* node) override {
        measure_performance(node);
    }

    void visitInitialBlock(InitialBlock* node) override {
        measure_performance(node);
    }

    void visitConstructorBlock(ConstructorBlock* node) override {
        measure_performance(node);
    }

    void visitDestructorBlock(DestructorBlock* node) override {
        measure_performance(node);
    }

    void visitDerivativeBlock(DerivativeBlock* node) override {
        measure_performance(node);
    }

    void visitLinearBlock(LinearBlock* node) override {
        measure_performance(node);
    }

    void visitNonLinearBlock(NonLinearBlock* node) override {
        measure_performance(node);
    }

    void visitDiscreteBlock(DiscreteBlock* node) override {
        measure_performance(node);
    }

    void visitPartialBlock(PartialBlock* node) override {
        measure_performance(node);
    }

    void visitFunctionTableBlock(FunctionTableBlock* node) override {
        measure_performance(node);
    }

    void visitFunctionBlock(FunctionBlock* node) override {
        measure_performance(node);
    }

    void visitProcedureBlock(ProcedureBlock* node) override {
        measure_performance(node);
    }

    void visitNetReceiveBlock(NetReceiveBlock* node) override {
        measure_performance(node);
    }

    void visitBreakpointBlock(BreakpointBlock* node) override {
        measure_performance(node);
    }

    void visitTerminalBlock(TerminalBlock* node) override {
        measure_performance(node);
    }

    void visitBeforeBlock(BeforeBlock* node) override {
        measure_performance(node);
    }

    void visitAfterBlock(AfterBlock* node) override {
        measure_performance(node);
    }

    void visitBABlock(BABlock* node) override {
        measure_performance(node);
    }

    void visitForNetcon(ForNetcon* node) override {
        measure_performance(node);
    }

    void visitKineticBlock(KineticBlock* node) override {
        measure_performance(node);
    }

    void visitMatchBlock(MatchBlock* node) override {
        measure_performance(node);
    }

    /// certain constructs needs to be excluded from usage counting
    /// and hence need to provide empty implementations

    void visitConductanceHint(ConductanceHint* /*node*/) override {
    }

    void visitLocalListStatement(LocalListStatement* /*node*/) override {
    }

    void visitSuffix(Suffix* /*node*/) override {
    }

    void visitUseion(Useion* /*node*/) override {
    }

    void visitValence(Valence* /*node*/) override {
    }

    void print(std::stringstream& ss) {
        ss << stream.str();
    }
};

#endif
