#include <../../nrnconf.h>

/******************************************************************************
 *
 * File: factorial.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "errcodes.hpp"
#include "scoplib.h"

#include <cmath>

using namespace neuron::scopmath; // for errcodes.hpp
/****************************************************************/
/*								*/
/*  Abstract: factorial()					*/
/*								*/
/*    Calculates the factorial of an integer argument		*/
/*								*/
/*  Calling sequence: factorial(n)				*/
/*								*/
/*  Arguments							*/
/*    Input:	n, double	number whose factorial is	*/
/*				desired				*/
/*								*/
/*    Output:	arguments are unchanged				*/
/*								*/
/*  Functions called: none					*/
/*								*/
/*  Returns: double precision value of the factorial.  If the	*/
/*	     argument of the function is negative, function	*/
/*	     returns -1 as an error code.			*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/****************************************************************/
double factorial(double n) {
#define two_pi 6.28318530
    int i;
    double x, value;

    x = (int) (n + 0.1);
    if (x < 0)
	abort_run(NEG_ARG);

    if (x < 2)
	value = 1.0;
    else if (x < 20)
    {
	value = 1.0;
	for (i = n; i > 1; i--)
	    value = value * i;
    }
    else
    {				/* Use Stirling's approximation */
	value = std::exp(-x) * std::pow(x, x) * std::sqrt(two_pi * x);
    }
    return (value);
}
