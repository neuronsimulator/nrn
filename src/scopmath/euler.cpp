#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: euler.c
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "errcodes.h"
#include "scoplib.h"

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
int euler(int _ninits, int neqn, int* var, int* der, double* p, double *t, double dt, int(*func)(), double** work) {
#define der_(arg)  p[der[arg]]
#define var_(arg)  p[var[arg]]
    /* Calculate the derivatives */
    func();

    /* Update dependent variables */
    for (int i = 0; i < neqn; i++) {
	    var_(i) += dt * (der_(i));
    }

    return (SUCCESS);
}
