#include <../../nrnconf.h>
#include <newton_struct.h>
/******************************************************************************
 *
 * File: newton.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "newton.c,v 1.3 1999/01/04 12:46:48 hines Exp" ;
#endif

/*------------------------------------------------------------*/
/*                                                            */
/*  NEWTON                                        	      */
/*                                                            */
/*    Iteratively solves simultaneous nonlinear equations by  */
/*    Newton's method, using a Jacobian matrix computed by    */
/*    finite differences.				      */
/*                                                            */
/*  Returns: 0 if no error; 2 if matrix is singular or ill-   */
/*			     conditioned; 1 if maximum	      */
/*			     iterations exceeded	      */
/*                                                            */
/*  Calling sequence: newton(n, x, p, pfunc, value)	      */
/*                                                            */
/*  Arguments:                                                */
/*                                                            */
/*    Input: n, integer, number of variables to solve for.    */
/*                                                            */
/*           x, pointer to array  of the solution             */
/*		vector elements	possibly indexed by index     */
/*                                                            */
/*	     p,  array of parameter values		      */
/*                                                            */
/*           pfunc, pointer to function which computes the    */
/*               deviation from zero of each equation in the  */
/*               model.                                       */
/*                                                            */
/*	     value, pointer to array to array  of             */
/*		 the function values.			      */
/*                                                            */
/*    Output: x contains the solution value or the most       */
/*               recent iteration's result in the event of    */
/*               an error.                                    */
/*                                                            */
/*  Functions called: makevector, freevector, makematrix,     */
/*		      freematrix			      */
/*		      buildjacobian, crout, solve	      */
/*                                                            */
/*------------------------------------------------------------*/

#include <stdlib.h>
#include <math.h>
#include "errcodes.h"

int nrn_newton_thread(NewtonSpace* ns, int n, int* index, double* x,
 FUN pfunc, double* value, void* ppvar, void* thread, void* nt) {
    int i, count = 0, error, *perm;
    double **jacobian, *delta_x, change = 1.0, max_dev, temp;

    /*
     * Create arrays for Jacobian, variable increments, function values, and
     * permutation vector
     */
	delta_x = ns->delta_x;
	jacobian = ns->jacobian;
	perm = ns->perm;
    /* Iteration loop */

    while (count++ < MAXITERS)
    {
	if (change > MAXCHANGE)
	{
	    /*
	     * Recalculate Jacobian matrix if solution has changed by more
	     * than MAXCHANGE
	     */

	    nrn_buildjacobian_thread(ns, n, index, x, pfunc, value, jacobian, ppvar, thread, nt);
	    for (i = 0; i < n; i++)
		value[i] = - value[i];	/* Required correction to
				 		 * function values */

	    if ((error = nrn_crout_thread(ns, n, jacobian, perm)) != SUCCESS)
		break;
	}
	nrn_scopmath_solve_thread(n, jacobian, value, perm, delta_x, (int *)0);

	/* Update solution vector and compute norms of delta_x and value */

	change = 0.0;
 if (index) {
	for (i = 0; i < n; i++)
	{
	    if (fabs(x[index[i]]) > ZERO && (temp = fabs(delta_x[i] / (x[index[i]]))) > change)
		change = temp;
	    x[index[i]] += delta_x[i];
	}
 }else{
	for (i = 0; i < n; i++)
	{
	    if (fabs(x[i]) > ZERO && (temp = fabs(delta_x[i] / (x[i]))) > change)
		change = temp;
	    x[i] += delta_x[i];
	}
 }
	(*pfunc) (x, ppvar, thread, nt); /* Evaluate function values with new solution */
	max_dev = 0.0;
	for (i = 0; i < n; i++)
	{
	    value[i] = - value[i];	/* Required correction to function
					 * values */
	    if ((temp = fabs(value[i])) > max_dev)
		max_dev = temp;
	}

	/* Check for convergence or maximum iterations */

	if (change <= CONVERGE && max_dev <= ZERO)
	    break;
	if (count == MAXITERS)
	{
	    error = EXCEED_ITERS;
	    break;
	}
    }				/* end of while loop */

    return (error);
}

