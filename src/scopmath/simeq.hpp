/** @file simeq.hpp
 *  @copyright (c) 1984-90 Duke University
 */
#pragma once
#include "errcodes.hpp"

#include <cmath>
#include <cstdlib>

namespace neuron::scopmath {
/**
 * Solves simultaneous linear equations by Gaussian elimination, with partial
 * pivoting (row interchange only). The coefficient matrix is destroyed in this
 * subroutine. The indices for the pivot rows are stored in the vector perm[];
 * perm[i] is the index of the row to be used as the ith row of the matrix. Rows
 * are not actually swapped. If a pivot or diagonal element is < ROUNDOFF,
 * currently set to 1.e-20, the matrix is considered singular or
 * ill-conditioned.
 *
 * @param coef coef[n][n+1] augmented matrix of coefficients
 * @param n # of equations
 * @param soln[out] solution vector
 *
 * @return 0 if no error; 2 if matrix is singular or ill-conditioned
 */
template <typename Array>
int simeq(int n, double** coef, Array soln, int* index) {
    static int np = 0;
    static int* perm = nullptr;

    /* Create and initialize permutation vector */
    if (np < n) {
        delete[] perm;
        perm = new int[n];
        np = n;
    }
    for (auto i = 0; i < n; i++) {
        perm[i] = i;
    }
    // The following loop is performed once for each row and implicitly reduces
    // the diagonal element to 1 and all elements below it to 0. To save
    // arithmetic operations, elements of the coefficient matrix which will not
    // later be used are not operated on (i.e., only values to the right of the
    // diagonal in a row are modified by the algorithm).
    int isave = 0;
    for (auto j = 0; j < n; j++) {
        // First, find pivot row (i.e., row of the lower triangle which, when
        // transposed into the jth position, will put the largest column element on
        // the diagonal. Store pivot index in perm[].
        auto ipivot = perm[j];
        for (auto i = j + 1; i < n; i++) {
            auto const jrow = perm[i];
            if (fabs(coef[ipivot][j]) < fabs(coef[jrow][j])) {
                ipivot = jrow;
                isave = i;
            }
        }

        /* Now be sure the pivot found isn't too small  */

        if (fabs(coef[ipivot][j]) < ROUNDOFF)
            return (SINGULAR);

        /* Swap row indices in perm[] if pivot is not perm[j] */

        if (ipivot != perm[j]) {
            perm[isave] = perm[j];
            perm[j] = ipivot;
        }

        /* Row normalization */

        for (auto kcol = j + 1; kcol <= n; kcol++)
            coef[ipivot][kcol] /= coef[ipivot][j];

        /* Row reduction */

        for (auto i = j + 1; i < n; i++) {
            auto const jrow = perm[i];
            for (auto kcol = j + 1; kcol <= n; kcol++) {
                coef[jrow][kcol] -= coef[ipivot][kcol] * coef[jrow][j];
            }
        }
    } /* end loop over all rows */

    /* Back substitution */

    if (index) {
        for (auto i = n - 1; i >= 0; i--) {
            auto const jrow = perm[i];
            soln[index[i]] = coef[jrow][n];
            for (auto j = i + 1; j < n; j++) {
                soln[index[i]] -= coef[jrow][j] * soln[index[j]];
            }
        }
    } else {
        for (auto i = n - 1; i >= 0; i--) {
            auto const jrow = perm[i];
            soln[i] = coef[jrow][n];
            for (auto j = i + 1; j < n; j++) {
                soln[i] -= coef[jrow][j] * soln[j];
            }
        }
    }
    return SUCCESS;
}
}  // namespace neuron::scopmath
using neuron::scopmath::simeq;
