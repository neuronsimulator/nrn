#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: squarewa.c
 *
 * Copyright (c) 1984-1991
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"

#include <cmath>
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
double squarewave(int* reset_integ, double* old_value, double t, double period, double amplitude) {
    double temp, value;

    temp = std::modf(t / period, &temp);
    if (temp < 0.5)
        value = amplitude;
    else
        value = -amplitude;

    if (value != *old_value)
        *reset_integ = 1;
    *old_value = value;
    return (value);
}
