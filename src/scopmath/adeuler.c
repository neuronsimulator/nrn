#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: adeuler.c
 *
 * Copyright (c) 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "adeuler.c,v 1.2 1999/01/04 12:46:40 hines Exp" ;
#endif

/*--------------------------------------------------------------*/
/*                                                              */
/*  Abstract:  adeuler()                                        */
/*      Adaptive Euler integration subroutine for a set of	*/
/*      first-order ordinary differential equations. Integrates	*/
/*      from current time, t, to t + delta_t.			*/
/*                                                              */
/*  Calling sequence:                                           */
/*      adeuler(neqn,var,der,p,t,delta_t,func,work,maxerror)	*/
/*                                                              */
/*  Returns: int error code					*/
/*                                                              */
/*  Arguments:                                                  */
/*      Input:  neqn, int, number of equations                  */
/*                                                              */
/*              var, pointer to array of addresses of the       */
/*                       state variables                        */
/*                                                              */
/*              der, pointer to array of addresses of the       */
/*                      derivatives of the state variables      */
/*                                                              */
/*              p, double precision variable array		*/
/*                                                              */
/*              t, pointer to current time			*/
/*                                                              */
/*		delta_t, double, time step for integration	*/
/*								*/
/*              func, name of function which evaluates the	*/
/*			derivatives of the dependent variables	*/
/*                                                              */
/*              *work, pointer to double precision work array	*/
/*                                                              */
/*		maxerror, double, maximum single-step error	*/
/*                                                              */
/*     Output:  time, the dependent variables, and the time	*/
/*			derivatives of the dependent variables	*/
/*			are updated				*/
/*                                                              */
/*  Functions called: derivative function (named in call) must  */
/*              be supplied by user; call is func(p)            */
/*                                                              */
/*--------------------------------------------------------------*/

#include <math.h>
#include "errcodes.h"

int adeuler(_ninits, neqn, var, der, p, t, delta_t, func, work, maxerror)
int _ninits;
double p[], *t, delta_t, maxerror, **work; int var[]; int der[];
#define der_(arg)  p[der[arg]]
#define var_(arg)  p[var[arg]]
int neqn, (*func) ();
{
    static int initialized = 0;
    int i;
    extern double *makevector();
    double end_t, temp, Dderiv;
    static double dt;

    if (*work == (double *) 0)
	*work = makevector(neqn);
    if (initialized < _ninits)
    {
	if (delta_t < 0.01)
	    dt = delta_t / 10.;
	else
	    dt = 0.001;
	(*func) (p);
	initialized = _ninits;
    }

    end_t = *t + delta_t;
    for (; *t < end_t; *t += dt)
    {
	if (*t + dt > end_t)
	    dt = end_t - *t;

	/* Update dependent variables and save derivatives */

	for (i = 0; i < neqn; i++)
	{
	    var_(i) += dt * (der_(i));
	    (*work)[i] = der_(i);
	}

	/* Calculate the derivatives at t + dt */

	(*func) (p);

	Dderiv = 0.0;

	/* Estimate maximal second derivative */

	for (i = 0; i < neqn; i++)
	    if ((temp = fabs(der_(i) - (*work)[i]) / dt) > Dderiv)
		Dderiv = temp;

	/* Calculate the step size for the next integration step */

	if (Dderiv != 0.0)
	    dt = sqrt(2.0 * maxerror / Dderiv);
    }

    /* Restore original value of t -- updated in model() */

    *t = end_t - delta_t;

    if (dt < ROUNDOFF)
	return (PRECISION);
    else
	return (SUCCESS);
}
