#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: perpulse.c
 *
 * Copyright (c) 1984-1991
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"

#include <cmath>
/*--------------------------------------------------------------*/
/*								*/
/*  Abstract: perpulse()					*/
/*								*/
/*    Generates periodic pulses of specified height, duration,	*/
/*    and delay between pulses					*/
/*								*/
/*  Returns: Double precision value of the pulse height or zero	*/
/*								*/
/*  Calling sequence: perpulse(t, lag, height, duration, delay)	*/
/*								*/
/*  Arguments							*/
/*    Input:	t, double	the independent variable,	*/
/*				usually time			*/
/*								*/
/*		lag, double	value of t at which first pulse	*/
/*				begins				*/
/*								*/
/*		height, double	pulse height			*/
/*								*/
/*		duration,double duration of the pulse		*/
/*								*/
/*		delay, double	delay between pulses		*/
/*								*/
/*    Output:  reset_integ, *int  set to 1 if discontinuity is  */
/*                              generated                       */
/*              old_value, *double  saved value from previous   */
/*                              call                            */
/*								*/
/*--------------------------------------------------------------*/
double perpulse(int* reset_integ, double* old_value, double t, double lag, double height, double duration, double delay) {
    double temp, period, value;

    if (t < lag)
	value = 0.0;
    else
    {
	period = duration + delay;
	temp = std::modf((t - lag) / period, &value);
	if ((temp *= period) < duration)
	    value = height;
	else
	    value = 0.0;
    }

    if (*old_value != value) *reset_integ = 1;
    *old_value = value;

    return (value);
}
