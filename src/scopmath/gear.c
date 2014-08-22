#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: gear.c
 *
 * Copyright (c) 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
"gear.c,v 1.4 1999/01/04 12:46:47 hines Exp";
#endif

/*--------------------------------------------------------------
 *
 * Abstract: gear
 *
 *   Solves simultaneous differential equations using the stiffly
 *   stable gear algorithm with time step adjustment.  For an
 *   explanation of the algorithm see L. O. Chua & P.-M. Lin,
 *   "Computer Aided Analysis of Electronic Circuits," Prentice-
 *   Hall, 1975, pp. 496--506, 524--534.
 *
 * Returns: int error code
 *
 * Calling sequence: gear(n, y, d, p, t, dt, dy, work, maxerror)
 *
 * Arguments
 *   Input:     n, int, number of differential equations
 *
 *              y, pointer to array of addresses of the state
 *                 variables
 *
 *              d, pointer to array of addresses of the derivatives
 *                 of the state variables
 *
 *              p, double precision variable array
 *
 *              t, pointer to current time
 *
 *              dt, double, integration interval
 *
 *              dy, name of function which evaluates the time
 *                  derivatives of the dependent variables
 *
 *              *work, pointer to double precision work array
 *
 *              maxerror, double, maximum single-step truncation
 *				  error
 *
 * Output:      *y[], *d[], p[], double precision arrays containing
 *                  the values of the dependent variables and their
 *                  derivatives at t + dt.
 *
 *
 * Functions called: (*dy)
 *
 *--------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "errcodes.h"

#if MAC
#define h mlhh
#endif

static int good_order, good_steps, jac_iters, order, *perm;
int error_code;
static double **Nord, **jacobian, h, *diff, *corr_fn, b[6] = {1., 2. / 3., 6. / 11., 12. / 25., 60. / 137., 60. / 147.},

    coeff[6][7] = {
    {1., 1., 0., 0., 0., 0., 0.},
    {2. / 3., 1., 1. / 3., 0., 0., 0., 0.},
    {6. / 11., 1., 6. / 11., 1. / 11., 0., 0., 0.},
    {24. / 50., 1., 35. / 50., 10. / 50., 1. / 50., 0., 0.},
    {120. / 274., 1., 225. / 274., 85. / 274., 15. / 274., 1. / 274., 0.},
    {720. / 1764., 1., 1624. / 1764., 735. / 1764., 175. / 1764., 21. / 1764., 1. / 1764.},
},
    err_coeff[6] = {0.5, 4. / 9., 18. / 22., 288. / 125., 1200. / 137., 43200. / 1029.};

int gear(_ninits, n, y, d, p, t, dt, dy, work, maxerror)
int _ninits, n, (*dy) ();
double p[], *t, dt, **work, maxerror; int y[]; int d[];
#define d_(arg)  p[d[arg]]
#define y_(arg)  p[y[arg]]
{
    static int initialized = 0;
    int i, init_gear(), prep_jac(), predictor(), corrector(), interpolate(), change_order(),
        change_h();
    double told, tend;

    told = *t;			/* Store original value of independent
				 * variable */
    tend = *t + dt;		/* Calculate next breakpoint value */

    if (initialized != _ninits)
    {
	if ((error_code = init_gear(n, work, y, dy, p, d, tend, maxerror)) != SUCCESS)
	    return (error_code);
	initialized = _ninits;
    }

    while (*t < tend && error_code == SUCCESS)
	if (*t + h >= tend)
	{
	    interpolate(n, p, y, *t, tend);
	    *t = tend;
	}
	else
	{			/* Take generic step */
	    *t += h;
	    predictor(n, *work);
	    if ((error_code = corrector(n, y, d, p, dy, t, tend, maxerror, *work)) == SUCCESS)
	    {
		/* Update state variables */
		for (i = 0; i < n; i++)
		    y_(i) = Nord[i][0];

		/*
		 * Increase order, calculate optimal integration step size,
		 * and/or re-evaluate jacobian if appropriate number of
		 * successful steps have been taken.
		 */
		if (++good_order >= MAX_GOOD_ORDER && order < MAXORDER)
		{
		    change_order(1, n, *work);
		    if (++good_steps >= order)
			error_code = change_h(1, n, maxerror, *work);
		}
		else if (++good_steps >= MAX_GOOD_STEPS)
		    error_code = change_h(0, n, maxerror, *work);
		if (error_code == SUCCESS)
		    if (++jac_iters >= MAX_JAC_ITERS)
			error_code = prep_jac(n, y, d, p, dy);
	    }			/* end of successful corrector section */
	}			/* end of integration step calculation */

    /* Restore original value of t; it is updated in model() */
    *t = told;

    return (error_code);
}

