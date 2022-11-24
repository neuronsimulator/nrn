#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: spline.c
 *
 * Copyright (c) 1987-1991
 *   Duke University
 *
 ******************************************************************************/
#include "newton_struct.h"
#include "scoplib.h"
/****************************************************************/
/*								*/
/*  Abstract: derivs()						*/
/*								*/
/*    Computes the second derivatives at the base points for	*/
/*    cubic spline interpolation.  Call this function once	*/
/*    prior to calling spline for each interpolation.  Note	*/
/*    that the second derivatives for the terminal base points	*/
/*    are assumed to be zero, i.e., a "natural" spline.  The	*/
/*    base points must be stored in a vector variable in	*/
/*    ascending order of numerical value.			*/
/*								*/
/*  Calling sequence: derivs(nbase, x, y, h, der)		*/
/*								*/
/*  Arguments:							*/
/*	Input:	nbase, int	number of base points		*/
/*		x, double	x values at the base points	*/
/*		y, double	y values at the base points	*/
/*								*/
/*	Output:	h, double	spacing of base points		*/
/*		der, double	second derivatives at the	*/
/*				base points			*/
/*								*/
/*  Returns: 0 if no error; 2 if tridiagonal matrix is singular	*/
/*			      or ill-conditioned		*/
/*								*/
/*  Functions called: tridiag(), makevector(), freevector(),	*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/****************************************************************/
int derivs(int nbase, double* x, double* y, double* h, double* der) {
    int i, error;
    double *a, *b, *c, *d;

    /*
     * Set up tridagonal matrix, arrays a, b, and c, and the constant vector
     * d and allocate storage for the derivative pointer array
     */

    a = makevector(nbase);
    b = makevector(nbase);
    c = makevector(nbase);
    d = makevector(nbase);

    for (i = 0; i < nbase - 1; i++)
    {
	h[i] = x[i + 1] - x[i];
	d[i] = (y[i + 1] - y[i]) / h[i];
    }
    for (i = 0; i < nbase - 2; i++)
    {
	b[i] = 2.0;
	c[i] = h[i + 1] / (h[i] + h[i + 1]);
	a[i] = 1.0 - c[i];
	d[i] = 6.0 * (d[i + 1] - d[i]) / (h[i] + h[i + 1]);
    }

    /*
     * Solve for second derivatives at interior base points and return in der
     * array
     */

    error = tridiag(nbase - 2, a, b, c, d, der+1);
    der[0] = 0.0;
    der[nbase - 1] = 0.0;

    freevector(a);
    freevector(b);
    freevector(c);
    freevector(d);

    return (error);
}

/****************************************************************/
/*								*/
/*  Abstract: spline()						*/
/*								*/
/*    Evaluates the cubic function at the point for which an	*/
/*    interpolation is desired.  Call this function once for	*/
/*    each interpolation to be performed.			*/
/*								*/
/*  Calling sequence: spline(nbase, x, y, h, der, x_inter)	*/
/*								*/
/*  Arguments:							*/
/*    Input:	nbase, int	number of base points		*/
/*		x, double	x values at the base points	*/
/*		y, double	y values at the base points	*/
/*		h, double	separations of the base points	*/
/*		der, double	second derivatives at the	*/
/*				base points			*/
/*		x_inter, double	value at which interpolation	*/
/*				is desired			*/
/*								*/
/*  Returns: double precision value of the cubic spline on the	*/
/*	     appropriate subinterval.  Returns -1.e35 if the	*/
/*	     interpolation point is outside the range spanned	*/
/*	     by the base points.				*/
/*								*/
/*  Functions called: none					*/
/*								*/
/*  Files accessed: none					*/
/****************************************************************/
double spline(int nbase, double* x, double* y, double* h, double* der, double x_inter) {
    int i;
    double factor, factor1, y_inter;

    /* find subinterval containing point at which interpolation is desired */

    if ((x_inter < x[0]) || (x_inter > x[nbase - 1]))
	return (-1.e35);

    for (i = 0; i < nbase - 1; i++)
	if ((x_inter >= x[i]) && (x_inter <= x[i + 1]))
	    break;

    /* Perform interpolation */
    factor = x_inter - x[i];
    factor1 = x[i + 1] - x_inter;
    y_inter = (der[i] * factor1 * factor1 * factor1) / (6.0 * h[i]);
    y_inter += (der[i + 1] * factor * factor * factor) / (6.0 * h[i]);
    y_inter += (y[i + 1] / h[i] - der[i + 1] * h[i] / 6.0) * factor;
    y_inter += (y[i] / h[i] - der[i] * h[i] / 6.0) * factor1;

    return (y_inter);
}
