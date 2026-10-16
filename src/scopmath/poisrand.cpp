#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: poisrand.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"

#include <cmath>
/*--------------------------------------------------------------*/
/*								*/
/*  POISRAND()							*/
/*								*/
/*    Selects a random number from the Poisson distribution	*/
/*    with specified mean					*/
/*								*/
/*  Returns: integer from the Poisson distribution		*/
/*								*/
/*  Calling sequence: poisrand(mean)				*/
/*								*/
/*  Arguments							*/
/*    Input:	mean, double	mean of the distribution	*/
/*								*/
/*    Output:	arguments are unchanged				*/
/*								*/
/*  Functions called: random					*/
/*								*/
/*--------------------------------------------------------------*/
int poisrand(double mean) {
    int n;
    double s, q;

    n = 0;
    s = std::exp(-mean);
    q = 1.0;
    while (q >= s) {
        q = q * scop_random();
        ++n;
    }
    return (n - 1);
}
