#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: sawtooth.c
 *
 * Copyright (c) 1984-1991
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"

#include <cmath>
/****************************************************************/
/*								*/
/*  Abstract: sawtooth()					*/
/*								*/
/*    Computes the value of a sawtooth function (periodic ramp)	*/
/*    of specified amplitude and period				*/
/*								*/
/*  Returns: Double precision value of the sawtooth function	*/
/*								*/
/*  Calling sequence: sawtooth(t, period, amplitude)		*/
/*								*/
/*  Arguments							*/
/*    Input:	t, double	the independent variable,	*/
/*				usually time			*/
/*								*/
/*		period, double	duration of the ramp incline	*/
/*				for each cycle			*/
/*								*/
/*		amplitude, double				*/
/*				ramp height for each cycle	*/
/*								*/
/*    Output:  reset_integ, *int  set to 1 if discontinuity is  */
/*                              generated                       */
/*              old_value, *double  saved value from previous   */
/*                              call                            */
/*								*/
/*  Functions called: none					*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/*  Author: Michael C. Kohn					*/
/*								*/
/*  Date: 23 June 1987  					*/
/*								*/
/****************************************************************/
double sawtooth(int* reset_integ, double* old_value, double t, double period, double amplitude) {
    double value = amplitude * std::modf(t / period, &value);
    if (value != *old_value) *reset_integ = 1;
    *old_value = value;
    return (value);
}
