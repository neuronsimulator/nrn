#include <sstream>

#include "visitors/cnexp_solve_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "parser/diffeq_driver.hpp"
#include "visitors/visitor_utils.hpp"
#include "symtab/symbol.hpp"

using namespace ast;

void CnexpSolveVisitor::visit_solve_block(SolveBlock* node) {
    if (node->method) {
        solve_method = node->method->value->eval();
    }
}

void CnexpSolveVisitor::visit_derivative_block(DerivativeBlock* node) {
    derivative_block = true;
    node->visit_children(this);
    derivative_block = false;
}

void CnexpSolveVisitor::visit_binary_expression(BinaryExpression* node) {
    auto& lhs = node->lhs;
    auto& rhs = node->rhs;

    /// we have to only solve binary expressions in derivative block
    if (!derivative_block || (node->op.value != BOP_ASSIGN)) {
        return;
    }

    /// lhs of the expression should be variable
    if (!lhs->is_var_name()) {
        return;
    }

    auto name = std::dynamic_pointer_cast<VarName>(lhs)->name;

    if (name->is_prime_name()) {
        /// convert ode into string format
        std::stringstream stream;
        NmodlPrintVisitor v(stream);
        node->visit_children(&v);
        auto equation = stream.str();
        diffeq::Driver diffeq_driver;

        if (solve_method == cnexp_method) {
            std::string solution;
            /// check if ode can be solved with cnexp method
            if (diffeq_driver.cnexp_possible(equation, solution)) {
                auto statement = create_statement(solution);
                auto expr_statement = std::dynamic_pointer_cast<ExpressionStatement>(statement);
                auto bin_expr =
                    std::dynamic_pointer_cast<BinaryExpression>(expr_statement->expression);
                lhs.reset(bin_expr->lhs->clone());
                rhs.reset(bin_expr->rhs->clone());
            } else {
                std::cerr << "cnexp solver not possible";
            }
        } else if (solve_method == derivimplicit_method || solve_method == euler_method) {
            auto varname = "D" + name->get_name();
            auto variable = new ast::Name(new ast::String(varname));
            lhs.reset(variable);
            auto symbol = std::make_shared<symtab::Symbol>(varname, ModToken());
            symbol->set_original_name(name->get_name());
            symbol->created_from_state();
            program_symtab->insert(symbol);
        } else {
            std::cerr << "solver method '" + solve_method + "' not supported";
        }
    }
}

void CnexpSolveVisitor::visit_program(Program* node) {
    program_symtab = node->get_symbol_table();
    node->visit_children(this);
}