/*--------------------------------------------------------------
 *
 * init_gear: initialize the gear algorithm when first starting
 * a simulation run, when the corrector iteration diverged, or
 * when a RESET command was issued.
 *
 *--------------------------------------------------------------*/

int init_gear(n, work, y, dy, p, d, tend, maxerror)
int n, (*dy) ();
double **work, p[], tend, maxerror; int y[]; int d[];
{
    extern double **makematrix(), *makevector();
    int i, prep_jac();
    double temp, max_y, max_der;

    if (*work == NULL)
    {
	/*
	 * Required workspace is 7*n to store the n 7-dimensional Nordsieck
	 * vectors prior to the current step and 2*n to store the backward
	 * differences used in calculating the optimal step size.
	 * 
	 * The Nord[n][7] matrix is packed into work by rows, followed by the n
	 * backward differences at the start start of the integration step,
	 * followed by the n backward differences at the end of the
	 * integration step.
	 */
	*work = makevector(9 * n);

	/* Allocate arrays for gear algorithm. */
	Nord = makematrix(n, 7);
	jacobian = makematrix(n, n);
	diff = makevector(n);
	corr_fn = makevector(n);
	if ((perm = (int *) calloc((unsigned) n, (unsigned) sizeof(int))) == NULL)
	    error_code = LOWMEM;
	if (error_code != SUCCESS)
	    return (error_code);
    }

    (*dy) (p);			/* Calculate derivatives d_(i) */

    /* Calculate initial step size */
    max_y = max_der = 0.0;
    for (i = 0; i < n; i++)
    {
	if ((temp = fabs(y_(i))) > max_y)
	    max_y = temp;
	if ((temp = fabs(d_(i))) > max_der)
	    max_der = temp;
    }
    h = maxerror / ((temp = max_y / tend) > max_der ? temp : max_der);
    if (h <= ROUNDOFF)
	h = maxerror * tend;

    /* Initialize Nordsieck vectors and backwards differences */
    for (i = 0; i < n; i++)
    {
	Nord[i][0] = y_(i);
	Nord[i][1] = d_(i) * h;
	(*work)[8 * n + i] = 0.0;
    }

    order = 1;
    good_steps = 0;
    good_order = 0;

    /* Calculate jacobian and triangularize */
    error_code = prep_jac(n, y, d, p, dy);

    return (error_code);
}

/*--------------------------------------------------------------
 *
 * prep_jac: calculate jacobian matrix, transform it into the
 * matrix I - b*h*J, and triangularize.
 *
 *--------------------------------------------------------------*/

int prep_jac(n, y, d, p, dy)
int n, (*dy) ();
double p[]; int y[]; int d[];
{
    extern int buildjacobian(), crout();
    int i, j;
    double a;

    buildjacobian(n, y, p, dy, d, jacobian);
    a = -b[order - 1] * h;
    for (i = 0; i < n; i++)
    {
	for (j = 0; j < n; j++)
	    jacobian[i][j] *= a;
	jacobian[i][i] += 1.0;
    }
    error_code = crout(n, jacobian, perm);

    jac_iters = 0;

    return (error_code);
}

/*--------------------------------------------------------------
 *
 * predictor: project Nordsieck vectors to t + h by multiplying
 * by the Pascal triangle.
 *
 *--------------------------------------------------------------*/

int predictor(n, work)
int n;
double *work;
{
    int i, j, k;

    /* Store current Nordsieck vectors and backwards differences */
    for (i = 0; i < n; i++)
    {
	work[7 * n + i] = work[8 * n + i];
	for (j = 0; j <= order; j++)
	    work[7 * i + j] = Nord[i][j];
    }

    /*
     * Multiply Nordsieck vectors by Pascal triangle to extrapolate to next
     * step's values. Note that highest order term is unchanged.
     */
    for (i = 0; i < n; i++)
	for (j = 1; j <= order; j++)
	    for (k = order - 1; k >= j - 1; k--)
		Nord[i][k] += Nord[i][k + 1];
    return 0;
}

/*--------------------------------------------------------------
 *
 * corrector: solve for corrector function vector by Gaussian
 * elimination until test for convergence is satisfied.
 *
 *--------------------------------------------------------------*/

