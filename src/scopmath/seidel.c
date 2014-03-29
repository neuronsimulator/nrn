#include <../../nrnconf.h>

/******************************************************************************
 *
 * File: seidel.c
 *
 * Copyright (c) 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "seidel.c,v 1.3 1999/01/04 12:46:50 hines Exp" ;
#endif
/****************************************************************/
/*                                                              */
/*   seidel()                                                   */
/*                                                              */
/*   Solves simultaneous linear equations by the Gauss-Seidel	*/
/*   method with successive over-relaxation (w = 1.5).  The	*/
/*   coef array elements are transformed by this routine.  If	*/
/*   the maximal fractional change in the solution in succes-	*/
/*   sive iterations is within CONVERGE (currently 1.e-6), the	*/
/*   iterations are terminated.  MAXITER is the maximal number	*/
/*   (currently set to 50) of iterations before termination;	*/
/*   'TOLER' is the limit for considering diagonal elements as	*/
/*   arbitrarily close to zero (currently set to 1.e-20), in	*/
/*   which case the error flag (*perr) is set to 1 and control	*/
/*   returns to the calling routine.  This function skips over	*/
/*   matrix elements that equal zero to save computation time	*/
/*   for sparse matrices.					*/
/*                                                              */
/* Calling sequence: seidel(n, coef, soln)			*/
/*                                                              */
/* Arguments:                                                   */
/*     Input:   coef[n][n+1], double, augmented matrix of       */
/*                      coefficients, maximum dimensions        */
/*                      currently 50 x 51, controlled by      	*/
/*                      '#define MAXCOLS 52'                  	*/
/*                      currently 201 x 202, controlled by      */
/*                      '#define MAXCOLS 203'                   */
/*		soln[n], double, initial estimate of solution	*/
/*              n, int, # of equations                          */
/*    Output:   coef[n][n+1], double, transformed matrix of	*/
/*			coefficients				*/
/*		soln[n], double, solution vector               */
/*                                                              */
/*  Returns: 0 if no error;                                     */
/*           1 if no convergence in MAXITERS iterations		*/
/*	     2 if matrix is singular or ill-conditioned		*/
/*                                                              */
/*                                                              */
/* Functions called: makevector(), freevector()			*/
/*                                                              */
/* Files Accessed: none						*/
/*                                                              */
/****************************************************************/

#include <stdlib.h>
#include <math.h>
#include "errcodes.h"
#define w 1.5

int seidel(n, coef, soln, index)
int n,*index;
double *coef[], soln[];
{
    extern int freevector();
    extern double *makevector();
    int i, j, iter = 0, *buff;
    double change, maxchange, *last_value;
    struct rowstruct
    {
	int length;		/* number of nonzero elements in row */
	int *colindex;		/* column indices of nonzero elements */
    }
       *rowptr;

    last_value = makevector(n);
    buff = (int *) malloc((unsigned) (n * sizeof(int)));
    rowptr = (struct rowstruct *)
	malloc((unsigned) (n * sizeof(struct rowstruct)));

    /* Transform coefficient matrix and constant vector */

    for (i = 0; i < n; i++)
    {
	if (fabs(coef[i][i]) < ROUNDOFF)	/* Check for ill-conditioning */
	    return (SINGULAR);

	coef[i][n] *= w / coef[i][i];	/* Constant vector */
	rowptr[i].length = 0;
	for (j = 0; j < n; j++)
	    if (j != i && fabs(coef[i][j]) > ROUNDOFF)
		/*
		 * Set up arrays of indices to nonzero off-diagonal matrix
		 * elements
		 */
	    {
		buff[rowptr[i].length] = j;
		rowptr[i].length++;
		coef[i][j] *= -w / coef[i][i];
	    }
	rowptr[i].colindex =
	    (int *) malloc((unsigned) (rowptr[i].length * sizeof(int)));
	for (j = 0; j < rowptr[i].length; j++)
	    rowptr[i].colindex[j] = buff[j];
    }

 if (index) { int k;
    do				/* Iterative refinement loop */
    {
	for (i = 0; i < n; i++)
	{
	    k = index[i];
	    last_value[i] = soln[k];
	    soln[k] = coef[i][n] + (1. - w) * (soln[k]);
	    for (j = 0; j < rowptr[i].length; j++)
		/* Loop only over nonzero elements */
		soln[k] += coef[i][*(rowptr[i].colindex + j)] * (soln[index[j]]);
	}
	iter++;
	/*
	 * Calculate the maximal relative change in the solution and check
	 * for convergence.
	 */
	maxchange = fabs(*last_value - soln[index[0]]);
	if (fabs(*last_value) > ROUNDOFF)
	    maxchange /= fabs(*last_value);
	for (i = 1; i < n; i++)
	{
	    change = fabs(last_value[i] - soln[index[i]]);
	    if (fabs(last_value[i]) > ROUNDOFF)
		change /= fabs(last_value[i]);
	    if (change > maxchange)
		maxchange = change;
	}
    }
    while (maxchange > CONVERGE && iter < MAXITERS);
 }else{
    do				/* Iterative refinement loop */
    {
	for (i = 0; i < n; i++)
	{
	    last_value[i] = soln[i];
	    soln[i] = coef[i][n] + (1. - w) * (soln[i]);
	    for (j = 0; j < rowptr[i].length; j++)
		/* Loop only over nonzero elements */
		soln[i] += coef[i][*(rowptr[i].colindex + j)] * (soln[j]);
	}
	iter++;
	/*
	 * Calculate the maximal relative change in the solution and check
	 * for convergence.
	 */
	maxchange = fabs(*last_value - *soln);
	if (fabs(*last_value) > ROUNDOFF)
	    maxchange /= fabs(*last_value);
	for (i = 1; i < n; i++)
	{
	    change = fabs(last_value[i] - soln[i]);
	    if (fabs(last_value[i]) > ROUNDOFF)
		change /= fabs(last_value[i]);
	    if (change > maxchange)
		maxchange = change;
	}
    }
    while (maxchange > CONVERGE && iter < MAXITERS);
 }
    freevector(last_value);
    for (i = 0; i < n; i++)
	free((char *) rowptr[i].colindex);
    free((char *) rowptr);
    free((char *) buff);

    if (iter >= MAXITERS)
	return (EXCEED_ITERS);
    else
	return (SUCCESS);
}
