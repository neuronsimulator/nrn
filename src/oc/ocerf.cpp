#include <../../nrnconf.h>
#include <errno.h>
#ifndef MINGW
#if DOS || defined(WIN32)
/******************************************************************************
 *
 * File: erf.cpp
 *
 * Copyright (c) 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] = "erf.cpp,v 1.5 1997/11/24 16:21:37 hines Exp";
#endif

/************************************************************
 *
 *  Abstract: erf()
 *
 *    Normalized error function.
 *
 *	erf(z) = (2/sqrt(PI)) * Integral from 0 to z of
 *			        exp(-t*t) dt
 *
 *    Note that erf(-z) = -erf(z).  To obtain the cumulative
 *    Gaussian distribution function at z probits:
 *
 *	z = (x - mean)/std_dev
 *	cdf(z) = 0.5 * (1 + erf(z))
 *
 *  Calling sequence: erf(z)
 *
 *  Argument: Input:	z, double
 *
 *  Functions called: none
 *
 *  Returns: double precision value of error function is on
 *	     the interval [-1, 1]
 *
 *  Files accessed: none
 *
 ***********************************************************/

#include <math.h>
#define a1 0.254829592
#define a2 -0.284496736
#define a3 1.421413741
#define a4 -1.453152027
#define a5 1.061405429
double erf(double z) {
    double t, value;

    t = 1. / (1. + 0.3275911 * fabs(z));
    value = 1. - (((((a5 * t + a4) * t + a3) * t + a2) * t + a1) * t) * exp(-z * z);

    if (z >= 0.0)
        return (value);
    else
        return (-value);
}

double erfc(double z) {
    return 1. - erf(z);
}
#endif
#endif
