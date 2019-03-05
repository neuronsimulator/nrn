/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <string>

namespace nmodl {
namespace parser {
namespace diffeq {

/**
 * \class Term
 * \brief Represent a term in differential equation and it's "current" solution
 *
 * When differential equation is parsed, each variable/term is represented
 * by this class. As expressions are formed, like a+b, the solution gets
 * updated
 */

struct Term {
    /// expression being solved
    std::string expr;

    /// derivative of the expression
    std::string deriv = "0.0";

    /// \todo : need to check in neuron implementation?
    std::string a = "0.0";
    std::string b = "0.0";

    Term() = default;

    Term(const std::string& expr, const std::string& state);

    Term(std::string expr, std::string deriv, std::string a, std::string b)
        : expr(expr)
        , deriv(deriv)
        , a(a)
        , b(b) {}

    /// helper routines used in parser

    bool deriv_nonzero() {
        return deriv != "0.0";
    }

    bool a_nonzero() {
        return a != "0.0";
    }

    bool b_nonzero() {
        return b != "0.0";
    }

    void print();
};


/**
 * \class DiffEqContext
 * \brief Helper class used by driver and parser while solving diff equation
 *
 */

class DiffEqContext {
    /// name of the solve method
    std::string method;

    /// name of the state variable
    std::string state;

    /// rhs of equation
    std::string rhs;

    /// order of the diff equation
    int order = 0;

    /// if equation is non-linear then expression to use during code generation
    std::string expr_for_nonlinear;

    /// return solution for cnexp method
    std::string get_cnexp_solution();

    /// return solution for non-cnexp method
    std::string get_non_cnexp_solution();

    /// for non-cnexp methods : return the solution based on if equation is linear or not
    std::string get_cvode_linear_diffeq();
    std::string get_cvode_nonlinear_diffeq();

    /// \todo: methods inherited neuron implementation
    std::string cvode_deriv();
    std::string cvode_eqnrhs();

  public:
    /// "final" solution of the equation
    Term solution;

    /// if derivative of the equation is invalid
    bool deriv_invalid = false;

    /// if equation itself is invalid
    bool eqn_invalid = false;

    DiffEqContext() = default;

    DiffEqContext(std::string state, int order, std::string rhs, std::string method)
        : state(state)
        , order(order)
        , rhs(rhs)
        , method(method) {}

    /// return the state variable
    std::string state_variable() const {
        return state;
    }

    /// return solution of the differential equation
    std::string get_solution(bool& cnexp_possible);

    /// return expression with Dstate added
    std::string get_expr_for_nonlinear();

    /// print the context (for debugging)
    void print();
};

}  // namespace diffeq
}  // namespace parser
}  // namespace nmodl
