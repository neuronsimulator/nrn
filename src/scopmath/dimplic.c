#include <../../nrnconf.h>
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

int derivimplicit(_ninits, n, slist, dlist, p, pt, dt, fun, ptemp)
int n, _ninits;
double *p, *pt, dt, **ptemp;
int *slist, *dlist;
int (*fun)();
{
    int i;

    (*fun)();
    return 0;
}

int derivimplicit_thread(int n, int* slist, int* dlist, double** p,
  int(*fun)(void*, unsigned long, void*, void*, void*),
  void* ppvar, void* thread, void* nt, void* ml, unsigned long iml) {
    (*fun)(ml, iml, ppvar, thread, nt);
    return 0;
}


