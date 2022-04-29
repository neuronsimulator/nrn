/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <iostream>

#include "lexer/diffeq_lexer.hpp"
#include "utils/string_utils.hpp"


namespace nmodl {
namespace parser {
namespace diffeq {

Term::Term(const std::string& expr, const std::string& state)
    : expr(expr)
    , b(expr) {
    if (expr == state) {
        deriv = "1.0";
        a = "1.0";
        b = "0.0";
    }
}


void Term::print() const {
    std::cout << "Term [expr, deriv, a, b] : " << expr << ", " << deriv << ", " << a << ", " << b
              << '\n';
}


void DiffEqContext::print() const {
    std::cout << "-----------------DiffEq Context----------------" << std::endl;
    std::cout << "deriv_invalid = " << deriv_invalid << std::endl;
    std::cout << "eqn_invalid   = " << eqn_invalid << std::endl;
    std::cout << "expr          = " << solution.expr << std::endl;
    std::cout << "deriv         = " << solution.deriv << std::endl;
    std::cout << "a             = " << solution.a << std::endl;
    std::cout << "b             = " << solution.b << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;
}


std::string DiffEqContext::cvode_deriv() const {
    std::string result;
    if (!deriv_invalid) {
        result = solution.deriv;
    }
    return result;
}


std::string DiffEqContext::cvode_eqnrhs() const {
    std::string result;
    if (!eqn_invalid) {
        result = solution.b;
    }
    return result;
}


/**
 * When non-cnexp method is used, for solving non-linear equations we need original
 * expression but with replacing every state variable with (state+0.001). In order
 * to do this we scan original expression and build new by replacing only state variable.
 */
std::string DiffEqContext::get_expr_for_nonlinear() const {
    std::string expression;

    /// build lexer instance
    std::istringstream in(rhs);
    DiffeqLexer scanner(&in);

    /// scan entire expression
    while (true) {
        auto sym = scanner.next_token();
        auto token_type = sym.type_get();
        if (token_type == DiffeqParser::by_type(DiffeqParser::token::END).type_get()) {
            break;
        }
        /// extract value of the token and check if it is a token
        auto value = sym.value.as<std::string>();
        if (value == state) {
            expression += "(" + value + "+0.001)";
        } else {
            expression += value;
        }
    }
    return expression;
}


/**
 * Return solution for non-cnexp method and when equation is linear
 */
std::string DiffEqContext::get_cvode_linear_diffeq() const {
    auto result = "D" + state + " = " + "D" + state + "/(1.0-dt*(" + solution.deriv + "))";
    return result;
}


/**
 * Return solution for non-cnexp method and when equation is non-linear.
 */
std::string DiffEqContext::get_cvode_nonlinear_diffeq() const {
    std::string expr = get_expr_for_nonlinear();
    std::string sol = "D" + state + " = " + "D" + state + "/(1.0-dt*(";
    sol += "((" + expr + ")-(" + solution.expr + "))/0.001))";
    return sol;
}


/**
 * Return solution for cnexp method
 */
std::string DiffEqContext::get_cnexp_solution() const {
    auto a = cvode_deriv();
    auto b = cvode_eqnrhs();
    /**
     * a is zero typically when rhs doesn't have state variable
     * in this case we have to remove "last" term "/(0.0)" from b
     * and then return : state = state - dt*(b)
     */
    std::string result;
    if (a == "0.0") {
        std::string suffix = "/(0.0)";
        b = b.substr(0, b.size() - suffix.size());
        result = state + " = " + state + "-dt*(" + b + ")";
    } else {
        result = state + " = " + state + "+(1.0-exp(dt*(" + a + ")))*(" + b + "-" + state + ")";
    }
    return result;
}


/**
 * Return solution for euler method
 */
std::string DiffEqContext::get_euler_solution() const {
    return state + " = " + state + "+dt*(" + rhs + ")";
}


/**
 * Return solution for non-cnexp method
 */
std::string DiffEqContext::get_non_cnexp_solution() const {
    std::string result;
    if (!deriv_invalid) {
        result = get_cvode_linear_diffeq();
    } else if (deriv_invalid and eqn_invalid) {
        result = get_cvode_nonlinear_diffeq();
    } else {
        throw std::runtime_error("Error in differential equation solver with non-cnexp");
    }
    return result;
}


/**
 * Return the solution for differential equation based on method used.
 *
 * \todo Currently we have tested cnexp, euler and derivimplicit methods with
 * all equations from BBP models. Need to test this against various other mod
 * files, especially kinetic schemes, reaction-diffusion etc.
 */
std::string DiffEqContext::get_solution(bool& cnexp_possible) {
    std::string solution;
    if (method == "euler") {
        cnexp_possible = false;
        solution = get_euler_solution();
    } else if (method == "cnexp" && !(deriv_invalid && eqn_invalid)) {
        cnexp_possible = true;
        solution = get_cnexp_solution();
    } else {
        cnexp_possible = false;
        solution = get_non_cnexp_solution();
    }
    return solution;
}

}  // namespace diffeq
}  // namespace parser
}  // namespace nmodl
