#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: quad.c
 *
 * Copyright (c) 1990
 *   Duke University
 *
 ******************************************************************************/

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
 
// double quadrature(double npts, double* x, double* y)
// {
//     int n, i;
//     double integral = 0.0;

//     n = (int) (npts - 0.9);
//     for (i = 0; i < n; i++)
// 	integral += 0.5 * (x[i + 1] - x[i]) * (y[i] + y[i + 1]);
//     return (integral);
// }
