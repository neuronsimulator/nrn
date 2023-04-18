/** @file euler.hpp
 *  @copyright (c) 1984-90 Duke University
 */
#pragma once
#include "errcodes.hpp"
/**
 * @brief Euler integration subroutine for a set of first-order ordinary differential equations.
 *
 * @param ninits ???
 * @param neqn Number of equations.
 * @param var  Pointer to array of addresses of the	state variables.
 * @param der  Pointer to array of addresses of the	derivatives of the state variables.
 * @param p    Double precision variable array.
 * @param t    Pointer to current time.
 * @param dt   Integration stepsize.
 * @param func Function that evaluates the derivatives of the dependent variables.
 * @param work Pointer to work array; not used but kept for consistency with other integrators.
 *
 * Output: time, the dependent variables, and the time derivatives of the dependent variables are
 * updated.
 *
 * @return Error code (always SUCCESS for euler)
 */
namespace neuron::scopmath {
template <typename Array, typename IndexArray>
int euler(int ninits,
          int neqn,
          IndexArray var,
          IndexArray der,
          Array p,
          double* t,
          double dt,
          int (*func)(),
          double** work) {
    // Calculate the derivatives
    func();

    // Update dependent variables
    for (int i = 0; i < neqn; i++) {
        p[var[i]] += dt * p[der[i]];
    }

    return SUCCESS;
}
}  // namespace neuron::scopmath
using neuron::scopmath::euler;
