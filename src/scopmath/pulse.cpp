#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: pulse.c
 *
 * Copyright (c) 1984-1991
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"
/*-----------------------------------------------------------------------------
 *
 *  PULSE()
 *
 *	Generates a single pulse of specified height and duration
 *
 *  Calling sequence:
 *
 *	pulse(t, lag, height, duration)
 *
 *  Arguments:
 *    Input:	t, double	the independent variable,
 *				usually time
 *
 *		lag, double	value of t at which pulse begins
 *
 *		height, double	pulse height
 *
 *		duration,double duration of the pulse
 *
 *    Output:  reset_integ, *int  set to 1 if discontinuity is
 *                              generated                    
 *              old_value, *double  saved value from previous
 *                              call                        
 *
 *  Returns:
 *
 *  Functions called:
 *
 *  Files accessed:
 *	none
 *
 */
double pulse(int* reset_integ, double* old_value, double t, double lag, double height, double duration) {
    double value = 0.0;

    if (t < lag)
	value = 0.0;
    else if (t < lag + duration)
	value = height;
    else
	value = 0.0;

    if (*old_value != value) *reset_integ = 1;
    *old_value = value;
    return (value);
}
