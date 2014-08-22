#include <../../nrnconf.h>

/******************************************************************************
 *
 * File: runge.c
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "runge.c,v 1.1.1.1 1994/10/12 17:22:23 hines Exp" ;
#endif

/****************************************************************/
/*                                                              */
/*  Abstract: runge	                                        */
/*                                                              */
/*    Solves simultaneous differential equations using a fourth */
/*    order Runge-Kutta algorithm.  The independent variable is */
/*    assumed to be time.  The dependent variables are indexed  */
/*    from 0 to n - 1; n = no. of differential equations.  The  */
/*    state variables and their derivatives must each be in a   */
/*    contiguous block in the variable array and quantities     */
/*    refering to the same variable must appear in the same     */
/*    order in each block.                                      */
/*                                                              */
/*  Returns: int error code (aways SUCCESS for runge)		*/
/*                                                              */
/*  Calling sequence:						*/
/*	runge(n, y, d, p, t, h, dy, work)		   	*/
/*                                                              */
/*  Arguments                                                   */
/*    Input:    n, int, number of differential equations        */
/*                                                              */
/*              y, pointer to array of addresses of the		*/
/*                       state variables                        */
/*                                                              */
/*              d, pointer to array of addresses of the		*/
/*                      derivatives of the state variables      */
/*                                                              */
/*              p, double precision variable array              */
/*                                                              */
/*              t, pointer to current time                      */
/*                                                              */
/*              h, double, integration step size = delta_t      */
/*                                                              */
/*              dy, name of function which evaluates the time   */
/* 	             derivatives of the dependent variables	*/
/*                                                              */
/*              *work, pointer to double precision work array	*/
/*                                                              */
/*    Output:   p[], double precision array containing the      */
/*              values of the dependent variables and their     */
/*              derivatives at time + h.  Time is also updated. */
/*                                                              */
/*  Functions called: (*dy), which is derfunc(p) in SCoP        */
/*                                                              */
/*  Files accesses: none                                        */
/*                                                              */
/****************************************************************/

#include "errcodes.h"

int runge(_ninits, n, y, d, p, t, h, dy, work)
int _ninits, n, (*dy) ();
double p[], *t, h, **work; int y[]; int d[];
#define d_(arg)  p[d[arg]]
#define y_(arg)  p[y[arg]]
{
    int i;
    double temp;
    extern double *makevector();

    if (*work == (double *) 0)
	*work = makevector(n << 1);

    /* Get derivatives at current time point */

    (*dy) (p);

    for (i = 0; i < n; i++)
    {
	/* Store values of state variables at current time */

	(*work)[i] = y_(i);

	/* Eulerian extrapolation for half a time step */

	temp = (h / 2.0) * (d_(i));
	y_(i) += temp;

	/* Update solution */

	(*work)[n + i] = (*work)[i] + temp / 3.0;
    }

    /* Get derivatives at extrapolated values of variables at t + h/2 */

    *t += h / 2.0;
    (*dy) (p);

    for (i = 0; i < n; i++)
    {
	/* Backwards Eulerian extrapolation for half a time step */

	temp = h * (d_(i));
	y_(i) = (*work)[i] + temp / 2.0;

	/* Update solution */

	(*work)[n + i] += temp / 3.0;
    }

    /* Get derivatives at new extrapolated values of variables at t + h/2 */

    (*dy) (p);

    for (i = 0; i < n; i++)
    {
	/* Extrapolation of variables for full time step, using midpoint rule */

	temp = h * (d_(i));
	y_(i) = (*work)[i] + temp;

	/* Update solution */

	(*work)[n + i] += temp / 3.0;
    }

    /* Get derivatives at extrapolated values of variables at t + h */

    *t += h / 2.0;
    (*dy) (p);

    for (i = 0; i < n; i++)

	/* Compute the values of the dependent variables at t + h */

	y_(i) = (*work)[n + i] + (h / 6.0) * (d_(i));

/* Restore t to original value -- it will be updated in model() */

    *t -= h;

    return (SUCCESS);
}
