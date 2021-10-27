/******************************************************************************
 *
 * File: crout.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

/*--------------------------------------------------------------*/
/*                                                              */
/*  CROUT 		                                        */
/*                                                              */
/*    Performs an LU triangular factorization of a real matrix  */
/*    by the Crout algorithm using partial pivoting.  Rows are  */
/*    not normalized; implicit equilibration is used.  ROUNDOFF */
/*    is the minimal value for a pivot element without its being*/
/*    considered too close to zero (currently set to 1.0E-20).  */
/*                                                              */
/*  Returns: 0 if no error; 2 if matrix is singular or ill-	*/
/*			      conditioned			*/
/*                                                              */
/*  Calling sequence:  crout(n, a, perm)	                */
/*                                                              */
/*  Arguments:                                                  */
/*                                                              */
/*    Input:  n, integer, number of rows of the matrix          */
/*                                                              */
/*            a, double precision matrix to be factored         */
/*                                                              */
/*    Output:                                                   */
/*                                                              */
/*            a, factors required to transform the constant     */
/*               vector in the set of simultaneous equations    */
/*               are stored in the lower triangle; factors for  */
/*               back substitution are stored in the upper      */
/*               triangle.                                      */
/*                                                              */
/*            perm, integer, permutation vector to store row    */
/*                           interchanges                       */
/*                                                              */
/*  Functions called: makevector(), freevector()		*/
/*                                                              */
/*--------------------------------------------------------------*/
#include <cmath>
#include "coreneuron/mechanism/mech/cfile/scoplib.h"
#include "coreneuron/sim/scopmath/newton_struct.h"
#include "coreneuron/sim/scopmath/errcodes.h"
namespace coreneuron {
#define ix(arg) ((arg) *_STRIDE)

/* having a differnt permutation per instance may not be a good idea */
int nrn_crout_thread(NewtonSpace* ns, int n, double** a, int* perm, _threadargsproto_) {
    int save_i = 0;

    /* Initialize permutation and rowmax vectors */

    double* rowmax = ns->rowmax;
    for (int i = 0; i < n; i++) {
        perm[ix(i)] = i;
        int k = 0;
        for (int j = 1; j < n; j++)
            if (fabs(a[i][ix(j)]) > fabs(a[i][ix(k)]))
                k = j;
        rowmax[ix(i)] = a[i][ix(k)];
    }

    /* Loop over rows and columns r */

    for (int r = 0; r < n; r++) {
        /*
         * Operate on rth column.  This produces the lower triangular matrix
         * of terms needed to transform the constant vector.
         */

        for (int i = r; i < n; i++) {
            double sum = 0.0;
            int irow = perm[ix(i)];
            for (int k = 0; k < r; k++) {
                int krow = perm[ix(k)];
                sum += a[irow][ix(k)] * a[krow][ix(r)];
            }
            a[irow][ix(r)] -= sum;
        }

        /* Find row containing the pivot in the rth column */

        int pivot = perm[ix(r)];
        double equil_1 = fabs(a[pivot][ix(r)] / rowmax[ix(pivot)]);
        for (int i = r + 1; i < n; i++) {
            int irow = perm[ix(i)];
            double equil_2 = fabs(a[irow][ix(r)] / rowmax[ix(irow)]);
            if (equil_2 > equil_1) {
                /* make irow the new pivot row */

                pivot = irow;
                save_i = i;
                equil_1 = equil_2;
            }
        }

        /* Interchange entries in permutation vector if necessary */

        if (pivot != perm[ix(r)]) {
            perm[ix(save_i)] = perm[ix(r)];
            perm[ix(r)] = pivot;
        }

        /* Check that pivot element is not too small */

        if (fabs(a[pivot][ix(r)]) < ROUNDOFF)
            return (SINGULAR);

        /*
         * Operate on row in rth position.  This produces the upper
         * triangular matrix whose diagonal elements are assumed to be unity.
         * This matrix is used in the back substitution algorithm.
         */

        for (int j = r + 1; j < n; j++) {
            double sum = 0.0;
            for (int k = 0; k < r; k++) {
                int krow = perm[ix(k)];
                sum += a[pivot][ix(k)] * a[krow][ix(j)];
            }
            a[pivot][ix(j)] = (a[pivot][ix(j)] - sum) / a[pivot][ix(r)];
        }
    }
    return (SUCCESS);
}

/*--------------------------------------------------------------*/
/*                                                              */
/*  SOLVE()                                          		*/
/*                                                              */
/*    Performs forward substitution algorithm to transform the  */
/*    constant vector in the linear simultaneous equations to   */
/*    be consistent with the factored matrix.  Then performs    */
/*    back substitution to find the solution to the simultane-  */
/*    ous linear equations.                                     */
/*                                                              */
/*  Returns: no return variable                                 */
/*                                                              */
/*  Calling sequence:  solve(n, a, b, perm, p, y)               */
/*                                                              */
/*  Arguments:                                                  */
/*                                                              */
/*    Input:  n, integer, number of rows of the matrix          */
/*                                                              */
/*            a, double precision matrix containing the         */
/*               factored matrix of coefficients of the linear  */
/*               equations.                                     */
/*                                                              */
/*            b, vector of function values			*/
/*                                                              */
/*            perm, integer, permutation vector to store row    */
/*                           interchanges                       */
/*                                                              */
/*    Output:                                                   */
/*                                                              */
/*            p[y[i]] contains the solution vector                    */
/*                                                              */
/*--------------------------------------------------------------*/
void nrn_scopmath_solve_thread(int n,
                               double** a,
                               double* b,
                               int* perm,
                               double* p,
                               int* y,
                               _threadargsproto_)
#define y_(arg) _p[y[arg] * _STRIDE]
#define b_(arg) b[ix(arg)]
{
    /* Perform forward substitution with pivoting */
    // if (y) { // pgacc bug. nullptr on cpu but not on GPU
    if (0) {
        for (int i = 0; i < n; i++) {
            int pivot = perm[ix(i)];
            double sum = 0.0;
            for (int j = 0; j < i; j++)
                sum += a[pivot][ix(j)] * (y_(j));
            y_(i) = (b_(pivot) - sum) / a[pivot][ix(i)];
        }

        /*
         * Note that the y vector is already in the correct order for back
         * substitution.  Perform back substitution, pivoting the matrix but not
         * the y vector.  There is no need to divide by the diagonal element as
         * this is assumed to be unity.
         */

        for (int i = n - 1; i >= 0; i--) {
            int pivot = perm[ix(i)];
            double sum = 0.0;
            for (int j = i + 1; j < n; j++)
                sum += a[pivot][ix(j)] * (y_(j));
            y_(i) -= sum;
        }
    } else {
        for (int i = 0; i < n; i++) {
            int pivot = perm[ix(i)];
            double sum = 0.0;
            if (i > 0) {  // pgacc bug. with i==0 the following loop executes once
                for (int j = 0; j < i; j++) {
                    sum += a[pivot][ix(j)] * (p[ix(j)]);
                }
            }
            p[ix(i)] = (b_(pivot) - sum) / a[pivot][ix(i)];
        }

        /*
         * Note that the y vector is already in the correct order for back
         * substitution.  Perform back substitution, pivoting the matrix but not
         * the y vector.  There is no need to divide by the diagonal element as
         * this is assumed to be unity.
         */

        for (int i = n - 1; i >= 0; i--) {
            int pivot = perm[ix(i)];
            double sum = 0.0;
            for (int j = i + 1; j < n; j++)
                sum += a[pivot][ix(j)] * (p[ix(j)]);
            p[ix(i)] -= sum;
        }
    }
}
}  // namespace coreneuron
