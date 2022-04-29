/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include "parser/diffeq_context.hpp"

namespace nmodl {
namespace parser {

/**
 * \brief Helper functions for solving differential equations
 *
 * Differential euqation parser (diffeq.yy) needs to solve derivative
 * of various binary expressions and those are provided below. This
 * implementation is based on original neuron implementation.
 *
 * \todo The implementations here are verbose and has duplicate code.
 * Need to revisit this, may be using better library like symengine
 * altogether.
 */
namespace diffeq {

/// operators beign supported as part of binary expressions
enum class MathOp { add = 1, sub, mul, div };

template <MathOp Op>
inline Term eval_derivative(const Term& first,
                            const Term& second,
                            bool& deriv_invalid,
                            bool& eqn_invalid);


/// implement (f(x) + g(x))' = f'(x) + g'(x)
template <>
inline Term eval_derivative<MathOp::add>(const Term& first,
                                         const Term& second,
                                         bool& deriv_invalid,
                                         bool& eqn_invalid) {
    Term solution;
    solution.expr = first.expr + "+" + second.expr;

    if (first.deriv_nonzero() && second.deriv_nonzero()) {
        solution.deriv = first.deriv + "+" + second.deriv;
    } else if (first.deriv_nonzero()) {
        solution.deriv = first.deriv;
    } else if (second.deriv_nonzero()) {
        solution.deriv = second.deriv;
    }

    if (first.a_nonzero() && second.a_nonzero()) {
        solution.a = first.a + "+" + second.a;
    } else if (first.a_nonzero()) {
        solution.a = first.a;
    } else if (second.a_nonzero()) {
        solution.a = second.a;
    }

    if (first.b_nonzero() && second.b_nonzero()) {
        solution.b = first.b + "+" + second.b;
    } else if (first.b_nonzero()) {
        solution.b = first.b;
    } else if (second.b_nonzero()) {
        solution.b = second.b;
    }

    return solution;
}


/// implement (f(x) - g(x))' = f'(x) - g'(x)
template <>
inline Term eval_derivative<MathOp::sub>(const Term& first,
                                         const Term& second,
                                         bool& deriv_invalid,
                                         bool& eqn_invalid) {
    Term solution;
    solution.expr = first.expr + "-" + second.expr;

    if (first.deriv_nonzero() && second.deriv_nonzero()) {
        solution.deriv = first.deriv + "-" + second.deriv;
    } else if (first.deriv_nonzero()) {
        solution.deriv = first.deriv;
    } else if (second.deriv_nonzero()) {
        solution.deriv = "(-" + second.deriv + ")";
    }

    if (first.a_nonzero() && second.a_nonzero()) {
        solution.a = first.a + "-" + second.a;
    } else if (first.a_nonzero()) {
        solution.a = first.a;
    } else if (second.a_nonzero()) {
        solution.a = "(-" + second.a + ")";
    }

    if (first.b_nonzero() && second.b_nonzero()) {
        solution.b = first.b + "-" + second.b;
    } else if (first.b_nonzero()) {
        solution.b = first.b;
    } else if (second.b_nonzero()) {
        solution.b = "(-" + second.b + ")";
    }

    return solution;
}


/// implement (f(x) * g(x))' = f'(x)g(x) + f(x)g'(x)
template <>
inline Term eval_derivative<MathOp::mul>(const Term& first,
                                         const Term& second,
                                         bool& deriv_invalid,
                                         bool& eqn_invalid) {
    Term solution;
    solution.expr = first.expr + "*" + second.expr;

    if (first.deriv_nonzero() && second.deriv_nonzero()) {
        solution.deriv = "((" + first.deriv + ")*(" + second.expr + ")";
        solution.deriv += "+(" + first.expr + ")*(" + second.deriv + "))";
    } else if (first.deriv_nonzero()) {
        solution.deriv = "(" + first.deriv + ")*(" + second.expr + ")";
    } else if (second.deriv_nonzero()) {
        solution.deriv = "(" + first.expr + ")*(" + second.deriv + ")";
    }

    if (first.a_nonzero() && second.a_nonzero()) {
        eqn_invalid = true;
    } else if (first.a_nonzero() && second.b_nonzero()) {
        solution.a = "(" + first.a + ")*(" + second.b + ")";
    } else if (second.a_nonzero() && first.b_nonzero()) {
        solution.a = "(" + first.b + ")*(" + second.a + ")";
    }

    if (first.b_nonzero() && second.b_nonzero()) {
        solution.b = "(" + first.b + ")*(" + second.b + ")";
    }

    return solution;
}


/**
 * Implement (f(x) / g(x))' = (f'(x)g(x) - f(x)g'(x))/g^2(x)
 * Note that the implementation is very limited for the g(x)
 * and this needs to be discussed with Michael.
 */
template <>
inline Term eval_derivative<MathOp::div>(const Term& first,
                                         const Term& second,
                                         bool& deriv_invalid,
                                         bool& eqn_invalid) {
    Term solution;
    solution.expr = first.expr + "/" + second.expr;

    if (second.deriv_nonzero()) {
        deriv_invalid = true;
    } else if (first.deriv_nonzero()) {
        solution.deriv = "(" + first.deriv + ")/" + second.expr;
    }

    if (second.a_nonzero()) {
        eqn_invalid = true;
    } else if (first.a_nonzero()) {
        solution.a = "(" + first.a + ")/" + second.expr;
    }

    if (first.b_nonzero()) {
        solution.b = "(" + first.b + ")/" + second.expr;
    }

    return solution;
}

}  // namespace diffeq
}  // namespace parser
}  // namespace nmodl
