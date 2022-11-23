#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: harmonic.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"

#include <cmath>

/****************************************************************/
/*								*/
/*  Abstract: harmonic()					*/
/*								*/
/*    Generates a sine wave of specified period, amplitude, and	*/
/*    offset							*/
/*								*/
/*  Returns: Double precision value of the harmonic function	*/
/*								*/
/*  Calling sequence: harmonic(t, period, amplitude, offset)	*/
/*								*/
/*  Arguments							*/
/*    Input:	t, double	the independent variable,	*/
/*				usually time			*/
/*								*/
/*		period, double	length of each cycle in same	*/
/*				units as t			*/
/*								*/
/*		amplitude, double				*/
/*				positive amplitude of wave	*/
/*								*/
/*		offset, double	shift of waveform in same units	*/
/*				as t				*/
/*								*/
/*    Output:	arguments unchanged				*/
/*								*/
/*  Functions called: none					*/
/*								*/
/*  Files accessed: none					*/
/*								*/
/****************************************************************/
#define TWO_PI 6.2831853
double harmonic(double t, double period, double amplitude, double offset) {
    return amplitude * std::sin(TWO_PI / period * (t + offset));
}
