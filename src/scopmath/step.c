#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: step.c
 *
 * Copyright (c) 1984-1991
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "step.c,v 1.1.1.1 1994/10/12 17:22:25 hines Exp" ;
#endif

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

double 
step(reset_integ, old_value, t, jumpt, jump)
double *old_value, t, jumpt, jump;
int *reset_integ;
{
    double value;

    if (t >= jumpt)
	value = jump;
    else
	value = 0.0;

    if (*old_value != value) *reset_integ = 1;
    *old_value = value;
    return (value);
}
