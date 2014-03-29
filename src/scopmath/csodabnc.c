#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: csodabnc.c
 *
 * Copyright (c) 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "csodabnc.c,v 1.2 1997/08/30 14:32:02 hines Exp" ;
#endif

#include <stdio.h>
#include <stdlib.h>
#include "scoplib.h"

typedef long int integer;
typedef double doublereal;

static int g_neq;
static double *_p;
static int *g_slist, *g_dlist;
#define g_slist_(arg)	_p[g_slist[arg]]
#define g_dlist_(arg)	_p[g_dlist[arg]]
static double *g_t;
static int (*modl_fun)();

static int funct();
static void to_gear();
static void from_gear();
static void to_modl();
static void from_modl();

int
clsoda(_ninits, n, slist, dlist, p, pt, dt, fun, ptemp, maxerr) /* Changed from gear() MDF */
	int _ninits, n;
	double  *p, *pt, dt, **ptemp, maxerr;
	int *slist, *dlist;
	int (*fun)();
{
	int i;
	double savet;
	static int ninitsav = -1;

    /* Local variables */
    static doublereal atol;
    static integer jdum, itol, iopt;
    static doublereal rtol;
    static integer iout;
    static doublereal tout, *y=(doublereal *)0;
    extern /* Subroutine */ int lsoda_();
    static integer itask, *iwork;
    static doublereal *rwork;
    static integer jt, istate;
    static integer neq, liw, lrw;

	if (_p != p) {
		_p = p;
	}
if (g_slist != slist || g_neq != n) {
	if (y) {
		free((char *)y);
		y = (doublereal *)0;
	}
	if (rwork) {
		free((char *)rwork);
		rwork = (doublereal *)0;
	}
	if (iwork) {
		free((char *)iwork);
		iwork = (integer *)0;
	}
	if ((y = (doublereal *)malloc(sizeof(doublereal)*n)) == (doublereal *)0) {
		return 1;
	}
    lrw = 22 + n * ((16 > n+9)? 16: n+9) ;
	if ((rwork = (doublereal *)malloc(sizeof(doublereal)*lrw)) == (doublereal *)0) {
		return 1;
	}
    liw = 20 + n;
	if ((iwork = (integer *)malloc(sizeof(integer)*liw)) == (integer *)0) {
		return 1;
	}
	g_slist = slist;
	g_dlist = dlist;
	g_neq = n;
	g_t = pt;
	modl_fun = fun;
}
    neq = n;
    itol = 1;
    rtol = maxerr;
    atol = maxerr;
    itask = 1;
    if (_ninits != ninitsav) {
    	ninitsav = _ninits;
    	istate = 1;
    }else{
    	istate = 2;
    }
    iopt = 0;
    jt = 2;
    tout = *pt + dt;

	savet = *pt;
	to_gear(y);
	lsoda_(funct, &neq, y, pt, &tout, &itol, &rtol, &atol, &itask, &istate, 
		&iopt, rwork, &lrw, iwork, &liw, &jdum, &jt);
	from_gear(y);
	*pt = savet;
	
	return 0;
}

static int
funct(neq, t, y, ydot)
	integer *neq;
	double *t, *y, *ydot;
{
	to_modl(t, y);
	modl_fun();
	from_modl(ydot);
	return 0;
}

static
void to_gear(y)
	double *y;
{
	int i;

	for (i=0; i<g_neq; i++) {
		y[i] = g_slist_(i);
	}
}

static
void from_gear(y)
	double *y;
{
	int i;

	for (i=0; i<g_neq; i++) {
		g_slist_(i) = y[i];
	}
}

static
void to_modl(t, y)
	double *t, *y;
{
	int i;

	*g_t = *t;
	for (i=0; i<g_neq; i++) {
		g_slist_(i) = y[i];
	}
}

static
void from_modl(ydot)
	double *ydot;
{
	int i;

	for (i=0; i<g_neq; i++) {
		ydot[i] = g_dlist_(i);
	}
}

int s_wsfe() {
	printf("called s_wsfe\n");
	abort_run(1);
	return 0;
}
int s_stop() {
	printf("called s_stop\n");
	abort_run(1);
	return 0;
}
int do_fio() {
	printf("called do_fio\n");
	abort_run(1);
	return 0;
}
int e_wsfe() {
	printf("called e_wsfe\n");
	abort_run(1);
	return 0;
}
