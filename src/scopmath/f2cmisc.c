#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: f2cmisc.c
 *
 * Copyright (c) 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "f2cmisc.c,v 1.1.1.1 1994/10/12 17:22:20 hines Exp" ;
#endif


/* Three functions are used by lsoda which are apparently from the f77
library.  These are:

double d_sign(&this,&that)  this and that are doublereal
double pow_dd(&this,&that)  this and that are doublereal
double pow_di(&this,&that)  this is doublereal and that is int
double sqrt(doublereal)     Looks directly compatible with C sqrt
*/

#include <math.h>

double d_sign(input, sign)
double input[1], sign[1];
{
  if (*sign >= 0. && *input >= 0.) return(*input);
  if (*sign <= 0. && *input <= 0.) return(*input);
  return(-(*input));
}

double pow_dd(mantissa, exponent)
double mantissa[1], exponent[1];
{
  return(pow(*mantissa, *exponent));
}

double pow_di(mantissa, exponent)
double mantissa[1];
int exponent[1];
{
  return(pow(*mantissa, (double) *exponent));
}

