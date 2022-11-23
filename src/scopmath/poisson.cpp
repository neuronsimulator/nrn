#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: poisson.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "poisson.c,v 1.2 1997/08/30 14:32:14 hines Exp" ;
#endif

/*--------------------------------------------------------------*/
/*								*/
/*  POISSON()							*/
/*								*/
/*    Computes the value of a Poisson probability density	*/
/*    function at a particular x value given the mean		*/
/*								*/
/*  Returns: Double precision value of the probability		*/
/*								*/
/*  Calling sequence: poisson(x, mean)				*/
/*								*/
/*  Arguments							*/
/*    Input:	x, int		the independent variable	*/
/*								*/
/*		mean, double	mean of the distribution	*/
/*								*/
/*    Output:	arguments are unchanged				*/
/*								*/
/*  Functions called: factorial					*/
/*								*/
/*--------------------------------------------------------------*/
#include <cmath>
#include "scoplib.h"

double poisson(double x, double mean)
{
    double value;
    value = (std::pow(mean, x) * std::exp(-mean)) / factorial(x);
    return (value);
}
