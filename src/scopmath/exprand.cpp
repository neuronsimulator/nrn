#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: exprand.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"

#include <cmath>
/****************************************************************/
/*								*/
/*  Abstract: exprand()						*/
/*								*/
/*    Random sample from the exponential distribution exp(-mean*x)	*/
/*								*/
/*  Calling sequence: exprand()					*/
/*								*/
/*  Arguments:							*/
/*	Input:	none						*/
/*								*/
/*	Output:	none						*/
/*								*/
/*  Functions called: scop_random()					*/
/*								*/
/*  Returns: Double precision number from the distribution	*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/*								*/
/****************************************************************/
double exprand(double mean) {
    return (-mean * std::log(scop_random()));
}
