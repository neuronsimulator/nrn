#include "coreneuron/mech/cfile/scoplib.h"
/******************************************************************************
 *
 * File: dimplic.c
 *
 * Copyright (c) 1988, 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#include "coreneuron/mech/mod2c_core_thread.h"

int derivimplicit_thread(int n, int* slist, int* dlist,
  int(*fun)(_threadargsproto_), _threadargsproto_) {
    (*fun)(_threadargs_);
    return 0;
}