int corrector(n, y, d, p, dy, t, tend, maxerror, work)
int n, (*dy) ();
double p[], *t, tend, maxerror, *work; int y[]; int d[];
{
    int i, j, terms, failures = 0, conv = NO_SOLN, conv_test(), retry_step(),
        init_gear();
    double temp, norm, trunc_err;
    extern int solve();

    while (conv == NO_SOLN)
    {
	terms = 0;
	do
	{
	    /* Update state variables and calculate derivatives */
	    for (i = 0; i < n; i++)
		y_(i) = Nord[i][0];
	    (*dy) (p);

	    /*
	     * Calculate difference vector, and solve the linear equation
	     * jacobian * corr_fn = diff for the corrector function vector.
	     */
	    for (i = 0; i < n; i++)
		diff[i] = d_(i) * h - Nord[i][1];
	    solve(n, jacobian, diff, perm, corr_fn, (int *)0);

	    /* Accumulate corrector terms */
	    for (i = 0; i < n; i++)
		for (j = 0; j <= order; j++)
		    Nord[i][j] += corr_fn[i] * coeff[order - 1][j];
	} while ((conv = conv_test(n, ++terms, maxerror)) == NO_SOLN);

	switch (conv)
	{
	    case EXCEED_ITERS:
		/*
		 * Corrector failed to converge in maximum iterations; reduce
		 * order and step size and try again
		 */
		if (++failures < MAXFAIL)
		{
		    if ((error_code = retry_step(n, t, y, d, p, dy, maxerror, work)) == SUCCESS)
			conv = NO_SOLN;
		}
		else
		    error_code = EXCEED_ITERS;
		break;

	    case DIVERGED:
		/*
		 * Corrector diverged; reduce order and step size and try
		 * again
		 */
		if (++failures < MAXFAIL)
		{
		    if ((error_code = retry_step(n, t, y, d, p, dy, maxerror, work)) == SUCCESS)
			conv = NO_SOLN;
		}
		else
		    error_code = DIVERGED;
		break;

	    case SUCCESS:
		/* Calculate local truncation error */
		for (norm = 0.0, i = 0; i < n; i++)
		{
		    /*
		     * Calculate backwards differences of highest order
		     * elements of Nordsieck vectors
		     */
		    temp = work[8 * n + i] = Nord[i][order] - work[7 * i + order];
		    norm += temp * temp;
		}
		norm = sqrt(norm / (double) n);
		trunc_err = err_coeff[order - 1] * norm;

		/* Reduce order to 1 if truncation error exceeds maxerror */
		if (trunc_err > maxerror)
		{
		    if (++failures < MAXFAIL)
		    {
			/*
			 * Retrieve values of state variables at start of
			 * step
			 */
			for (i = 0; i < n; i++)
			    y_(i) = work[7 * i];
			if ((error_code = init_gear(n, &work, y, dy, p, d, tend, maxerror)) == SUCCESS)
			{
			    predictor(n, work);
			    conv = NO_SOLN;
			}
		    }
		    else
			error_code = CORR_FAIL;
		}
	}			/* end of convergence test */
    }				/* end of corrector loop */

    return (error_code);
}

/*--------------------------------------------------------------
 *
 * conv_test: test for convergence of corrector iteration and
 * return status code.
 *
 *--------------------------------------------------------------*/

int conv_test(n, iter, maxerror)
int n, iter;
double maxerror;
{
    int i;
    double temp, norm, pred_trunc_err;
    static double old_norm, conv_rate;

    /* Calculate norm of corr_fn and convergence rate */
    if (iter == 1)
    {
	conv_rate = 0.7;
	old_norm = 1.0;
    }
    for (norm = 0.0, i = 0; i < n; i++)
    {
	temp = corr_fn[i];
	norm += temp * temp;
    }
    norm = sqrt(norm / (double) n);
    conv_rate = (conv_rate *= 0.2) < (temp = norm / old_norm) ? temp : conv_rate;
    if (iter == 1)
    {
	old_norm = norm;
	return (NO_SOLN);
    }

    /*
     * Compute predicted value of the local truncation error on the next
     * iteration of the corrector and return status code
     */
    pred_trunc_err = (temp = 1.5 * conv_rate) > 1.0 ? 1.0 : temp;
    pred_trunc_err *= 2.0 * norm * err_coeff[order - 1];
    if (pred_trunc_err <= maxerror)	/* Convergence achieved */
	return (SUCCESS);
    else if (norm > 2.0 * old_norm)	/* Corrector iteration diverged */
	return (DIVERGED);
    else if (iter >= MAXTERMS)	/* Exceeded maximum number of corrector
				 * iterations */
	return (EXCEED_ITERS);

    /* Solution not yet obtained; continue corrector iteration */

    old_norm = norm;		/* Save current norm for next iteration */

    return (NO_SOLN);
}

