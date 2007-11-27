#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: legendre.c
 *
 * Copyright (c) 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "legendre.c,v 1.1.1.1 1994/10/12 17:22:21 hines Exp" ;
#endif

/***************************************************************
 *
 *  Abstract: legendre
 *
 *    Definite integral by Gauss-Legendre quadrature using
 *    a 10th degree Legendre polynomial
 *
 *  Calling sequence: legendre(a, b, func)
 *
 *  Returns: Double precision value of the integral
 *
 *  Arguments:
 *	a, double	beginning of the integration interval
 *	b, double	end of the integration interval
 *	func		name of the function to be integrated
 *
 *  Functions called: func(x) supplied by user; x is the
 *		      independent variable of the function
 *
 *  Files accessed: none
 *
 **************************************************************/

double zero[10] = {-0.9739065285, -0.8650633677, -0.6794095683, -0.4333953941,
    -0.1488743390, 0.1488743390, 0.4333953941, 0.6794095683,
0.8650633677, 0.9739065285},

    weight[10] = {0.0666713443, 0.1494513492, 0.2190863625, 0.2692667193,
    0.2955242247, 0.2955242247, 0.2692667193, 0.2190863625,
0.1494513492, 0.0666713443};

double 
legendre(a, b, func)
double a, b, (*func) ();
{
    int i;
    double integral = 0.0;

    for (i = 0; i < 10; i++)
	integral += weight[i] * func(0.5 * (zero[i] * (b - a) + a + b));
    return (0.5 * (b - a) * integral);
}