/*------------------------------------------------------------*/
/*                                                            */
/*  BUILDJACOBIAN                                 	      */
/*                                                            */
/*    Creates the Jacobian matrix by computing partial deriv- */
/*    atives by finite central differences.  If the column    */
/*    variable is nonzero, an increment of 2% of the variable */
/*    is used.  STEP is the minimum increment allowed; it is  */
/*    currently set to 1.0E-6.                                */
/*                                                            */
/*  Returns: no return variable                               */
/*                                                            */
/*  Calling sequence:					      */
/*	 buildjacobian(n, index, x, pfunc, value, jacobian)       */
/*                                                            */
/*  Arguments:                                                */
/*                                                            */
/*    Input: n, integer, number of variables                  */
/*                                                            */
/*           x, pointer to array of addresses of the solution */
/*		vector elements				      */
/*                                                            */
/*	     p, array of parameter values		      */
/*                                                            */
/*           pfunc, pointer to function which computes the    */
/*                     deviation from zero of each equation   */
/*                     in the model.                          */
/*                                                            */
/*	     value, pointer to array of addresses of function */
/*		       values				      */
/*                                                            */
/*    Output: jacobian, double, computed jacobian matrix      */
/*                                                            */
/*  Functions called:  user-supplied function with argument   */
/*                     (p) to compute vector of function      */
/*		       values for each equation.	      */
/*		       makevector(), freevector()	      */
/*                                                            */
/*------------------------------------------------------------*/

#define max(x, y) (fabs(x) > y ? x : y)

static void nrn_buildjacobian_thread(NewtonSpace* ns,
  int n, int* index, double* x, FUN pfunc,
  double* value, double** jacobian, void* ppvar, void* thread, void* nt) {
    int i, j;
    double increment, *high_value, *low_value;

	high_value = ns->high_value;
	low_value = ns->low_value;

    /* Compute partial derivatives by central finite differences */

 if (index) {
    for (j = 0; j < n; j++)
    {
	increment = max(fabs(0.02 * (x[index[j]])), STEP);
	x[index[j]] += increment;
	(*pfunc) (x, ppvar, thread, nt);
	for (i = 0; i < n; i++)
	    high_value[i] = value[i];
	x[index[j]] -= 2.0 * increment;
	(*pfunc) (x, ppvar, thread, nt);
	for (i = 0; i < n; i++)
	{
	    low_value[i] = value[i];

	    /* Insert partials into jth column of Jacobian matrix */

	    jacobian[i][j] = (high_value[i] - low_value[i]) / (2.0 * increment);
	}

	/* Restore original variable and function values. */

	x[index[j]] += increment;
	(*pfunc) (x, ppvar, thread, nt);
    }
 }else{
    for (j = 0; j < n; j++)
    {
	increment = max(fabs(0.02 * (x[j])), STEP);
	x[j] += increment;
	(*pfunc) (x, ppvar, thread, nt);
	for (i = 0; i < n; i++)
	    high_value[i] = value[i];
	x[j] -= 2.0 * increment;
	(*pfunc) (x, ppvar, thread, nt);
	for (i = 0; i < n; i++)
	{
	    low_value[i] = value[i];

	    /* Insert partials into jth column of Jacobian matrix */

	    jacobian[i][j] = (high_value[i] - low_value[i]) / (2.0 * increment);
	}

	/* Restore original variable and function values. */

	x[j] += increment;
	(*pfunc) (x, ppvar, thread, nt);
    }
 }
}

NewtonSpace* nrn_cons_newtonspace(int n) {
    NewtonSpace* ns = (NewtonSpace*)emalloc(sizeof(NewtonSpace));
    ns->n = n;
    ns->delta_x = makevector(n);
    ns->jacobian = makematrix(n, n);
    ns->perm = (int *) emalloc((unsigned) (n * sizeof(int)));
    ns->high_value = makevector(n);
    ns->low_value = makevector(n);
    ns->rowmax = makevector(n);
    return ns;
}

void nrn_destroy_newtonspace(NewtonSpace* ns) {
    free((char *) ns->perm);
    freevector(ns->delta_x);
    freematrix(ns->jacobian);
    freevector(ns->high_value);
    freevector(ns->low_value);
    freevector(ns->rowmax);
    free ((char*) ns);
}
