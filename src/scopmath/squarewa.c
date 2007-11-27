#include <../../nrnconf.h>

/******************************************************************************
 *
 * File: squarewa.c
 *
 * Copyright (c) 1984-1991
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "squarewa.c,v 1.2 1997/08/30 14:32:22 hines Exp" ;
#endif

/****************************************************************/
/*								*/
/*  Abstract: squarewave()					*/
/*								*/
/*    Generates a sqare wave of specified amplitude and period	*/
/*								*/
/*  Calling sequence: squarewave(t, period, amplitude)		*/
/*								*/
/*  Arguments							*/
/*    Input:	t, double	the independent variable,	*/
/*				usually time			*/
/*								*/
/*		period, double	length of each cycle, including */
/*				positive and negative phases	*/
/*								*/
/*		amplitude, double				*/
/*				positive amplitude of wave	*/
/*								*/
/*    Output:  reset_integ, *int  set to 1 if discontinuity is  */
/*                              generated                       */
/*              old_value, *double  saved value from previous   */
/*                              call                            */
/*								*/
/*  Returns: Double precision value of the square wave function	*/
/*	     Positive amplitude is first half of cycle, and	*/
/*	     negative amplitude is second half of cycle.	*/
/*								*/
/*  Functions called: none					*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/****************************************************************/

double 
squarewave(reset_integ, old_value, t, period, amplitude)
double *old_value, t, period, amplitude;
int *reset_integ;
{
    double temp, value;
#ifndef MAC
    extern double modf();
#endif

    temp = modf(t / period, &temp);
    if (temp < 0.5)
	value = amplitude;
    else
	value = -amplitude;

    if (value != *old_value) *reset_integ = 1;
    *old_value = value;
    return (value);
}
