/** @file newton.hpp
 *  @copyright (c) 1987-90 Duke University
 */
#pragma once
#include "crout_thread.hpp"
#include "errcodes.hpp"
#include "newton_struct.h"
#include "oc_ansi.h"

#include <cmath>
#include <cstdlib>

namespace neuron::scopmath {
namespace detail::newton_thread {
/**
 * Creates the Jacobian matrix by computing partial derivatives by finite
 * central differences. If the column variable is nonzero, an increment of 2% of
 * the variable is used. STEP is the minimum increment allowed; it is currently
 * set to 1.0E-6.
 * @param n number of variables
 * @param x pointer to array of addresses of the solution vector elements
 * @param p array of parameter values
 * @param pfunc pointer to function which computes the deviation from zero of
 * each equation in the model.
 * @param value pointer to array of addresses of function values
 * @param[out] jacobian computed jacobian matrix
 */
template <typename Array, typename Callable, typename IndexArray, typename... Args>
void buildjacobian(NewtonSpace* ns,
                   int n,
                   IndexArray index,
                   Array x,
                   Callable pfunc,
                   double* value,
                   double** jacobian,
                   Args&&... args) {
    int i, j;
    double increment, *high_value, *low_value;

    high_value = ns->high_value;
    low_value = ns->low_value;

    /* Compute partial derivatives by central finite differences */

    if (index) {
        for (j = 0; j < n; j++) {
            increment = std::fmax(fabs(0.02 * (x[index[j]])), STEP);
            x[index[j]] += increment;
            pfunc(args...);
            for (i = 0; i < n; i++)
                high_value[i] = value[i];
            x[index[j]] -= 2.0 * increment;
            pfunc(args...);
            for (i = 0; i < n; i++) {
                low_value[i] = value[i];

                /* Insert partials into jth column of Jacobian matrix */

                jacobian[i][j] = (high_value[i] - low_value[i]) / (2.0 * increment);
            }

            /* Restore original variable and function values. */

            x[index[j]] += increment;
            pfunc(args...);
        }
    } else {
        for (j = 0; j < n; j++) {
            increment = std::fmax(fabs(0.02 * (x[j])), STEP);
            x[j] += increment;
            pfunc(args...);
            for (i = 0; i < n; i++)
                high_value[i] = value[i];
            x[j] -= 2.0 * increment;
            pfunc(args...);
            for (i = 0; i < n; i++) {
                low_value[i] = value[i];

                /* Insert partials into jth column of Jacobian matrix */

                jacobian[i][j] = (high_value[i] - low_value[i]) / (2.0 * increment);
            }

            /* Restore original variable and function values. */

            x[j] += increment;
            pfunc(args...);
        }
    }
}
}  // namespace detail::newton_thread
/**
 * Iteratively solves simultaneous nonlinear equations by Newton's method, using
 * a Jacobian matrix computed by finite differences.
 *
 * @param n number of variables to solve for.
 * @param x pointer to array of the solution vector elements possibly indexed by
 * index
 * @param p array of parameter values
 * @param pfunc pointer to function which computes the deviation from zero of
 * each equation in the model.
 * @param value pointer to array to array of the function values.
 * @param[out] x contains the solution value or the most recent iteration's
 * result in the event of an error.
 *
 * @returns 0 if no error; 2 if matrix is singular or ill-conditioned; 1 if
 * maximum iterations exceeded
 */
template <typename Array, typename Function, typename IndexArray, typename... Args>
int nrn_newton_thread(NewtonSpace* ns,
                      int n,
                      IndexArray index,
                      Array x,
                      Function pfunc,
                      double* value,
                      Args&&... args) {
    int i, count = 0, error, *perm;
    double **jacobian, *delta_x, change = 1.0, max_dev, temp;

    /*
     * Create arrays for Jacobian, variable increments, function values, and
     * permutation vector
     */
    delta_x = ns->delta_x;
    jacobian = ns->jacobian;
    perm = ns->perm;
    /* Iteration loop */

    while (count++ < MAXITERS) {
        if (change > MAXCHANGE) {
            /*
             * Recalculate Jacobian matrix if solution has changed by more
             * than MAXCHANGE
             */

            detail::newton_thread::buildjacobian(ns, n, index, x, pfunc, value, jacobian, args...);
            for (i = 0; i < n; i++)
                value[i] = -value[i]; /* Required correction to
                                       * function values */

            if ((error = nrn_crout_thread(ns, n, jacobian, perm)) != SUCCESS)
                break;
        }
        nrn_scopmath_solve_thread(n, jacobian, value, perm, delta_x, nullptr);

        /* Update solution vector and compute norms of delta_x and value */

        change = 0.0;
        if (index) {
            for (i = 0; i < n; i++) {
                if (fabs(x[index[i]]) > ZERO && (temp = fabs(delta_x[i] / (x[index[i]]))) > change)
                    change = temp;
                x[index[i]] += delta_x[i];
            }
        } else {
            for (i = 0; i < n; i++) {
                if (fabs(x[i]) > ZERO && (temp = fabs(delta_x[i] / (x[i]))) > change)
                    change = temp;
                x[i] += delta_x[i];
            }
        }
        // Evaluate function values with new solution
        pfunc(args...);
        max_dev = 0.0;
        for (i = 0; i < n; i++) {
            value[i] = -value[i]; /* Required correction to function
                                   * values */
            if ((temp = fabs(value[i])) > max_dev)
                max_dev = temp;
        }

        /* Check for convergence or maximum iterations */

        if (change <= CONVERGE && max_dev <= ZERO)
            break;
        if (count == MAXITERS) {
            error = EXCEED_ITERS;
            break;
        }
    } /* end of while loop */

    return (error);
}

inline NewtonSpace* nrn_cons_newtonspace(int n) {
    NewtonSpace* ns = (NewtonSpace*) hoc_Emalloc(sizeof(NewtonSpace));
    ns->n = n;
    ns->delta_x = makevector(n);
    ns->jacobian = makematrix(n, n);
    ns->perm = (int*) hoc_Emalloc((unsigned) (n * sizeof(int)));
    ns->high_value = makevector(n);
    ns->low_value = makevector(n);
    ns->rowmax = makevector(n);
    return ns;
}

inline void nrn_destroy_newtonspace(NewtonSpace* ns) {
    free((char*) ns->perm);
    freevector(ns->delta_x);
    freematrix(ns->jacobian);
    freevector(ns->high_value);
    freevector(ns->low_value);
    freevector(ns->rowmax);
    free(ns);
}
}  // namespace neuron::scopmath
using neuron::scopmath::nrn_cons_newtonspace;
using neuron::scopmath::nrn_destroy_newtonspace;
using neuron::scopmath::nrn_newton_thread;
