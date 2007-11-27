#include <../../nrnconf.h>

/******************************************************************************
 *
 * File: revsigmo.c
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "revsigmo.c,v 1.2 1997/08/30 14:32:15 hines Exp" ;
#endif

/****************************************************************/
/*								*/
/*  Abstract: revsigmoid					*/
/*								*/
/*    Calculates the value of a reverse sigmoidal function of	*/
/*    one variable given the values of three constants in the	*/
/*    function.							*/
/*								*/
/*  Returns: Double precision value of the sigmoidal function	*/
/*								*/
/*  Calling sequence: revsigmoid(x, max, K, n)			*/
/*								*/
/*  Arguments							*/
/*    Input:	x, double	the independent variable	*/
/*								*/
/*		max, double	maximum value the sigmoidal	*/
/*				function can attain		*/
/*								*/
/*		K, double	the value for x at which the	*/
/*				function value is max/2		*/
/*								*/
/*		n, double	parameter controlling the	*/
/*				steepness of the sigmoid curve.	*/
/*				It is the power to which x and	*/
/*				K are raised.			*/
/*								*/
/*    Output:	arguments are unchanged				*/
/*								*/
/*  Functions called: none					*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/****************************************************************/

double 
revsigmoid(x, max, K, n)
double x, max, K, n;
{
    double value, temp;
#ifndef MAC
    extern double pow();
#endif

    temp = pow(K, n);
    value = (max * temp) / (temp + pow(x, n));
    return (value);
}
