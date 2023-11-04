/*
# =============================================================================
# Originally crout.c from SCoP library, Copyright (c) 1987-90 Duke University
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

namespace coreneuron {
#if defined(scopmath_crout_ix) || defined(scopmath_crout_y) || defined(scopmath_crout_b)
#error "naming clash on crout_thread.hpp-internal macros"
#endif
#define scopmath_crout_b(arg)  b[scopmath_crout_ix(arg)]
#define scopmath_crout_ix(arg) CNRN_FLAT_INDEX_IML_ROW(arg)
#define scopmath_crout_y(arg)  _p[CNRN_FLAT_INDEX_IML_ROW(y[arg])]

/**
 * Performs an LU triangular factorization of a real matrix by the Crout
 * algorithm using partial pivoting. Rows are not normalized; implicit
 * equilibration is used. ROUNDOFF is the minimal value for a pivot element
 * without its being considered too close to zero (currently set to 1.0E-20).
 *
 * @return 0 if no error; 2 if matrix is singular or ill-conditioned
 * @param n number of rows of the matrix
 * @param a double precision matrix to be factored
 * @param[out] a factors required to transform the constant vector in the set of
 *               simultaneous equations are stored in the lower triangle;
 *               factors for back substitution are stored in the upper triangle.
 * @param[out] perm permutation vector to store row interchanges
 *
 * @note Having a differnt permutation per instance may not be a good idea.
 */
inline int nrn_crout_thread(NewtonSpace* ns, int n, double** a, int* perm, _threadargsproto_) {
    int save_i = 0;

    /* Initialize permutation and rowmax vectors */
    double* rowmax = ns->rowmax;
    for (int i = 0; i < n; i++) {
        perm[scopmath_crout_ix(i)] = i;
        int k = 0;
        for (int j = 1; j < n; j++)
            if (fabs(a[i][scopmath_crout_ix(j)]) > fabs(a[i][scopmath_crout_ix(k)]))
                k = j;
        rowmax[scopmath_crout_ix(i)] = a[i][scopmath_crout_ix(k)];
    }

    /* Loop over rows and columns r */
    for (int r = 0; r < n; r++) {
        /*
         * Operate on rth column.  This produces the lower triangular matrix
         * of terms needed to transform the constant vector.
         */

        for (int i = r; i < n; i++) {
            double sum = 0.0;
            int irow = perm[scopmath_crout_ix(i)];
            for (int k = 0; k < r; k++) {
                int krow = perm[scopmath_crout_ix(k)];
                sum += a[irow][scopmath_crout_ix(k)] * a[krow][scopmath_crout_ix(r)];
            }
            a[irow][scopmath_crout_ix(r)] -= sum;
        }

        /* Find row containing the pivot in the rth column */
        int pivot = perm[scopmath_crout_ix(r)];
        double equil_1 = fabs(a[pivot][scopmath_crout_ix(r)] / rowmax[scopmath_crout_ix(pivot)]);
        for (int i = r + 1; i < n; i++) {
            int irow = perm[scopmath_crout_ix(i)];
            double equil_2 = fabs(a[irow][scopmath_crout_ix(r)] / rowmax[scopmath_crout_ix(irow)]);
            if (equil_2 > equil_1) {
                /* make irow the new pivot row */

                pivot = irow;
                save_i = i;
                equil_1 = equil_2;
            }
        }

        /* Interchange entries in permutation vector if necessary */
        if (pivot != perm[scopmath_crout_ix(r)]) {
            perm[scopmath_crout_ix(save_i)] = perm[scopmath_crout_ix(r)];
            perm[scopmath_crout_ix(r)] = pivot;
        }

        /* Check that pivot element is not too small */
        if (fabs(a[pivot][scopmath_crout_ix(r)]) < ROUNDOFF)
            return SINGULAR;

        /*
         * Operate on row in rth position.  This produces the upper
         * triangular matrix whose diagonal elements are assumed to be unity.
         * This matrix is used in the back substitution algorithm.
         */
        for (int j = r + 1; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < r; k++) {
                int krow = perm[scopmath_crout_ix(k)];
                sum += a[pivot][scopmath_crout_ix(k)] * a[krow][scopmath_crout_ix(j)];
            }
            a[pivot][scopmath_crout_ix(j)] = (a[pivot][scopmath_crout_ix(j)] - sum) /
                                             a[pivot][scopmath_crout_ix(r)];
        }
    }
    return SUCCESS;
}

/**
 * Performs forward substitution algorithm to transform the constant vector in
 * the linear simultaneous equations to be consistent with the factored matrix.
 * Then performs back substitution to find the solution to the simultaneous
 * linear equations.
 *
 * @param n number of rows of the matrix
 * @param a double precision matrix containing the factored matrix of
 *          coefficients of the linear equations
 * @param b vector of function values
 * @param perm permutation vector to store row interchanges
 * @param[out] p[y[i]] contains the solution vector
 */
inline void nrn_scopmath_solve_thread(int n,
                                      double** a,
                                      double* b,
                                      int* perm,
                                      double* p,
                                      int* y,
                                      _threadargsproto_) {
    /* Perform forward substitution with pivoting */
    // if (y) { // pgacc bug. nullptr on cpu but not on GPU
    if (0) {
        for (int i = 0; i < n; i++) {
            int pivot = perm[scopmath_crout_ix(i)];
            double sum = 0.0;
            for (int j = 0; j < i; j++)
                sum += a[pivot][scopmath_crout_ix(j)] * (scopmath_crout_y(j));
            scopmath_crout_y(i) = (scopmath_crout_b(pivot) - sum) / a[pivot][scopmath_crout_ix(i)];
        }

        /*
         * Note that the y vector is already in the correct order for back
         * substitution.  Perform back substitution, pivoting the matrix but not
         * the y vector.  There is no need to divide by the diagonal element as
         * this is assumed to be unity.
         */

        for (int i = n - 1; i >= 0; i--) {
            int pivot = perm[scopmath_crout_ix(i)];
            double sum = 0.0;
            for (int j = i + 1; j < n; j++)
                sum += a[pivot][scopmath_crout_ix(j)] * (scopmath_crout_y(j));
            scopmath_crout_y(i) -= sum;
        }
    } else {
        for (int i = 0; i < n; i++) {
            int pivot = perm[scopmath_crout_ix(i)];
            double sum = 0.0;
            if (i > 0) {  // pgacc bug. with i==0 the following loop executes once
                for (int j = 0; j < i; j++) {
                    sum += a[pivot][scopmath_crout_ix(j)] * (p[scopmath_crout_ix(j)]);
                }
            }
            p[scopmath_crout_ix(i)] = (scopmath_crout_b(pivot) - sum) /
                                      a[pivot][scopmath_crout_ix(i)];
        }

        /*
         * Note that the y vector is already in the correct order for back
         * substitution.  Perform back substitution, pivoting the matrix but not
         * the y vector.  There is no need to divide by the diagonal element as
         * this is assumed to be unity.
         */
        for (int i = n - 1; i >= 0; i--) {
            int pivot = perm[scopmath_crout_ix(i)];
            double sum = 0.0;
            for (int j = i + 1; j < n; j++)
                sum += a[pivot][scopmath_crout_ix(j)] * (p[scopmath_crout_ix(j)]);
            p[scopmath_crout_ix(i)] -= sum;
        }
    }
}
#undef scopmath_crout_b
#undef scopmath_crout_ix
#undef scopmath_crout_y
}  // namespace coreneuron
