/** @file newton.hpp
 *  @copyright (c) 1987-90 Duke University
 */
#pragma once
#include "errcodes.hpp"
#include "newton_struct.h"

#include <cmath>
#include <cstdlib>

namespace neuron::scopmath {
/**
 * Creates the Jacobian matrix by computing partial derivatives by finite central differences. If
 * the column variable is nonzero, an increment of 2% of the variable is used. STEP is the minimum
 * increment allowed; it is currently set to 1.0E-6.
 *
 * @param n number of variables
 * @param x pointer to array of addresses of the solution vector elements
 * @param p array of parameter values
 * @param pfunc pointer to function which computes the deviation from zero of each equation in the
 * model.
 * @param value pointer to array of addresses of function values.
 *
 * Output: jacobian, computed jacobian matrix.
 */
template <typename Array>
int buildjacobian(int n, int* index, Array x, int (*pfunc)(), double* value, double** jacobian) {
    int i, j;
    double increment, *high_value, *low_value;

    high_value = makevector(n);
    low_value = makevector(n);

    /* Compute partial derivatives by central finite differences */

    if (index) {
        for (j = 0; j < n; j++) {
            increment = std::fmax(std::fabs(0.02 * (x[index[j]])), STEP);
            x[index[j]] += increment;
            (*pfunc)();
            for (i = 0; i < n; i++)
                high_value[i] = value[i];
            x[index[j]] -= 2.0 * increment;
            (*pfunc)();
            for (i = 0; i < n; i++) {
                low_value[i] = value[i];

                /* Insert partials into jth column of Jacobian matrix */

                jacobian[i][j] = (high_value[i] - low_value[i]) / (2.0 * increment);
            }

            /* Restore original variable and function values. */

            x[index[j]] += increment;
            (*pfunc)();
        }
    } else {
        for (j = 0; j < n; j++) {
            increment = std::fmax(std::fabs(0.02 * (x[j])), STEP);
            x[j] += increment;
            (*pfunc)();
            for (i = 0; i < n; i++)
                high_value[i] = value[i];
            x[j] -= 2.0 * increment;
            (*pfunc)();
            for (i = 0; i < n; i++) {
                low_value[i] = value[i];

                /* Insert partials into jth column of Jacobian matrix */

                jacobian[i][j] = (high_value[i] - low_value[i]) / (2.0 * increment);
            }

            /* Restore original variable and function values. */

            x[j] += increment;
            (*pfunc)();
        }
    }
    freevector(high_value);
    freevector(low_value);
    return 0;
}
/**
 * Iteratively solves simultaneous nonlinear equations by Newton's method, using a Jacobian matrix
 * computed by finite differences.
 *
 * @param n number of variables to solve for.
 * @param x pointer to array of the solution vector elements possibly indexed by index
 * @param p array of parameter values
 * @param pfunc pointer to function which computes the deviation from zero of each equation in the
 * model.
 * @param value pointer to array to array of the function values.
 *
 * @return 0 if no error; 2 if matrix is singular or ill-conditioned; 1 if maximum iterations
 * exceeded.
 *
 * Output: x contains the solution value or the most recent iteration's result in the event of an
 * error.
 */
template <typename Array>
int newton(int n, int* index, Array x, int (*pfunc)(), double* value) {
    int i, count = 0, error, *perm;
    double **jacobian, *delta_x, change = 1.0, max_dev, temp;

    /*
     * Create arrays for Jacobian, variable increments, function values, and
     * permutation vector
     */

    delta_x = makevector(n);
    jacobian = makematrix(n, n);
    perm = (int*) malloc((unsigned) (n * sizeof(int)));

    /* Iteration loop */

    while (count++ < MAXITERS) {
        if (change > MAXCHANGE) {
            /*
             * Recalculate Jacobian matrix if solution has changed by more
             * than MAXCHANGE
             */

            buildjacobian(n, index, x, pfunc, value, jacobian);
            for (i = 0; i < n; i++)
                value[i] = -value[i]; /* Required correction to
                                       * function values */

            if ((error = crout(n, jacobian, perm)) != SUCCESS)
                break;
        }
        solve(n, jacobian, value, perm, delta_x, (int*) 0);

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
        (*pfunc)(); /* Evaluate function values with new solution */
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

    free((char*) perm);
    freevector(delta_x);
    freematrix(jacobian);
    return (error);
}
}  // namespace neuron::scopmath
using neuron::scopmath::buildjacobian;
using neuron::scopmath::newton;
