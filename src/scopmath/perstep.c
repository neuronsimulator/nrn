#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: perstep.c
 *
 * Copyright (c) 1987-1991
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "perstep.c,v 1.1.1.1 1994/10/12 17:22:22 hines Exp" ;
#endif

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

double 
perstep(reset_integ, old_value, t, lag, period, jump)
double *old_value, t, lag, period, jump;
int *reset_integ;
{
    int njumps;
    double value;

    if (t < lag)
	value = 0.0;
    else
    {
	njumps = (int) ((t - lag) / period) + 1;
	value = (double) njumps * jump;
    }

    if (*old_value != value) *reset_integ = 1;
    *old_value = value;

    return (value);
}
