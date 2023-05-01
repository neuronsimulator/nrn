/** @file deflate.hpp
 *  @copyright (c) 1988-90 Duke University
 */
#pragma once
#include "errcodes.hpp"
#include "newton_struct.h"

#include <cmath>

namespace neuron::scopmath {
inline constexpr auto CONTINUE = 0;
/**
 * Finds the real roots of a polynomial with real coefficients by Newtonian deflation. MAXITERS is
 * the maximum number of iterations of the Newton algorithm for root finding allowed before exiting
 * (currently set to 50). Convergence achieved when value of polynomial (remainder) <= ZERO and the
 * change in the approximation to the root between successive iterations is <= CONVERGE.
 *
 * r = approximation to root of p(x)
 * p(r) = remainder (= 0 at true root)
 * p(x)/(x - r) = q(x)
 * q(r) = slope
 * r' = new approximation to root
 *    = r - remainder/slope
 *
 * @param[in] degree degree of polynomial
 * @param[in] pcoeff real coefficients of array polynomial
 * @param[out] root real roots of polynomial array
 *
 * @returns if successful: number of roots found; if unsucessful: negative of error code
 */
inline int deflate(double degree, double* pcoeff, double* root) {
    int n, ndeg, iter, i, status = CONTINUE;
    double *qcoeff, r = 0.01, slope, remainder, rnew;

    /* r is the current approximation to the next root to be found */

    ndeg = (int) (degree + 0.1);
    n = ndeg;
    qcoeff = makevector(ndeg);
    while (n > 1) {
        iter = 0;
        qcoeff[n - 1] = pcoeff[n];

        /* Begin Newton iteration */

        while (status == CONTINUE) {
            slope = qcoeff[n - 1];
            for (i = n - 1; i > 0; i--) {
                /*
                 * Compute values of polynomial (remainder) and quotient
                 * (slope) by synthetic division using Horner's method
                 */
                qcoeff[i - 1] = pcoeff[i] + qcoeff[i] * r;
                slope = qcoeff[i - 1] + slope * r;
            }
            remainder = pcoeff[0] + qcoeff[0] * r;

            /* Check for error */

            if (fabs(slope) < ROUNDOFF) {
                status = DIVCHECK;
                break;
            } else if (++iter >= MAXITERS) {
                status = EXCEED_ITERS;
                break;
            }

            if (status == CONTINUE) {
                /* Update approximation to root and test for convergence */

                rnew = r - remainder / slope;
                if (fabs(remainder) <= ZERO && fabs(rnew - r) <= CONVERGE)
                    break;
                r = rnew;
            }
        } /* End Newton iteration */

        if (status != CONTINUE)
            break;

        root[ndeg - n] = r;
        /*
         * Put quotient coefficients into original polynomial coefficient
         * array and decrement the degree of the polynomial to deflate on the
         * next pass.
         */
        for (i = 0; i < n; i++)
            pcoeff[i] = qcoeff[i];
        n--;
    } /* End of loop over degrees > 1 */

    if (n == ndeg) /* No roots found */
    {
        freevector(qcoeff);
        return (-status);
    }

    /* Calculate final root */

    root[ndeg - n] = -pcoeff[0] / pcoeff[1];
    freevector(qcoeff);
    return (ndeg - n + 1);
}
}  // namespace neuron::scopmath
using neuron::scopmath::deflate;
