/** @file runge.hpp
 *  @copyright (c) 1984-90 Duke University
 */
#pragma once
#include "errcodes.hpp"
#include "newton_struct.h"
#include "scoplib.h"
namespace neuron::scopmath {
/**
 * Solves simultaneous differential equations using a fourth order Runge-Kutta
 * algorithm. The independent variable is assumed to be time. The dependent
 * variables are indexed from 0 to n - 1; n = no. of differential equations. The
 * state variables and their derivatives must each be in a contiguous block in
 * the variable array and quantities refering to the same variable must appear
 * in the same order in each block.
 *
 * @param ninits ???
 * @param n Number of differential equations.
 * @param y Pointer to array of addresses of the state variables
 * @param d Pointer to array of addresses of the derivatives of the state
 * variables.
 * @param p Double precision variable array.
 * @param t Pointer to current time
 * @param h Integration step size = delta_t.
 * @param dy Function that evaluates the time derivatives of the dependent
 * variables.
 * @param work Double precision work array.
 *
 * Output: p[], double precision array containing the values of the dependent
 * variables and their derivatives at time + h. Time is also updated.
 * @return Error code (always SUCCESS for runge)
 */
template <typename Array, typename IndexArray>
int runge(int ninits,
          int n,
          IndexArray y,
          IndexArray d,
          Array p,
          double* t,
          double h,
          int (*dy)(),
          double** work) {
    auto const d_ = [&d, &p](auto arg) -> auto& {
        return p[d[arg]];
    };
    auto const y_ = [&y, &p](auto arg) -> auto& {
        return p[y[arg]];
    };
    int i;
    double temp;

    if (*work == (double*) 0)
        *work = makevector(n << 1);

    // Get derivatives at current time point.
    dy();

    for (i = 0; i < n; i++) {
        // Store values of state variables at current time
        (*work)[i] = y_(i);

        // Eulerian extrapolation for half a time step
        temp = (h / 2.0) * (d_(i));
        y_(i) += temp;

        // Update solution
        (*work)[n + i] = (*work)[i] + temp / 3.0;
    }

    // Get derivatives at extrapolated values of variables at t + h/2.
    *t += h / 2.0;
    dy();

    for (i = 0; i < n; i++) {
        // Backwards Eulerian extrapolation for half a time step
        temp = h * (d_(i));
        y_(i) = (*work)[i] + temp / 2.0;

        // Update solution
        (*work)[n + i] += temp / 3.0;
    }

    // Get derivatives at new extrapolated values of variables at t + h/2
    dy();

    for (i = 0; i < n; i++) {
        // Extrapolation of variables for full time step, using midpoint rule
        temp = h * (d_(i));
        y_(i) = (*work)[i] + temp;

        // Update solution
        (*work)[n + i] += temp / 3.0;
    }

    // Get derivatives at extrapolated values of variables at t + h
    *t += h / 2.0;
    dy();

    for (i = 0; i < n; i++) {
        // Compute the values of the dependent variables at t + h
        y_(i) = (*work)[n + i] + (h / 6.0) * (d_(i));
    }

    // Restore t to original value -- it will be updated in model()
    *t -= h;

    return SUCCESS;
}
}  // namespace neuron::scopmath
using neuron::scopmath::runge;
