#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: normrand.c
 *
 * Copyright (c) 1987, 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "normrand.c,v 1.1.1.1 1994/10/12 17:22:22 hines Exp" ;
#endif


/*--------------------------------------------------------------*/
/*								*/
/*  NORMRAND							*/
/*								*/
/*    Selects a random number from the normal distribution with	*/
/*    specified mean and standard deviation			*/
/*								*/
/*  Returns: Double precision number from the distribution	*/
/*								*/
/*  Calling sequence: normrand(mean, std_dev)			*/
/*								*/
/*  Arguments							*/
/*    Input:	mean, double	mean of the distribution	*/
/*								*/
/*		std_dev, double	standard deviation of the	*/
/*				distribution			*/
/*								*/
/*    Output:	arguments are unchanged				*/
/*								*/
/*  Functions called: random					*/
/*								*/
/*--------------------------------------------------------------*/

double 
normrand(mean, std_dev)
double mean, std_dev;
{
    double s, v1, v2;
    extern double sqrt(), log(), scop_random();

    s = 1.0;
    while (s >= 1.0)
    {
	v1 = 2.0 * scop_random() - 1.0;
	v2 = 2.0 * scop_random() - 1.0;
	s = (v1 * v1) + (v2 * v2);
    }
    v2 = v1 * sqrt(-2.0 * log(s) / s);
    return (v2 * std_dev + mean);
}
