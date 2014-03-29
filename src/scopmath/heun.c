#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: heun.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "heun.c,v 1.1.1.1 1994/10/12 17:22:21 hines Exp" ;
#endif

/****************************************************************/
/*                                                              */
/*  Abstract: heun()						*/
/*                                                              */
/*    Predictor-corrector (modified Euler-Heun) integration     */
/*    subroutine for a set of first-order ordinary differential */
/*    equations.                                                */
/*                                                              */
/*  Calling sequence:                                           */
/*      heun(neqn, var, der, p, t, dt, func, work)		*/
/*                                                              */
/*  Returns: int error code (always SUCCESS for ;heun)		*/
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
/*              dt, double, integration stepsize		*/
/*                                                              */
/*              func, name of function which evaluates the	*/
/*			time derivatives of the dependent	*/
/*			variables				*/
/*                                                              */
/*              *work, pointer to double precision work array	*/
/*                                                              */
/*     Output:  time, the dependent variables, and the time	*/
/*			derivatives of the dependent variables	*/
/*			are updated				*/
/*                                                              */
/*  Functions called: derivative function (named in call) must	*/
/*                be supplied by user; call is func(p)          */
/*                                                              */
/*  Files accessed: none                                        */
/*                                                              */
/****************************************************************/

#include "errcodes.h"

int heun(_ninits, neqn, var, der, p, t, dt, func, work)
int _ninits;
double p[], *t, dt, **work; int var[]; int der[];
#define der_(arg)  p[der[arg]]
#define var_(arg)  p[var[arg]]
int neqn, (*func) ();
{
    static int _reset;
    int i, n2;
    double temp;
    extern double *makevector();

    if (*work == (double *) 0)
	*work = makevector(3 * neqn);

    n2 = neqn << 1;
    (*func) (p);		/* Get derivatives at current time point */

    for (i = 0; i < neqn; i++)
    {
	/* Save current values of the derivatives and dependent variables */

	(*work)[i] = der_(i);
	(*work)[neqn + i] = var_(i);
    }

    if (_reset)
    {
	/* If first integration step, use Euler method as predictor */

	for (i = 0; i < neqn; i++)
	    var_(i) += dt * (der_(i));
	_reset = 0;
    }
    else
	/* Else use midpoint rule as predictor */

	for (i = 0; i < neqn; i++)
	    var_(i) = (*work)[n2 + i] + 2. * dt * (der_(i));
    *t += dt;

    (*func) (p);		/* Get derivatives at new time point */
    for (i = 0; i < neqn; i++)
    {
	/* use Heun's rule for corrector */

	temp = (*work)[neqn + i] + (dt / 2.) * ((*work)[i] + (der_(i)));

	/*
	 * Reduce truncation error by taking weighted average of predicted
	 * and corrected values
	 */

	var_(i) = 0.8 * temp + 0.2 * (var_(i));

	/*
	 * Save variable values at start of integration step for next
	 * application of midpoint rule
	 */

	(*work)[n2 + i] = (*work)[neqn + i];
    }

    /* Restore original value of t -- updated in model() */

    *t -= dt;

    return (SUCCESS);
}
