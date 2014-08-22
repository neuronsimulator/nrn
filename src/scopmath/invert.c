#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: invert.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "invert.c,v 1.3 1999/01/04 12:46:47 hines Exp" ;
#endif

/****************************************************************/
/*                                                              */
/* Abstract: invert();                                          */
/*                                                              */
/*   Inverts a matrix by Gaussian elimination with partial	*/
/*   pivoting (row interchange only).  The zeroth row and	*/
/*   column elements of the matrix are not used.  The routine	*/
/*   replaces the matrix by its inverse.  The indices for the	*/
/*   pivot rows are stored in the vector perm[]; perm[i] is the	*/
/*   index of the row to be used as the ith row of the matrix.	*/
/*   If the magnitude of a pivot element < ROUNDOFF, currently	*/
/*   set to 1.e-20, the matrix is considered singular or ill-	*/
/*   conditioned and the algorithm terminates.			*/
/*                                                              */
/*  Returns: 0 if no error; 2 if matrix is singular or ill-	*/
/*			      conditioned			*/
/*                                                              */
/* Calling sequence: invert(n, matrix);				*/
/*                                                              */
/* Arguments:                                                   */
/*     Input:   matrix[n][n], double, matrix to be inverted	*/
/*              n, int, # of equations                          */
/*    Output:   matrix[n][n], double, inverse matrix            */
/*                                                              */
/* Functions called: makematrix(), freematrix()			*/
/*                                                              */
/* Files Accessed: none						*/
/*                                                              */
/****************************************************************/

#include <math.h>
#include <stdlib.h>
#include "errcodes.h"

int invert(n, matrix)
int n;
double *matrix[];
{
    extern int freematrix();
    extern double **makematrix();
    int ipivot, isave, jrow, krow, kcol, i, j, *perm;
    double **soln;

    /* Create and initialize solution matrix and permutation vector */

    soln = makematrix(n, n);
    perm = (int *) malloc((unsigned) (n * sizeof(int)));

    for (i = 0; i < n; i++)
    {
	perm[i] = i;
	for (j = i; j < n; j++)
	    if (i == j)
		soln[i][j] = 1.0;
	    else
		soln[i][j] = soln[j][i] = 0.0;
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
	    if (fabs(matrix[ipivot][j]) < fabs(matrix[jrow][j]))
	    {
		ipivot = jrow;
		isave = i;
	    }
	}

	/* Now be sure the pivot found isn't too small  */

	if (fabs(matrix[ipivot][j]) < ROUNDOFF)
	    return (SINGULAR);

	/* Swap row indices in perm[] if pivot is not perm[j] */

	if (ipivot != perm[j])
	{
	    perm[isave] = perm[j];
	    perm[j] = ipivot;
	}

	/* Row normalization */

	for (kcol = j + 1; kcol < n; kcol++)
	    matrix[ipivot][kcol] /= matrix[ipivot][j];
	for (kcol = 0; kcol < n; kcol++)
	    soln[ipivot][kcol] /= matrix[ipivot][j];

	/* Row reduction */

	for (i = j + 1; i < n; i++)
	{
	    jrow = perm[i];
	    for (kcol = j + 1; kcol < n; kcol++)
		matrix[jrow][kcol] -= matrix[ipivot][kcol] * matrix[jrow][j];
	    for (kcol = 0; kcol < n; kcol++)
		soln[jrow][kcol] -= soln[ipivot][kcol] * matrix[jrow][j];
	}
    }				/* end loop over all rows */

    /* Back substitution */

    for (i = n - 1; i >= 0; i--)
    {
	jrow = perm[i];
	for (kcol = 0; kcol < n; kcol++)
	    for (j = i + 1; j < n; j++)
	    {
		krow = perm[j];
		soln[jrow][kcol] -= matrix[jrow][j] * soln[krow][kcol];
	    }
    }

    /* Copy solution into original matrix */

    for (i = 0; i < n; i++)
    {
	jrow = perm[i];
	for (j = 0; j < n; j++)
	    matrix[i][j] = soln[jrow][j];
    }

    free((char *) perm);
    freematrix(soln);
    return (SUCCESS);
}
