#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: simplex.c
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
"simplex.c,v 1.3 1999/01/04 12:46:52 hines Exp";
#endif

/*--------------------------------------------------------------
 *
 *  Abstract: simplex
 *
 *    Finds those values of parameters which minimize the maximal
 *    absolute value of the state functions.
 *
 *
 *  Returns: int error code
 *
 *  Calling sequence: simplex(nparms, parms, pp, pfunc, value)
 *
 *  Arguments
 *	Input:	nparms	   int		number of parameters
 *		*parms[]   double	pointers to parameters
 *		pp[]	   double	vector of SCoP variables
 *		(*pfunc)() int		name of function to
 *					evaluate state variables
 *		*value[],double		pointers to state function
 *					values
 *
 *	Output:	*parms[]	double	contains optimal parameter values
 *
 *  Functions called: (*pfunc)
 *
 *--------------------------------------------------------------*/

#include <math.h>
#include "errcodes.h"

#if MAC
#define v mlhv
#endif

#undef ALPHA

# define	FITFUN(vec) fitfun(vec,parms,pp,pfunc,value)
# define	ALPHA	-1.0
# define	BETA	0.5
# define	GAMMA	2.0

static double **v;		/* The Simplex */
static int n;			/* Number of parameters to be optimized */
static double *y;		/* Function value of simplex vectors */
static int high, next, low;	/* Index of vector with largest, etc. value */
static double *vrefl, *vnew, *vcent, *v0;	/* Work space vectors */
#define HOC	1
/* HOC not using limits at this time. Either make it global or do the job
   in the error function */
static double **limits;		/* Limits on values of parameters */

int simplex(nparms, parms, pp, pfunc, value)
int nparms, (*pfunc) ();
double pp[];
int *parms, *value;
#define parms_(arg)  pp[parms[arg]]
#define value_(arg)  pp[value[arg]]
{
#if !HOC
    extern int arraysize, **p_to_prs;
    extern struct parrec
    {
	char name[21], valu[21], units[21];
	int flag, vsize, pindex;
    }   *prs;
#endif
    extern double *makevector(), **makematrix();
    int i, j, iter = 0, getcentroid(), reflect(), copyvec(), getminimum();
    double fitfun(), y0, ysave;

    /* Initialize */
    n = nparms;
    v = makematrix(n + 1, n);
    y = makevector(n + 1);
    v0 = makevector(n + 1);
    vrefl = makevector(n);
    vnew = makevector(n);
    vcent = makevector(n);
    limits = makematrix(n, 2);	/* limits on parameter values */

    /*
     * Form initial simplex of n+1 vectors. Initial guess is first vector
     */
    for (j = 0; j < n; j++)
    {
	v0[j] = parms_(j);
	v[0][j] = v0[j];
    }
    for (i = 1; i <= n; i++)
    {
	for (j = 0; j < n; j++)
	    v[i][j] = v0[j];
	if (v0[i - 1] < ROUNDOFF)
	    v[i][i - 1] += STEP;
	else
	    v[i][i - 1] += v0[i - 1] * INITSIMPLEX;
    }

    /* Get initial function values */
    for (i = 0; i <= n; i++)
	y[i] = FITFUN(v[i]);

#if !HOC
    /* Find limits on parameter values */
    for (i = 0; i < n; i++)
	for (j = 0; j < arraysize - 2; j++)
	    if (p_to_prs[j][0] == parms[i])
	    {
		sscanf(prs[p_to_prs[j][1]].valu, "%lf:%lf", &limits[i][0],
		       &limits[i][1]);
		break;
	    }
#endif

    /* Main loop */
    while (++iter)
    {

	/* Determine worst, next worst, and best point */
	for (i = 1, high = low = 0; i <= n; i++)
	{
	    if (y[low] > y[i])
		low = i;
	    if (y[high] < y[i])
		high = i;
	}
	for (i = 0, next = low; i <= n; i++)
	    if (i != high && y[next] < y[i])
		next = i;

	/*
	 * If maximum iterations exceeded, return parameters for vertex with
	 * lowest y
	 */
	if (iter >= MAXSMPLXITERS)
	{
	    getminimum(v[low], pp, parms);
	    return (EXCEED_ITERS);
	}

	/*
	 * If range of y values is < CONVERGE, return parameter values at
	 * centroid or at v[low], whichever has lowest y value.
	 */
	if ((y[high] - y[low]) < CONVERGE)
	{
	    getcentroid(v0, -1);
	    y0 = FITFUN(v0);
	    if ((y[high] - y0) < CONVERGE)
	    {
		if (y0 > y[low])
		{
		    getminimum(v[low], pp, parms);
		    return (SUCCESS);
		}
		else
		{
		    getminimum(v0, pp, parms);
		    return (SUCCESS);
		}
	    }
	    copyvec(v0, v[high]);
	    y[high] = y0;
	    continue;
	}

	/* Find centroid of all vectors except worst */
	getcentroid(vcent, high);

	/*
	 * Reflect the worst point through the centroid and decide if
	 * improvement
	 */
	reflect(vcent, v[high], vrefl, ALPHA);
	ysave = y0 = FITFUN(vrefl);
	if (y0 < y[low])	/* Looks promising, try extending */
	{
	    reflect(vcent, vrefl, vnew, GAMMA);
	    y0 = FITFUN(vnew);
	    if (y0 < ysave)
	    {
		copyvec(vnew, v[high]);
		y[high] = y0;
		continue;
	    }
	    copyvec(vrefl, v[high]);
	    y[high] = ysave;
	    continue;
	}
	if (y0 < y[next])	/* Use it */
	{
	    copyvec(vrefl, v[high]);
	    y[high] = ysave;
	    continue;
	}
	if (y0 > y[high])
	{
	    copyvec(v[high], vrefl);
	    ysave = y[high];
	}
	else
	{
	    copyvec(vrefl, v[high]);
	    y[high] = ysave;
	}

	/* Try a short projection */
	reflect(vcent, vrefl, vnew, BETA);
	y0 = FITFUN(vnew);
	if (y0 < y[high])
	{
	    copyvec(vnew, v[high]);
	    y[high] = y0;
	    if (y0 < y[next])	/* Use it */
		continue;
	}

	/* Shrink toward best point */
	for (i = 0; i <= n; i++)
	    if (i != low)
	    {
		for (j = 0; j < n; j++)
		    v[i][j] = (v[i][j] + v[low][j]) * BETA;
		y[i] = FITFUN(v[i]);
	    }
    }
    return EXCEED_ITERS;
}

