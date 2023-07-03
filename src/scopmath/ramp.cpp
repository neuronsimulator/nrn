#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: ramp.c
 *
 * Copyright (c) 1984-1991
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"
/*-----------------------------------------------------------------------------
 *
 *  RAMP()
 *
 *    Computes the value of a ramp function of specified height
 *    and duration
 *
 *  Calling sequence:
 *	ramp(t, lag, height, duration)
 *
 *  Arguments:
 *    Input:	t, double	the independent variable,
 *				usually time
 *
 *		lag, double	value of t at which ramp begins
 *
 *		height, double	ramp height
 *
 *		duration,double duration of the ramp incline
 *
 *    Output:  reset_integ, *int  set to 1 if discontinuity is
 *                              generated
 *              old_value, *double  saved value from previous
 *                              call
 *
 *  Returns:
 *	Double precision value of the ramp function
 *
 *  Functions called:
 *	none
 *
 *  Files accessed:
 *	none
 *
 */
double ramp(int* reset_integ,
            double* old_value,
            double t,
            double lag,
            double height,
            double duration) {
    if (t < lag) {
        if (*old_value != 0.)
            *reset_integ = 1;
        *old_value = 0.;
        return (0.0);
    } else if (t > lag + duration) {
        if (*old_value != height)
            *reset_integ = 1;
        *old_value = height;
        return (height);
    } else {
        if (*old_value == 0. || *old_value == height)
            *reset_integ = 1;
        *old_value = (t - lag) * height / duration;
        return (*old_value);
    }
}
