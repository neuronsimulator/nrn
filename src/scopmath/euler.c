#include <../../nrnconf.h>

/******************************************************************************
 *
 * File: euler.c
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "euler.c,v 1.1.1.1 1994/10/12 17:22:20 hines Exp" ;
#endif

/****************************************************************/
/*                                                              */
/*  Abstract:  euler()                                          */
/*      Euler integration subroutine for a set of first-order   */
/*      ordinary differential equations.                        */
/*                                                              */
/*  Calling sequence:                                           */
/*      euler(neqn, var, der, p, t, dt, func, &work)		*/
/*                                                              */
/*  Arguments:                                                  */
/*      Input:  neqn, int, number of equations                  */
/*                                                              */
/*              var, pointer to array of addresses of the	*/
/*			 state variables			*/
/*                                                              */
/*              der, pointer to array of addresses of the	*/
/*			derivatives of the state variables	*/
/*                                                              */
/*              p, double precision variable array		*/
/*                                                              */
/*              t, pointer to current time			*/
/*                                                              */
/*              dt, double, integration stepsize		*/
/*                                                              */
/*              func, name of function which evaluates the	*/
/*			derivatives of the dependent variables	*/
/*                                                              */
/*		*work, pointer to work array; not used in euler	*/
/*			but kept in call for consistency with	*/
/*			other integrators			*/
/*                                                              */
/*     Output:  time, the dependent variables, and the time	*/
/*			derivatives of the dependent variables	*/
/*			are updated				*/
/*                                                              */
/*  Functions called: derivative function (named in call) must  */
/*              be supplied by user; call is func(p)            */
/*                                                              */
/*  Returns: int error code (always SUCCESS for euler)		*/
/*                                                              */
/*  Files accessed: none                                        */
/*                                                              */
/****************************************************************/

#include "errcodes.h"

/* ARGSUSED */
int euler(_ninits, neqn, var, der, p, t, dt, func, work)
int _ninits;
double p[], *t, dt, **work; int var[]; int der[];
#define der_(arg)  p[der[arg]]
#define var_(arg)  p[var[arg]]
int neqn, (*func) ();
{
    int i;

    /* Calculate the derivatives */

    (*func) (p);

    /* Update dependent variables */

    for (i = 0; i < neqn; i++)
	var_(i) += dt * (der_(i));

    return (SUCCESS);
}
