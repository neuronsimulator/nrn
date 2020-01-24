/******************************************************************************
 *
 * File: errcodes.h
 *
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
 *   Duke University
 *
 * errcodes.h,v 1.1.1.1 1994/10/12 17:22:18 hines Exp
 *
 ******************************************************************************/
namespace coreneuron {
extern int abort_run(int);
}
#define ROUNDOFF 1.e-20
#define ZERO 1.e-8
#define STEP 1.e-6
#define CONVERGE 1.e-6
#define MAXCHANGE 0.05
#define INITSIMPLEX 0.25
#define MAXITERS 50
#define MAXSMPLXITERS 100
#define MAXSTEPS 20
#define MAXHALVE 15
#define MAXORDER 6
#define MAXTERMS 3
#define MAXFAIL 10
#define MAX_JAC_ITERS 20
#define MAX_GOOD_ORDER 2
#define MAX_GOOD_STEPS 3

#define SUCCESS 0
#define EXCEED_ITERS 1
#define SINGULAR 2
#define PRECISION 3
#define CORR_FAIL 4
#define INCONSISTENT 5
#define BAD_START 6
#define NODATA 7
#define NO_SOLN 8
#define LOWMEM 9
#define DIVCHECK 10
#define NOFORCE 11
#define DIVERGED 12
#define NEG_ARG 13
#define RANGE 14
