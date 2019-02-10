#include "symtab/symbol.hpp"
#include "utils/logger.hpp"
#include "visitors/sympy_solver_visitor.hpp"
#include "visitor_utils.hpp"
#include <iostream>
using namespace ast;
namespace py = pybind11;
using namespace py::literals;
using namespace syminfo;

void SympySolverVisitor::visit_solve_block(SolveBlock* node) {
    auto method = node->get_method();
    if (method) {
        solve_method = method->get_value()->eval();
    }
}

void SympySolverVisitor::visit_diff_eq_expression(DiffEqExpression* node) {
    if (solve_method != cnexp_method) {
        logger->warn(
            "SympySolverVisitor: solve method not cnexp, so not integrating "
            "expression analytically");
        return;
    }

    auto& lhs = node->get_expression()->lhs;
    auto& rhs = node->get_expression()->rhs;

    if (!lhs->is_var_name()) {
        logger->warn("SympySolverVisitor: LHS of differential equation is not a VariableName");
        return;
    }
    auto lhs_name = std::dynamic_pointer_cast<VarName>(lhs)->get_name();
    if (!lhs_name->is_prime_name()) {
        logger->warn("SympySolverVisitor: LHS of differential equation is not a PrimeName");
        return;
    }

    std::unordered_set<std::string> var_strings{};
    for (auto v : vars) {
        var_strings.insert(v->get_name());
    }

    auto locals = py::dict("equation_string"_a = nmodl::to_nmodl(node), "vars"_a = var_strings);
    py::exec(R"(
            from nmodl.ode import integrate2c
            exception_message = ""
            try:
                solution = integrate2c(equation_string, vars)
            except Exception as e:
                # if we fail, fail silently and return empty string
                solution = ""
                exception_message = str(e)
        )",
             py::globals(), locals);

    auto solution = locals["solution"].cast<std::string>();
    auto exception_message = locals["exception_message"].cast<std::string>();
    if (!exception_message.empty()) {
        logger->warn("SympySolverVisitor: python exception: " + exception_message);
    }
    if (!solution.empty()) {
        auto statement = create_statement(solution);
        auto expr_statement = std::dynamic_pointer_cast<ExpressionStatement>(statement);
        auto bin_expr =
            std::dynamic_pointer_cast<BinaryExpression>(expr_statement->get_expression());
        lhs.reset(bin_expr->lhs->clone());
        rhs.reset(bin_expr->rhs->clone());
    } else {
        logger->warn("SympySolverVisitor: analytic solution to differential equation not possible");
    }
}

void SympySolverVisitor::visit_derivative_block(ast::DerivativeBlock* node) {
    // get any local vars
    auto symtab = node->get_statement_block()->get_symbol_table();
    if (symtab) {
        auto localvars = symtab->get_variables_with_properties(NmodlType::local_var);
        for (auto v : localvars) {
            vars.push_back(v);
        }
    }
    node->visit_children(this);
}

void SympySolverVisitor::visit_program(ast::Program* node) {
    // get global vars
    auto symtab = node->get_symbol_table();
    NmodlType property = NmodlType::global_var | NmodlType::range_var | NmodlType::param_assign |
                         NmodlType::extern_var | NmodlType::prime_name | NmodlType::dependent_def |
                         NmodlType::read_ion_var | NmodlType::write_ion_var |
                         NmodlType::nonspecific_cur_var | NmodlType::electrode_cur_var |
                         NmodlType::section_var | NmodlType::constant_var |
                         NmodlType::extern_neuron_variable | NmodlType::state_var;
    if (symtab) {
        vars = symtab->get_variables_with_properties(property);
    }
    node->visit_children(this);
}