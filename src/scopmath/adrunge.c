#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: adrunge.c
 *
 * Copyright (c) 1989-1991
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
"adrunge.c,v 1.3 1999/01/04 12:46:41 hines Exp";
#endif

/*--------------------------------------------------------------
 *
 * Abstract: adrunge
 *
 *   Solves simultaneous differential equations using a fourth
 *   order Runge-Kutta algorithm with time step adjustment.
 *
 * Returns: int error code
 *
 * Calling sequence: adrunge(n, y, d, p, t, dt, dy, work, maxerror)
 *
 * Arguments
 *   Input:	n, int, number of differential equations
 *
 * 		y, pointer to array of addresses of the state
 *		   variables
 *
 * 		d, pointer to array of addresses of the derivatives
 *		   of the state variables
 *
 * 		p, double precision variable array
 *
 * 		t, pointer to current time
 *
 * 		dt, double, integration interval
 *
 * 		dy, name of function which evaluates the time
 * 		    derivatives of the dependent variables
 *
 * 		*work, pointer to double precision work array
 *
 * 		maxerror, double, maximum single-step truncation
 * 			  error
 *
 * Output:	p[], double precision array containing the
 *		     values of the dependent variables and their
 *		     derivatives at time + h.
 *
 * Functions called: (*dy), which is derfunc(p) in SCoP, runge()
 *
 *--------------------------------------------------------------*/

#include <math.h>
#include "errcodes.h"
#include "scoplib.h"

int adrunge(_ninits, n, y, d, p, t, dt, dy, work, maxerror)
int _ninits, n, (*dy) ();
double p[], *t, dt, **work, maxerror; int y[]; int d[];
#define d_(arg)  p[d[arg]]
#define y_(arg)  p[y[arg]]
{
    int i;
    static int initialized = 0, steps;
    double T_err, temp, end_t;
    static double h, *ystore;
	extern double *makevector();

    if (ystore == (double *) 0)
	ystore = makevector(n << 1);
    if (initialized < _ninits)
    {
	steps = -1;
	if (dt < 0.1)
	    h = dt / 10.;
	else
	    h = 0.01;
	initialized = _ninits;
    }
    end_t = *t + dt;

    for (; *t < end_t; *t += h)
    {
	/* Recalculate stepsize every ten integration steps */
	if (++steps % 10 == 0)
	{
	    /*
	     * Store original values of state variables and take a double
	     * time step.  Make sure that end_t is not exceeded.
	     */
	    for (i = 0; i < n; i++)
		ystore[i] = y_(i);
	    if (*t + 2.*h > end_t)
		h = (end_t - *t) / 2.0;
	    runge(_ninits, n, y, d, p, t, 2.0 * h, dy, work);

	    /* Restore original state variable values */
	    for (i = 0; i < n; i++)
	    {
		ystore[i + n] = y_(i);
		y_(i) = ystore[i];
	    }

	    /* Take two single time steps */

	    runge(_ninits, n, y, d, p, t, h, dy, work);
	    *t += h;
	    runge(_ninits, n, y, d, p, t, h, dy, work);
	    *t += h;
	    steps++;

	    /*
	     * Calculate truncation error T_err and find optimal time step h
	     */
	    T_err = 0.0;
	    for (i = 0; i < n; i++)
		if ((temp = fabs(y_(i) - ystore[i + n]) / 30.) > T_err)
		    T_err = temp;
	    /* Guard against overflows due to roundoff error */
	    if (T_err < ROUNDOFF)
		T_err = 0.1 * maxerror;
	    h = pow(maxerror * pow(h, 5.) / T_err, 0.25);
	    if (h < ROUNDOFF)
		return (PRECISION);
	}

	/*
	 * Generic integration step.  Note that no step is taken if the step
	 * size control algorithm has reached end_t.
	 */
	if (*t < end_t && *t + h > end_t)
	    runge(_ninits, n, y, d, p, t, end_t - *t, dy, work);
	else if (*t + h <= end_t)
	    runge(_ninits, n, y, d, p, t, h, dy, work);
    }

    /* Restore original value of time -- will be updated in model() */

    *t = end_t - dt;

    return (SUCCESS);
}
