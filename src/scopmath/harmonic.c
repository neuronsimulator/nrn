#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: harmonic.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "harmonic.c,v 1.1.1.1 1994/10/12 17:22:21 hines Exp" ;
#endif

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

double 
harmonic(t, period, amplitude, offset)
double t, period, amplitude, offset;
{
    double value;
    extern double sin();

    value = amplitude * sin(TWO_PI / period * (t + offset));
    return (value);
}
