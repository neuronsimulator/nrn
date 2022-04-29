/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <string>
#include <utility>

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

    /// \todo Need to check in neuron implementation?
    std::string a = "0.0";
    std::string b = "0.0";

    Term() = default;

    Term(const std::string& expr, const std::string& state);

    Term(std::string expr, std::string deriv, std::string a, std::string b)
        : expr(std::move(expr))
        , deriv(std::move(deriv))
        , a(std::move(a))
        , b(std::move(b)) {}

    /// helper routines used in parser

    bool deriv_nonzero() const {
        return deriv != "0.0";
    }

    bool a_nonzero() const {
        return a != "0.0";
    }

    bool b_nonzero() const {
        return b != "0.0";
    }

    void print() const;
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

    /// return solution for cnexp method
    std::string get_cnexp_solution() const;

    /// return solution for euler method
    std::string get_euler_solution() const;

    /// return solution for non-cnexp method
    std::string get_non_cnexp_solution() const;

    /// for non-cnexp methods : return the solution based on if equation is linear or not
    std::string get_cvode_linear_diffeq() const;
    std::string get_cvode_nonlinear_diffeq() const;

    /// \todo Methods inherited neuron implementation
    std::string cvode_deriv() const;
    std::string cvode_eqnrhs() const;

  public:
    /// "final" solution of the equation
    Term solution;

    /// if derivative of the equation is invalid
    bool deriv_invalid = false;

    /// if equation itself is invalid
    bool eqn_invalid = false;

    DiffEqContext() = default;

    DiffEqContext(std::string state, int /* order */, std::string rhs, std::string method)
        : state(std::move(state))
        , rhs(std::move(rhs))
        , method(std::move(method)) {}

    /// return the state variable
    const std::string& state_variable() const {
        return state;
    }

    /// return solution of the differential equation
    std::string get_solution(bool& cnexp_possible);

    /// return expression with Dstate added
    std::string get_expr_for_nonlinear() const;

    /// print the context (for debugging)
    void print() const;
};

}  // namespace diffeq
}  // namespace parser
}  // namespace nmodl
