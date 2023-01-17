#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: simeq.c
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "errcodes.hpp"
#include "scoplib.h"

#include <cmath>
#include <cstdlib>

using namespace neuron::scopmath; // for errcodes.hpp
/****************************************************************/
/*                                                              */
/* Abstract: simeq();                                           */
/*                                                              */
/*   Solves simultaneous linear equations by Gaussian		*/
/*   elimination, with partial pivoting (row interchange only). */
/*   The oefficient matrix is destroyed in this subroutine.     */
/*   The indices for the pivot rows are stored in the vector	*/
/*   perm[]; perm[i] is the index of the row to be used as the	*/
/*   ith row of the matrix.  Rows are not actually swapped.	*/
/*   If a pivot or diagonal element is < ROUNDOFF, currently	*/
/*   set to 1.e-20, the matrix is considered singular or ill-	*/
/*   conditioned.					        */
/*                                                              */
/* Calling sequence: simeq(n, coef, soln);               	*/
/*                                                              */
/* Arguments:                                                   */
/*     Input:   coef[n][n+1]	double	augmented matrix of     */
/*					coefficients		*/
/*              n		int	# of equations          */
/*    Output:   *soln[n]	double	solution vector         */
/*                                                              */
/* Returns: 0 if no error; 2 if matrix is singular or ill-	*/
/*			      conditioned			*/
/*                                                              */
/* Functions called: none					*/
/*                                                              */
/* Files Accessed: none						*/
/*                                                              */
/****************************************************************/
int simeq(int n, double** coef, double* soln, int* index) {
	int ipivot, isave=0, jrow, kcol, i, j;
	static int np; static int* perm;

	/* Create and initialize permutation vector */
	if (np < n) {
		if (perm) {
			free(perm);
		}
		perm = (int *) malloc((unsigned) (n * sizeof(int)));
		np = n;
	}
	for (i = 0; i < n; i++) {
		perm[i] = i;
	}
    /*
     * The following loop is performed once for each row and implicitly
     * reduces the diagonal element to 1 and all elements below it to 0. To
     * save arithmetic operations, elements of the coefficient matrix which
     * will not later be used are not operated on (i.e., only values to the
     * right of the diagonal in a row are modified by the algorithm).
     */

    for (j = 0; j < n; j++)
    {

	/*
	 * First, find pivot row (i.e., row of the lower triangle which, when
	 * transposed into the jth position, will put the largest column
	 * element on the diagonal.  Store pivot index in perm[].
	 */

	ipivot = perm[j];
	for (i = j + 1; i < n; i++)
	{
	    jrow = perm[i];
	    if (fabs(coef[ipivot][j]) < fabs(coef[jrow][j]))
	    {
		ipivot = jrow;
		isave = i;
	    }
	}

	/* Now be sure the pivot found isn't too small  */

	if (fabs(coef[ipivot][j]) < ROUNDOFF)
	    return (SINGULAR);

	/* Swap row indices in perm[] if pivot is not perm[j] */

	if (ipivot != perm[j])
	{
	    perm[isave] = perm[j];
	    perm[j] = ipivot;
	}

	/* Row normalization */

	for (kcol = j + 1; kcol <= n; kcol++)
	    coef[ipivot][kcol] /= coef[ipivot][j];

	/* Row reduction */

	for (i = j + 1; i < n; i++)
	{
	    jrow = perm[i];
	    for (kcol = j + 1; kcol <= n; kcol++)
		coef[jrow][kcol] -= coef[ipivot][kcol] * coef[jrow][j];
	}
    }				/* end loop over all rows */

    /* Back substitution */

 if (index) {
    for (i = n - 1; i >= 0; i--)
    {
	jrow = perm[i];
	soln[index[i]] = coef[jrow][n];
	for (j = i + 1; j < n; j++)
	    soln[index[i]] -= coef[jrow][j] * (soln[index[j]]);
    }
 }else{
    for (i = n - 1; i >= 0; i--)
    {
	jrow = perm[i];
	soln[i] = coef[jrow][n];
	for (j = i + 1; j < n; j++)
	    soln[i] -= coef[jrow][j] * (soln[j]);
    }
 }
    return (SUCCESS);
}
