#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: revsawto.c
 *
 * Copyright (c) 1984-1991
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"

#include <cmath>
/****************************************************************/
/*								*/
/*  Abstract: revsawtooth()					*/
/*								*/
/*    Computes the value of a reverse sawtooth function (a	*/
/*    periodic declining ramp) of specified amplitude and	*/
/*    period							*/
/*								*/
/*  Returns: Double precision value of the reverse sawtooth	*/
/*	     function						*/
/*								*/
/*  Calling sequence: revsawtooth(t, period, amplitude)		*/
/*								*/
/*  Arguments							*/
/*    Input:	t, double	the independent variable,	*/
/*				usually time			*/
/*								*/
/*		period, double	duration of the ramp decline	*/
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
/****************************************************************/
double revsawtooth(int* reset_integ, double* old_value, double t, double period, double amplitude) {
    double value;

    value = amplitude * (1.0 - std::modf(t / period, &value));

    if (value != *old_value) *reset_integ = 1;
    *old_value = value;
    return (value);
}