/*--------------------------------------------------------------
 *
 * interpolate: find state variables at next breakpoint by
 * interpolation of polynomial approximation given by the
 * power series in the Nordsieck vector.
 *
 *--------------------------------------------------------------*/

int interpolate(n, p, y, t, tend)
int n;
double *p, t, tend; int y[];
{
    int i, j;
    double s;

    s = (tend - t) / h;
    for (i = 0; i < n; i++)
    {
	y_(i) = 0.0;
	for (j = order; j >= 0; j--)
	    y_(i) = Nord[i][j] + s * (y_(i));
    }
    return 0;
}

/*--------------------------------------------------------------
 *
 * retry_step: retreat to the last prediction, recompute the
 * jacobian, change order if necessary, and compute step size.
 *
 *--------------------------------------------------------------*/

int retry_step(n, t, y, d, p, dy, maxerror, work)
int n, (*dy) ();
double *t, p[], maxerror, *work; int y[]; int d[];
{
    int i, j, incr = 0, change_order(), change_h(), prep_jac(), predictor();

    /*
     * Restore original values of state variables, Nordsieck vectors, and
     * backwards differences
     */
    for (i = 0; i < n; i++)
    {
	y_(i) = work[7 * i];
	work[8 * n + i] = work[7 * n + i];
	for (j = 0; j <= order; j++)
	    Nord[i][j] = work[7 * i + j];
    }

    /* Reduce integration order and compute new step size */
    if (order > 1)
    {
	change_order(-1, n, work);
	incr = -1;
    }
    *t -= h;
    if ((error_code = change_h(incr, n, maxerror, work)) == SUCCESS)
    {
	*t += h;

	/* Update jacobian matrix */
	if (jac_iters > 0)
	    error_code = prep_jac(n, y, d, p, dy);
    }

    if (error_code == SUCCESS)
	predictor(n, work);

    return (error_code);
}

/*--------------------------------------------------------------
 *
 * change_order: increase or decrease integration order by one;
 * calculate next highest order terms of Nordsieck vectors if
 * if increasing order
 *
 *--------------------------------------------------------------*/

int change_order(incr, n, work)
int incr, n;
double *work;
{
    int i;

    switch (incr)
    {
	case 1:		/* Increase order by one */
	    /* Calculate next higher order elements of Nordsieck vectors */
	    order++;
	    for (i = 0; i < n; i++)
		Nord[i][order] = work[8 * n + i] / (double) (order);
	    break;

	case -1:		/* Decrease order by one */
	    order--;
	    break;
    }
    good_order = 0;
    return 0;
}

/*--------------------------------------------------------------
 *
 * change_h: calculate optimal integration step size for current
 * order of gear integrator
 *
 *--------------------------------------------------------------*/

int change_h(incr, n, maxerror, work)
int incr, n;
double maxerror, *work;
{
    int i, j, old_order;
    double temp, norm = 0.0, alpha, r = 1.0;

    switch (incr)
    {
	case 0:		/* Order not changed */
	    /* Calculate norm of 1st backward differences */
	    for (i = 0; i < n; i++)
	    {
		temp = work[8 * n + i];
		norm += temp * temp;
	    }
	    norm = sqrt(norm / (double) n);
	    alpha = maxerror / (err_coeff[order - 1] * norm);
	    alpha = pow(alpha, 1. / (double) (order + 1)) / 1.2;
	    break;

	case -1:		/* Order was reduced */
	    old_order = order + 1;

	    /* Compute norm of highest order terms of Nordsieck vectors */
	    for (i = 0; i < n; i++)
	    {
		temp = Nord[i][old_order];
		norm += temp * temp;
	    }
	    norm = sqrt(norm / (double) n);
	    alpha = maxerror / ((double) old_order * err_coeff[old_order - 2] * norm);
	    alpha = pow(alpha, 1. / (double) old_order) / 1.3;
	    break;

	case 1:		/* Order was increased */
	    old_order = order - 1;

	    /* Calculate norm of 2nd backward differences */
	    for (i = 0; i < n; i++)
	    {
		temp = work[8 * n + i] - work[7 * n + i];
		norm += temp * temp;
	    }
	    norm = sqrt(norm / (double) n);
	    alpha = (double) order *maxerror / (err_coeff[old_order] * norm);
	    alpha = pow(alpha, 1. / (double) (order + 1)) / 1.4;
    }

    if ((h *= alpha) <= ROUNDOFF)
	return (PRECISION);

    /* Update Nordsieck vectors */
    for (j = 1; j <= order; j++)
    {
	r *= alpha;
	for (i = 0; i < n; i++)
	    Nord[i][j] *= r;
    }

    good_steps = 0;
    return (SUCCESS);
}
