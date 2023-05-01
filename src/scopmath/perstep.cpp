#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: perstep.c
 *
 * Copyright (c) 1987-1991
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"
/*--------------------------------------------------------------*/
/*								*/
/*  PERSTEP							*/
/*								*/
/*    Generates periodic jumps of specified height and duration	*/
/*    in a constant function.					*/
/*								*/
/*  Returns: Double precision value of the step function, zero	*/
/*	     before the first step is taken or the cumulative	*/
/*	     sum of step heights already taken			*/
/*								*/
/*  Calling sequence: perstep(t, base, lag, period, jump)	*/
/*								*/
/*  Arguments							*/
/*    Input:	t, double	the independent variable,	*/
/*				usually time			*/
/*								*/
/*		lag, double	value of t at which stepping	*/
/*				starts				*/
/*								*/
/*		period, double	duration of each step.  First	*/
/*				step taken			*/
/*								*/
/*		jump, double	step height			*/
/*								*/
/*    Output:  reset_integ, *int  set to 1 if discontinuity is  */
/*                              generated                       */
/*              old_value, *double  saved value from previous   */
/*                              call                            */
/*								*/
/*--------------------------------------------------------------*/
double perstep(int* reset_integ,
               double* old_value,
               double t,
               double lag,
               double period,
               double jump) {
    int njumps;
    double value;

    if (t < lag)
        value = 0.0;
    else {
        njumps = (int) ((t - lag) / period) + 1;
        value = (double) njumps * jump;
    }

    if (*old_value != value)
        *reset_integ = 1;
    *old_value = value;

    return (value);
}
