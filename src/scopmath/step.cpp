#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: step.c
 *
 * Copyright (c) 1984-1991
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"
/****************************************************************/
/*								*/
/*  Abstract: step()						*/
/*								*/
/*    Generates a single jump of specified height and duration	*/
/*    in a constant function.					*/
/*								*/
/*  Calling sequence: step(t, jumpt, jump)			*/
/*								*/
/*  Arguments							*/
/*    Input:	t, double	the independent variable,	*/
/*				usually time			*/
/*								*/
/*		jumpt, double	value of t at which step occurs	*/
/*								*/
/*		jump, double	step height			*/
/*								*/
/*    Output:  reset_integ, *int  set to 1 if discontinuity is  */
/*                              generated                       */
/*              old_value, *double  saved value from previous   */
/*                              call                            */
/*								*/
/*  Returns: Double precision value of the step function, zero	*/
/*	     before the step is taken or the step height after	*/
/*	     the step is taken					*/
/*								*/
/*  Functions called: none					*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/****************************************************************/
double step(int* reset_integ, double* old_value, double t, double jumpt, double jump) {
    double value;

    if (t >= jumpt)
        value = jump;
    else
        value = 0.0;

    if (*old_value != value)
        *reset_integ = 1;
    *old_value = value;
    return (value);
}
