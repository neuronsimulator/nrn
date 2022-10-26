/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/
#pragma once

/**
 * \dir
 * \brief Solver for a system of linear equations : Crout matrix decomposition
 *
 * \file
 * \brief Implementation of Crout matrix decomposition (LU decomposition) followed by
 * Forward/Backward substitution
 */

#include <Eigen/Core>
#include <cmath>

#if defined(CORENEURON_ENABLE_GPU) && !defined(DISABLE_OPENACC)
#include "coreneuron/utils/offload.hpp"
#endif

namespace nmodl {
namespace crout {

/**
 * \brief Crout matrix decomposition : in-place LU Decomposition of matrix A.
 *
 * LU decomposition function.
 * Implementation details : (Legacy code) coreneuron/sim/scopmath/crout*
 *
 * Description:
 * This routine uses Crout's method to decompose a row interchanged
 * version of the n x n matrix A into a lower triangular matrix L and a
 * unit upper triangular matrix U such that A = LU.
 * The matrices L and U replace the matrix A so that the original matrix
 * A is destroyed.
 * Note!  In Crout's method the diagonal elements of U are 1 and are not stored.
 * Note!  The determinant of A is the product of the diagonal elements of L.  (det A = det L * det U
 * = det L). The LU decomposition is convenient when one needs to solve the linear equation Ax = B
 * for the vector x while the matrix A is fixed and the vector B is varied.  The routine for solving
 * the linear system Ax = B after performing the LU decomposition for A is solveCrout (see below).
 *
 * The Crout method with partial pivoting is: Determine the pivot row and
 * interchange the current row with the pivot row, then assuming that
 * row k is the current row, k = 0, ..., n - 1 evaluate in order the
 * the following pair of expressions
 * L[i][k] = (A[i][k] - (L[i][0]*U[0][k] + . + L[i][k-1]*U[k-1][k]))
 *          for i = k, ... , n-1,
 * U[k][j] = A[k][j] - (L[k][0]*U[0][j] + ... + L[k][k-1]*U[k-1][j]) / L[k][k]
 *          for j = k+1, ... , n-1.
 * The matrix U forms the upper triangular matrix, and the matrix L
 * forms the lower triangular matrix.
 *
 * \param n The number of rows or columns of the matrix A
 * \param A matrix of size nxn : in-place LU decomposition (C-style arrays : row-major order)
 * \param pivot matrix of size n : The i-th element is the pivot row interchanged with row i
 *
 * @return 0 for SUCCESS || -1 for FAILURE (The matrix A is singular)
 */
#if defined(CORENEURON_ENABLE_GPU) && !defined(DISABLE_OPENACC)
nrn_pragma_acc(routine seq)
nrn_pragma_omp(declare target)
#endif
template <typename T>
EIGEN_DEVICE_FUNC inline int Crout(int n, T* A, int* pivot) {
    // roundoff is the minimal value for a pivot element without its being considered too close to
    // zero
    double roundoff = 1.e-20;
    int status = 0;
    int i, j, k;
    T *p_k{}, *p_row{}, *p_col{};
    T max;

    // For each row and column, k = 0, ..., n-1,
    for (k = 0, p_k = A; k < n; p_k += n, k++) {
        // find the pivot row
        pivot[k] = k;
        max = std::fabs(*(p_k + k));
        for (j = k + 1, p_row = p_k + n; j < n; j++, p_row += n) {
            if (max < std::fabs(*(p_row + k))) {
                max = std::fabs(*(p_row + k));
                pivot[k] = j;
                p_col = p_row;
            }
        }

        // and if the pivot row differs from the current row, then
        // interchange the two rows.
        if (pivot[k] != k)
            for (j = 0; j < n; j++) {
                max = *(p_k + j);
                *(p_k + j) = *(p_col + j);
                *(p_col + j) = max;
            }

        // and if the matrix is singular, return error
        if (std::fabs(*(p_k + k)) < roundoff) {
            status = -1;
        } else {
            // otherwise find the upper triangular matrix elements for row k.
            for (j = k + 1; j < n; j++) {
                *(p_k + j) /= *(p_k + k);
            }

            // update remaining matrix
            for (i = k + 1, p_row = p_k + n; i < n; p_row += n, i++)
                for (j = k + 1; j < n; j++)
                    *(p_row + j) -= *(p_row + k) * *(p_k + j);
        }
    }
    return status;
}
#if defined(CORENEURON_ENABLE_GPU) && !defined(DISABLE_OPENACC)
nrn_pragma_omp(end declare target)
#endif

/**
 * \brief Crout matrix decomposition : Forward/Backward substitution.
 *
 * Forward/Backward substitution function.
 * Implementation details : (Legacy code) coreneuron/sim/scopmath/crout*
 *
 * \param n The number of rows or columns of the matrix LU
 * \param LU LU-factorized matrix (C-style arrays : row-major order)
 * \param B rhs vector
 * \param x solution of (LU)x=B linear system
 * \param pivot matrix of size n : The i-th element is the pivot row interchanged with row i
 *
 * @return 0 for SUCCESS || -1 for FAILURE (The matrix A is singular)
 */
#if defined(CORENEURON_ENABLE_GPU) && !defined(DISABLE_OPENACC)
nrn_pragma_acc(routine seq)
nrn_pragma_omp(declare target)
#endif
template <typename T>
EIGEN_DEVICE_FUNC inline int solveCrout(int n, T* LU, T* B, T* x, int* pivot) {
    // roundoff is the minimal value for a pivot element without its being considered too close to
    // zero
    double roundoff = 1.e-20;
    int status = 0;
    int i, k;
    T* p_k;
    T dum;

    // Solve the linear equation Lx = B for x, where L is a lower
    // triangular matrix.
    for (k = 0, p_k = LU; k < n; p_k += n, k++) {
        if (pivot[k] != k) {
            dum = B[k];
            B[k] = B[pivot[k]];
            B[pivot[k]] = dum;
        }
        x[k] = B[k];
        for (i = 0; i < k; i++)
            x[k] -= x[i] * *(p_k + i);
        x[k] /= *(p_k + k);
    }

    // Solve the linear equation Ux = y, where y is the solution
    // obtained above of Lx = B and U is an upper triangular matrix.
    // The diagonal part of the upper triangular part of the matrix is
    // assumed to be 1.0.
    for (k = n - 1, p_k = LU + n * (n - 1); k >= 0; k--, p_k -= n) {
        if (pivot[k] != k) {
            dum = B[k];
            B[k] = B[pivot[k]];
            B[pivot[k]] = dum;
        }
        for (i = k + 1; i < n; i++)
            x[k] -= x[i] * *(p_k + i);
        if (std::fabs(*(p_k + k)) < roundoff)
            status = -1;
    }

    return status;
}
#if defined(CORENEURON_ENABLE_GPU) && !defined(DISABLE_OPENACC)
nrn_pragma_omp(end declare target)
#endif

}  // namespace crout
}  // namespace nmodl
