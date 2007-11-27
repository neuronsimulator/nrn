#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: gauss.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "gauss.c,v 1.2 1997/08/30 14:32:06 hines Exp" ;
#endif

/****************************************************************/
/*								*/
/*  Abstract: gauss()	          				*/
/*								*/
/*    Computes the value of a Gaussian probability density	*/
/*    function at a particular x value given the mean and	*/
/*    standard deviation					*/
/*								*/
/*  Returns: Double precision value of the probability		*/
/*								*/
/*  Calling sequence: gauss(x, mean, stddev)			*/
/*								*/
/*  Arguments							*/
/*    Input:	x, double	the independent variable	*/
/*								*/
/*		mean, double	mean of the distribution	*/
/*								*/
/*		stddev, double	standard deviation of the	*/
/*				distribution			*/
/*								*/
/*    Output:	arguments are unchanged				*/
/*								*/
/*  Functions called: none					*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/****************************************************************/

double 
gauss(x, mean, stddev)
double x, mean, stddev;
{
#define two_pi 6.28318530

    double temp, value;
#ifndef MAC
    extern double exp(), pow(), sqrt();
#endif

    temp = exp(-0.5 * pow((x - mean) / stddev, 2.));
    value = temp / (sqrt(two_pi) * stddev);

    return (value);
}
