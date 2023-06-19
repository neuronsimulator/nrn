/** @file crout.hpp
 *  @copyright (c) 1987-90 Duke University
 */
#pragma once
#include "errcodes.hpp"
#include "newton_struct.h"

#include <cmath>
namespace neuron::scopmath {
/**
 * Performs an LU triangular factorization of a real matrix by the Crout
 * algorithm using partial pivoting. Rows are not normalized; implicit
 * equilibration is used. ROUNDOFF is the minimal value for a pivot element
 * without its being considered too close to zero (currently set to 1.0E-20).
 *
 * @param n Number of rows of the matrix.
 * @param a Double precision matrix to be factored.
 * @param perm Permutation vector to store row interchanges.
 *
 * Output: a, factors required to transform the constant vector in the set of
 * simultaneous equations are stored in the lower triangle; factors for back
 * substitution are stored in the upper triangle. perm, permutation vector to
 * store row interchanges.
 *
 * @returns 0 if no error; 2 if matrix is singular or ill-conditioned.
 */
template <typename Matrix, typename Permutation>
int crout(int n, Matrix& a, Permutation& perm) {
    int i, j, k, r, pivot, irow, save_i = 0, krow;
    double sum, *rowmax, equil_1, equil_2;

    /* Initialize permutation and rowmax vectors */

    rowmax = makevector(n);
    for (i = 0; i < n; i++) {
        perm[i] = i;
        k = 0;
        for (j = 1; j < n; j++)
            if (fabs(a[i][j]) > fabs(a[i][k]))
                k = j;
        rowmax[i] = a[i][k];
    }

    /* Loop over rows and columns r */

    for (r = 0; r < n; r++) {
        /*
         * Operate on rth column.  This produces the lower triangular matrix
         * of terms needed to transform the constant vector.
         */

        for (i = r; i < n; i++) {
            sum = 0.0;
            irow = perm[i];
            for (k = 0; k < r; k++) {
                krow = perm[k];
                sum += a[irow][k] * a[krow][r];
            }
            a[irow][r] -= sum;
        }

        /* Find row containing the pivot in the rth column */

        pivot = perm[r];
        equil_1 = fabs(a[pivot][r] / rowmax[pivot]);
        for (i = r + 1; i < n; i++) {
            irow = perm[i];
            equil_2 = fabs(a[irow][r] / rowmax[irow]);
            if (equil_2 > equil_1) {
                /* make irow the new pivot row */

                pivot = irow;
                save_i = i;
                equil_1 = equil_2;
            }
        }

        /* Interchange entries in permutation vector if necessary */

        if (pivot != perm[r]) {
            perm[save_i] = perm[r];
            perm[r] = pivot;
        }

        /* Check that pivot element is not too small */

        if (fabs(a[pivot][r]) < ROUNDOFF)
            return (SINGULAR);

        /*
         * Operate on row in rth position.  This produces the upper
         * triangular matrix whose diagonal elements are assumed to be unity.
         * This matrix is used in the back substitution algorithm.
         */

        for (j = r + 1; j < n; j++) {
            sum = 0.0;
            for (k = 0; k < r; k++) {
                krow = perm[k];
                sum += a[pivot][k] * a[krow][j];
            }
            a[pivot][j] = (a[pivot][j] - sum) / a[pivot][r];
        }
    }
    freevector(rowmax);
    return (SUCCESS);
}

/**
 * Performs forward substitution algorithm to transform the constant vector in
 * the linear simultaneous equations to be consistent with the factored matrix.
 * Then performs back substitution to find the solution to the simultaneous
 * linear equations.
 *
 * @param n Number of rows of the matrix
 * @param a Double precision matrix containing the factored matrix of
 * coefficients of the linear equations.
 * @param b Vector of function values
 * @param perm Permutation vector to store row interchanges
 *
 * Output: p[y[i]] contains the solution vector.
 */
template <typename Array, typename Matrix, typename Permutation>
int solve(int n, Matrix& a, double* b, Permutation& perm, Array& p, int* y) {
    auto const y_ = [&p, y](auto arg) -> auto& {
        return p[y[arg]];
    };
    auto const b_ = [b](auto arg) -> auto& {
        return b[arg];
    };
    int i, j, pivot;
    double sum;

    /* Perform forward substitution with pivoting */
    if (y) {
        for (i = 0; i < n; i++) {
            pivot = perm[i];
            sum = 0.0;
            for (j = 0; j < i; j++)
                sum += a[pivot][j] * (y_(j));
            y_(i) = (b_(pivot) - sum) / a[pivot][i];
        }

        /*
         * Note that the y vector is already in the correct order for back
         * substitution.  Perform back substitution, pivoting the matrix but not
         * the y vector.  There is no need to divide by the diagonal element as
         * this is assumed to be unity.
         */

        for (i = n - 1; i >= 0; i--) {
            pivot = perm[i];
            sum = 0.0;
            for (j = i + 1; j < n; j++)
                sum += a[pivot][j] * (y_(j));
            y_(i) -= sum;
        }
    } else {
        for (i = 0; i < n; i++) {
            pivot = perm[i];
            sum = 0.0;
            for (j = 0; j < i; j++)
                sum += a[pivot][j] * (p[j]);
            p[i] = (b_(pivot) - sum) / a[pivot][i];
        }

        /*
         * Note that the y vector is already in the correct order for back
         * substitution.  Perform back substitution, pivoting the matrix but not
         * the y vector.  There is no need to divide by the diagonal element as
         * this is assumed to be unity.
         */

        for (i = n - 1; i >= 0; i--) {
            pivot = perm[i];
            sum = 0.0;
            for (j = i + 1; j < n; j++)
                sum += a[pivot][j] * (p[j]);
            p[i] -= sum;
        }
    }
    return 0;
}
}  // namespace neuron::scopmath
using neuron::scopmath::crout;
using neuron::scopmath::solve;
