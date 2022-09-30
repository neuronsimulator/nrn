#include <../../nrnconf.h>
#include "scoplib.h"
/******************************************************************************
 *
 * File: dimplic.c
 *
 * Copyright (c) 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] =
    "dimplic.c,v 1.1.1.1 1994/10/12 17:22:20 hines Exp" ;
#endif

int derivimplicit(int _ninits, int n, int* slist, int* dlist, double* p, double* pt, double dt, int (*fun)(), double** ptemp)
{
    int i;

    (*fun)();
    return 0;
}

int derivimplicit_thread(int n, int* slist, int* dlist, double* p, int(*fun)(double*, Datum*, Datum*, NrnThread*), Datum* ppvar, Datum* thread, NrnThread* nt) {
    (*fun)(p, ppvar, thread, nt);
    return 0;
}


