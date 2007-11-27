#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: exprand.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "exprand.c,v 1.2 1998/07/01 20:39:49 hines Exp" ;
#endif

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

double 
exprand(double mean)
{
    extern double scop_random(), log();

    return (-mean*log(scop_random()));
}
