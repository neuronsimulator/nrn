#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: threshol.c
 *
 * Copyright (c) 1984-1991
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"
/****************************************************************/
/*								*/
/*  Abstract: threshold()					*/
/*								*/
/*  Switches between inactive and active states		*/
/*								*/
/*  Calling sequence:						*/
/*	 threshold(reset_integ, old_value, x, limit, mode)	*/
/*								*/
/*  Arguments							*/
/*    Input:	x, double	test variable for switch	*/
/*								*/
/*		limit, double	threshold switch point		*/
/*								*/
/*		mode, pointer	*mode = "min" indicates limit is*/
/*		      to a	minimum value for active state	*/
/*		      string	*mode = "max" indicates limit is*/
/*				maximum value for active state	*/
/*    Output:  reset_integ, *int  set to 1 if discontinuity is  */
/*				generated			*/
/*		old_value, *double  saved value from previous	*/
/*				call				*/
/*								*/
/*  Returns: 0 if argument is below (or above, depending on	*/
/*	     mode)threshold limit, 1 otherwise.  Returns -1 if	*/
/*	     input error detected. Values are doubles.		*/
/*								*/
/*  Output:	*mode string is forced to be in lower case	*/
/*		also see reset_integ and old_value above	*/
/*								*/
/*  Functions called: none					*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/****************************************************************/
double threshold(int* reset_integ, double* old_value, double x, double limit, char* mode) {
    int i;
    double ret_val;

    for (i = 0; *(mode + i) != '\0'; i++)
        if (isupper(*(mode + i)))
            *(mode + i) = tolower(*(mode + i));

    if (strcmp(mode, "min") == 0) {
        if (x >= limit)
            ret_val = 1.;
        else
            ret_val = 0.;
    }

    else if (strcmp(mode, "max") == 0) {
        if (x <= limit)
            ret_val = 1.;
        else
            ret_val = 0.;
    } else
        ret_val = -1.;
    if (*old_value != ret_val)
        *reset_integ = 1;
    *old_value = ret_val;
    return (ret_val);
}
