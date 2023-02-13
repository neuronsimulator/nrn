/** @file newton.hpp
 *  @copyright (c) 1987-90 Duke University
 */
#pragma once
#include "errcodes.hpp"
#include "newton_struct.h"

#include <array>
#include <cmath>
#include <cstdlib>

namespace neuron::scopmath {
/**
 * Creates the Jacobian matrix by computing partial derivatives by finite
 * central differences. If the column variable is nonzero, an increment of 2% of
 * the variable is used. STEP is the minimum increment allowed; it is currently
 * set to 1.0E-6.
 *
 * @param n number of variables
 * @param x pointer to array of addresses of the solution vector elements
 * @param p array of parameter values
 * @param pfunc pointer to function which computes the deviation from zero of
 * each equation in the model.
 * @param value pointer to array of addresses of function values.
 *
 * Output: jacobian, computed jacobian matrix.
 */
template <std::size_t n, typename Array, typename Matrix>
int buildjacobian(int* index, Array& x, int (*pfunc)(), double* value, Matrix& jacobian) {
    std::array<double, n> high_value{}, low_value{};
    // Compute partial derivatives by central finite differences
    if (index) {
        for (auto j = 0; j < n; j++) {
            auto const increment = std::fmax(std::fabs(0.02 * (x[index[j]])), STEP);
            x[index[j]] += increment;
            pfunc();
            for (auto i = 0; i < n; i++) {
                high_value[i] = value[i];
            }
            x[index[j]] -= 2.0 * increment;
            pfunc();
            for (auto i = 0; i < n; i++) {
                low_value[i] = value[i];
                // Insert partials into jth column of Jacobian matrix
                jacobian[i][j] = (high_value[i] - low_value[i]) / (2.0 * increment);
            }

            // Restore original variable and function values.
            x[index[j]] += increment;
            pfunc();
        }
    } else {
        for (auto j = 0; j < n; j++) {
            auto const increment = std::fmax(std::fabs(0.02 * (x[j])), STEP);
            x[j] += increment;
            pfunc();
            for (auto i = 0; i < n; i++) {
                high_value[i] = value[i];
            }
            x[j] -= 2.0 * increment;
            pfunc();
            for (auto i = 0; i < n; i++) {
                low_value[i] = value[i];
                // Insert partials into jth column of Jacobian matrix
                jacobian[i][j] = (high_value[i] - low_value[i]) / (2.0 * increment);
            }
            // Restore original variable and function values.
            x[j] += increment;
            pfunc();
        }
    }
    return 0;
}
/**
 * Iteratively solves simultaneous nonlinear equations by Newton's method, using
 * a Jacobian matrix computed by finite differences.
 *
 * @tparam n number of variables to solve for.
 *
 * @param x pointer to array of the solution vector elements possibly indexed by
 * index
 * @param p array of parameter values
 * @param pfunc pointer to function which computes the deviation from zero of
 * each equation in the model.
 * @param value pointer to array to array of the function values.
 *
 * @return 0 if no error; 2 if matrix is singular or ill-conditioned; 1 if
 * maximum iterations exceeded.
 *
 * Output: x contains the solution value or the most recent iteration's result
 * in the event of an error.
 */
template <std::size_t n, typename Array>
int newton(int* index, Array&& x, int (*pfunc)(), double* value) {
    // Create arrays for Jacobian, variable increments, function values, and
    // permutation vector
    std::array<int, n> perm{};
    std::array<double, n> delta_x{};
    std::array<std::array<double, n>, n> jacobian{};

    int count{}, error{};
    for (double change{1.0}, max_dev{}; count++ < MAXITERS;) {
        if (change > MAXCHANGE) {
            // Recalculate Jacobian matrix if solution has changed by more than
            // MAXCHANGE
            buildjacobian<n>(index, x, pfunc, value, jacobian);
            for (auto i = 0; i < n; i++) {
                // Required correction to function values
                value[i] = -value[i];
            }
            if ((error = crout(n, jacobian, perm)) != SUCCESS) {
                break;
            }
        }
        solve(n, jacobian, value, perm, delta_x, nullptr);

        // Update solution vector and compute norms of delta_x and value
        change = 0.0;
        if (index) {
            for (auto i = 0; i < n; i++) {
                if (double temp; fabs(x[index[i]]) > ZERO &&
                                 (temp = fabs(delta_x[i] / (x[index[i]]))) > change) {
                    change = temp;
                }
                x[index[i]] += delta_x[i];
            }
        } else {
            for (auto i = 0; i < n; i++) {
                if (double temp; fabs(x[i]) > ZERO && (temp = fabs(delta_x[i] / (x[i]))) > change) {
                    change = temp;
                }
                x[i] += delta_x[i];
            }
        }
        // Evaluate function values with new solution
        pfunc();
        max_dev = 0.0;
        for (auto i = 0; i < n; i++) {
            // Required correction to function values
            value[i] = -value[i];
            if (double temp; (temp = fabs(value[i])) > max_dev) {
                max_dev = temp;
            }
        }

        // Check for convergence or maximum iterations
        if (change <= CONVERGE && max_dev <= ZERO) {
            break;
        }
        if (count == MAXITERS) {
            error = EXCEED_ITERS;
            break;
        }
    }
    return error;
}
}  // namespace neuron::scopmath
using neuron::scopmath::newton;
