#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: revhyper.c
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"
/****************************************************************/
/*								*/
/*  Abstract: revhyperbol()					*/
/*								*/
/*    Calculates the value of a reverse hyperbolic function	*/
/*    given the values of two constant parameters.		*/
/*								*/
/*  Returns: Double precision value of the hyperbolic function	*/
/*								*/
/*  Calling sequence: revhyperbol(x, max, K)			*/
/*								*/
/*  Arguments							*/
/*    Input:	x, double	the independent variable	*/
/*								*/
/*		max, double	maximum value of the hyperbolic	*/
/*				function			*/
/*								*/
/*		K, double	value of x for which the	*/
/*				function value is max/2		*/
/*								*/
/*    Output:	arguments are unchanged				*/
/*								*/
/*  Functions called: none					*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/*								*/
/****************************************************************/
double revhyperbol(double x, double max, double K) {
    double value;

    value = (max * K) / (x + K);
    return (value);
}
