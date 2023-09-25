#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: hyperbol.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"

/****************************************************************/
/*								*/
/*  Abstract: hyperbol()					*/
/*								*/
/*    Calculates the value of an hyperbolic function given the	*/
/*    values of two constant parameters.			*/
/*								*/
/*  Returns: Double precision value of the hyperbolic function	*/
/*								*/
/*  Calling sequence: hyperbol(x, max, K)			*/
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
/****************************************************************/
double hyperbol(double x, double max, double K) {
    return (max * x) / (x + K);
}
