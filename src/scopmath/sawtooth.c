#include <../../nrnconf.h>

/******************************************************************************
 *
 * File: sawtooth.c
 *
 * Copyright (c) 1984-1991
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "sawtooth.c,v 1.2 1997/08/30 14:32:17 hines Exp" ;
#endif

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

double 
sawtooth(reset_integ, old_value, t, period, amplitude)
double t, period, amplitude, *old_value;
int *reset_integ;
{
    double value;
#ifndef MAC
    extern double modf();
#endif

    value = amplitude * modf(t / period, &value);

    if (value != *old_value) *reset_integ = 1;
    *old_value = value;
    return (value);
}