int getcentroid(vcent, ignore)
double *vcent;
int ignore;
{
    int i, j;
    double *vp1, *vp2, fn;

    /* Centroid of all vectors except ignore in simplex */
    if (ignore < 0 || ignore > n)
	fn = (double) (n + 1);
    else
	fn = (double) (n);
    for (j = 0, vp1 = vcent; j < n; j++)
	*vp1++ = 0.;
    for (i = 0; i <= n; i++)
	if (i != ignore)
	    for (j = 0, vp1 = vcent, vp2 = v[i]; j < n; j++)
		*vp1++ += *vp2++;
    for (j = 0, vp1 = vcent; j < n; j++)
	*vp1++ /= fn;
    return 0;
}

int copyvec(v1, v2)			/* Copy vector 1 into vector 2 */
double *v1, *v2;
{
    int i;

    for (i = 0; i < n; i++)
	*v2++ = *v1++;
    return 0;
}

int reflect(v0, v1, v2, coef)	/* Projection operator */
double *v0, *v1, *v2, coef;
{
    int i;

    /*
     * project a vector from v0 to v2 through v1 with expansion coefficient
     * "coef"
     */

    for (i = 0; i < n; i++, v2++)
    {
	*v2 = coef * (*v1++ - v0[i]) + v0[i];

#if !HOC
	/* Prevent projected value from exceeding limits */
	if (*v2 < limits[i][0])
	    *v2 = limits[i][0];
	else if (*v2 > limits[i][1])
	    *v2 = limits[i][1];
#endif
    }
    return 0;

}

double
fitfun(vec, parms, pp, func, vals)
double vec[], pp[];int *parms, *vals;
int (*func) ();
{
    int i;
    double temp = 0.0;

    /* Update parameters and evaluate state functions */
    for (i = 0; i < n; i++)
	parms_(i) = vec[i];
    (*func) (pp);

    /* Find largest element of vector of state functions */
    for (i = 0; i < n; i++)
	if (temp < fabs(pp[vals[i]]))
	    temp = fabs(pp[vals[i]]);

    return (temp);
}

int getminimum(vec,pp, parms)
double vec[], *pp; int *parms;
{
    extern int freevector(), freematrix();
    int i;

    for (i = 0; i < n; i++)
	parms_(i) = vec[i];

    freematrix(v);
    freevector(y);
    freevector(v0);
    freevector(vrefl);
    freevector(vnew);
    freevector(vcent);
    freematrix(limits);
    return 0;
}
