#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: dimplic.c
 *
 * Copyright (c) 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/
#include "scoplib.h"

int derivimplicit(int _ninits, int n, int* slist, int* dlist, double* p, double* pt, double dt, int (*fun)(), double** ptemp) {
    fun();
    return 0;
}

int derivimplicit_thread(int n, int* slist, int* dlist, double* p, int(*fun)(double*, Datum*, Datum*, NrnThread*), Datum* ppvar, Datum* thread, NrnThread* nt) {
    fun(p, ppvar, thread, nt);
    return 0;
}
