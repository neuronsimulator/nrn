#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: quad.c
 *
 * Copyright (c) 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "quad.c,v 1.1.1.1 1994/10/12 17:22:22 hines Exp" ;
#endif

/*--------------------------------------------------------------
 *
 * Abstract: quadrature
 *
 *    Numerical quadrature of tabulated function by composite
 *    trapezoid rule
 *
 * Returns: value of integral
 *
 * Calling sequence: quadrature(npts, x, y)
 *
 *
 * Arguments:
 *    npts, double	number of points in tabulated function
 *    x[], double	array of independent variable values
 *    y[], double	array of dependent variable values
 *
 *--------------------------------------------------------------*/
 
double quadrature(npts, x, y)
double npts, x[], y[];
{
    int n, i;
    double integral = 0.0;

    n = (int) (npts - 0.9);
    for (i = 0; i < n; i++)
	integral += 0.5 * (x[i + 1] - x[i]) * (y[i] + y[i + 1]);
    return (integral);
}
