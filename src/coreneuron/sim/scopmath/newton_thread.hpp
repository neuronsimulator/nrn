/*
# =============================================================================
# Originally newton.c from SCoP library, Copyright (c) 1987-90 Duke University
# =============================================================================
# Subsequent extensive prototype and memory layout changes for CoreNEURON
#
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#pragma once
#include "coreneuron/sim/scopmath/errcodes.h"
#include "coreneuron/sim/scopmath/newton_struct.h"
#include "coreneuron/sim/scopmath/crout_thread.hpp"

#include <algorithm>
#include <cmath>

namespace coreneuron {
#if defined(scopmath_newton_ix) || defined(scopmath_newton_s) || defined(scopmath_newton_x)
#error "naming clash on newton_thread.hpp-internal macros"
#endif
#define scopmath_newton_ix(arg) ((arg) *_STRIDE)
#define scopmath_newton_s(arg)  _p[s[arg] * _STRIDE]
#define scopmath_newton_x(arg)  _p[(arg) *_STRIDE]
namespace detail {
/**
 * @brief Calculate the Jacobian matrix using finite central differences.
 *
 * Creates the Jacobian matrix by computing partial derivatives by finite
 * central differences. If the column variable is nonzero, an increment of 2% of
 * the variable is used. STEP is the minimum increment allowed; it is currently
 * set to 1.0E-6.
 *
 * @param n number of variables
 * @param x pointer to array of addresses of the solution vector elements
 * @param p array of parameter values
 * @param func callable that computes the deviation from zero of each equation
 *             in the model
 * @param value pointer to array of addresses of function values
 * @param[out] jacobian computed jacobian matrix
 */
template <typename F>
void nrn_buildjacobian_thread(NewtonSpace* ns,
                              int n,
                              int* index,
                              F const& func,
                              double* value,
                              double** jacobian,
                              _threadargsproto_) {
    double* high_value = ns->high_value;
    double* low_value = ns->low_value;

    /* Compute partial derivatives by central finite differences */

    for (int j = 0; j < n; j++) {
        double increment = std::max(std::fabs(0.02 * (scopmath_newton_x(index[j]))), STEP);
        scopmath_newton_x(index[j]) += increment;
        func(_threadargs_);  // std::invoke in C++17
        for (int i = 0; i < n; i++)
            high_value[scopmath_newton_ix(i)] = value[scopmath_newton_ix(i)];
        scopmath_newton_x(index[j]) -= 2.0 * increment;
        func(_threadargs_);  // std::invoke in C++17
        for (int i = 0; i < n; i++) {
            low_value[scopmath_newton_ix(i)] = value[scopmath_newton_ix(i)];

            /* Insert partials into jth column of Jacobian matrix */

            jacobian[i][scopmath_newton_ix(j)] = (high_value[scopmath_newton_ix(i)] -
                                                  low_value[scopmath_newton_ix(i)]) /
                                                 (2.0 * increment);
        }

        /* Restore original variable and function values. */

        scopmath_newton_x(index[j]) += increment;
        func(_threadargs_);  // std::invoke in C++17
    }
}
#undef scopmath_newton_x
}  // namespace detail

/**
 * Iteratively solves simultaneous nonlinear equations by Newton's method, using
 * a Jacobian matrix computed by finite differences.
 *
 * @return 0 if no error; 2 if matrix is singular or ill-conditioned; 1 if
 *         maximum iterations exceeded.
 * @param n number of variables to solve for
 * @param x pointer to array of the solution vector elements possibly indexed by
 *          index
 * @param p array of parameter values
 * @param func callable that computes the deviation from zero of each equation
 *             in the model
 * @param value pointer to array to array of the function values
 * @param[out] x contains the solution value or the most recent iteration's
 *               result in the event of an error.
 */
template <typename F>
inline int nrn_newton_thread(NewtonSpace* ns,
                             int n,
                             int* s,
                             F func,
                             double* value,
                             _threadargsproto_) {
    int count = 0, error = 0;
    double change = 1.0, max_dev, temp;
    int done = 0;
    /*
     * Create arrays for Jacobian, variable increments, function values, and
     * permutation vector
     */
    double* delta_x = ns->delta_x;
    double** jacobian = ns->jacobian;
    int* perm = ns->perm;
    /* Iteration loop */
    while (!done) {
        if (count++ >= MAXITERS) {
            error = EXCEED_ITERS;
            done = 2;
        }
        if (!done && change > MAXCHANGE) {
            /*
             * Recalculate Jacobian matrix if solution has changed by more
             * than MAXCHANGE
             */
            detail::nrn_buildjacobian_thread(ns, n, s, func, value, jacobian, _threadargs_);
            for (int i = 0; i < n; i++)
                value[scopmath_newton_ix(i)] = -value[scopmath_newton_ix(i)]; /* Required correction
                                                                               * to
                                                                               * function values */
            error = nrn_crout_thread(ns, n, jacobian, perm, _threadargs_);
            if (error != SUCCESS) {
                done = 2;
            }
        }

        if (!done) {
            nrn_scopmath_solve_thread(n, jacobian, value, perm, delta_x, (int*) 0, _threadargs_);

            /* Update solution vector and compute norms of delta_x and value */

            change = 0.0;
            if (s) {
                for (int i = 0; i < n; i++) {
                    if (std::fabs(scopmath_newton_s(i)) > ZERO &&
                        (temp = std::fabs(delta_x[scopmath_newton_ix(i)] /
                                          (scopmath_newton_s(i)))) > change)
                        change = temp;
                    scopmath_newton_s(i) += delta_x[scopmath_newton_ix(i)];
                }
            } else {
                for (int i = 0; i < n; i++) {
                    if (std::fabs(scopmath_newton_s(i)) > ZERO &&
                        (temp = std::fabs(delta_x[scopmath_newton_ix(i)] /
                                          (scopmath_newton_s(i)))) > change)
                        change = temp;
                    scopmath_newton_s(i) += delta_x[scopmath_newton_ix(i)];
                }
            }
            // Evaulate function values with new solution.
            func(_threadargs_);  // std::invoke in C++17
            max_dev = 0.0;
            for (int i = 0; i < n; i++) {
                value[scopmath_newton_ix(i)] = -value[scopmath_newton_ix(i)]; /* Required correction
                                                                               * to function
                                                                               * values */
                if ((temp = std::fabs(value[scopmath_newton_ix(i)])) > max_dev)
                    max_dev = temp;
            }

            /* Check for convergence or maximum iterations */

            if (change <= CONVERGE && max_dev <= ZERO) {
                // break;
                done = 1;
            }
        }
    } /* end of while loop */

    return (error);
}
#undef scopmath_newton_ix
#undef scopmath_newton_s

NewtonSpace* nrn_cons_newtonspace(int n, int n_instance);
void nrn_destroy_newtonspace(NewtonSpace* ns);
}  // namespace coreneuron
