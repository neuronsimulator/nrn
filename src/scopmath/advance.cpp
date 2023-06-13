#include <nrnconf.h>
/******************************************************************************
 *
 * File: advance.c
 *
 * Copyright (c) 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "errcodes.hpp"
#include "scoplib.h"
#include "newton_struct.h"

#include <cstdlib>
#include <cmath>

using namespace neuron::scopmath;  // for errcodes.hpp
static int oldsimeq(int n, double** coef, double* soln);
/*-----------------------------------------------------------------------------
 *
 *  _ADVANCE
 *
 *  This is an experimental numerical method for SCoP-3 which integrates kinetic
 *  rate equations.  It is intended to be used only by models generated by MODL,
 *  and its identity is meant to be concealed from the user.
 *
 */
int _advance(int _ninits,
             int n,
             int* s,
             int* d,
             double* p,
             double* t,
             double dt,
             int (*fun)(),
             double*** pcoef,
             int linflag)
#define d_(arg) p[d[arg]]
#define s_(arg) p[s[arg]]
{
    int i, j, ier;
    double err, **coef;

    if (!*pcoef) {
        *pcoef = makematrix(n + 1, n + 1);
    }
    coef = *pcoef;

    for (i = 0; i < n; i++) { /* save old state */
        d_(i) = s_(i);
    }

#if 0
    {
	int i, j;
	for (i = 0; i < n + 1; i++)
	{
	    for (j = 0; j < n + 1; j++)
	    {
		printf("%10.5g ", coef[i][j]);
	    } printf("\n");
	}
    }
#endif

    for (err = 1, j = 0; err > CONVERGE; j++) {
        zero_matrix(coef, n + 1, n + 1);
        (*fun)();
        if ((ier = oldsimeq(n, coef, coef[n]))) {
            return ier; /* answer in coef[n] */
        }

#if 0
	{
	    int i, j;
	    for (i = 0; i < n + 1; i++)
	    {
		for (j = 0; j < n + 1; j++)
		{
		    printf("%10.5g ", coef[i][j]);
		} printf("\n");
	    }
	}
#endif

        for (err = 0., i = 0; i < n; i++) {
            s_(i) += coef[n][i];
            err += fabs(coef[n][i]);
        }
        if (j > MAXSTEPS) {
            return EXCEED_ITERS;
        }
        if (linflag)
            break;
    }
    zero_matrix(coef, n + 1, n + 1);
    (*fun)();
    for (i = 0; i < n; i++) { /* restore Dstate at t+dt */
        d_(i) = (s_(i) - d_(i)) / dt;
    }
    return SUCCESS;
}

/*--------------------------------------------------------------*/
/*                                                              */
/*   OLSSIMEQ                                                   */
/*                                                              */
/*   Solves simultaneous linear equations by Gaussian		*/
/*   elimination, with partial pivoting (row interchange only). */
/*   The oefficient matrix is destroyed in this subroutine.     */
/*   The indices for the pivot rows are stored in the vector	*/
/*   perm[]; perm[i] is the index of the row to be used as the	*/
/*   ith row of the matrix.  Rows are not actually swapped.	*/
/*   If a pivot or diagonal element is<= TOLER, currently set	*/
/*   to 1.e-20, the matrix is considered singular or ill-	*/
/*   conditioned.					        */
/*                                                              */
/*  Returns: 0 if no error; 1 if matrix is singular or ill-	*/
/*			      conditioned			*/
/*                                                              */
/* Calling sequence: oldsimeq(n, coef, soln);               	*/
/*                                                              */
/* Arguments:                                                   */
/*     Input:   coef[n][n+1]	double	augmented matrix of     */
/*					coefficients		*/
/*              n		int	# of equations          */
/*    Output:   soln[n]	double	solution vector                 */
/*                                                              */
/*--------------------------------------------------------------*/


static int oldsimeq(int n, double** coef, double* soln) {
    int ipivot, isave = 0, jrow, kcol, i, j, *perm;

    /* Create and initialize permutation vector */

    perm = (int*) malloc((unsigned) (n * sizeof(int)));
    for (i = 0; i < n; i++)
        perm[i] = i;

    /*
     * The following loop is performed once for each row and implicitly
     * reduces the diagonal element to 1 and all elements below it to 0. To
     * save arithmetic operations, elements of the coefficient matrix which
     * will not later be used are not operated on (i.e., only values to the
     * right of the diagonal in a row are modified by the algorithm).
     */

    for (j = 0; j < n; j++) {
        /*
         * First, find pivot row (i.e., row of the lower triangle which, when
         * transposed into the jth position, will put the largest column
         * element on the diagonal.  Store pivot index in perm[].
         */

        ipivot = perm[j];
        for (i = j + 1; i < n; i++) {
            jrow = perm[i];
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

        for (kcol = j + 1; kcol <= n; kcol++)
            coef[ipivot][kcol] /= coef[ipivot][j];

        /* Row reduction */

        for (i = j + 1; i < n; i++) {
            jrow = perm[i];
            for (kcol = j + 1; kcol <= n; kcol++)
                coef[jrow][kcol] -= coef[ipivot][kcol] * coef[jrow][j];
        }
    } /* end loop over all rows */

    /* Back substitution */

    for (i = n - 1; i >= 0; i--) {
        jrow = perm[i];
        soln[i] = coef[jrow][n];
        for (j = i + 1; j < n; j++)
            soln[i] -= coef[jrow][j] * (soln[j]);
    }

    free(perm);
    return (SUCCESS);
}
