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
                         DIFUN fun, _threadargsproto_) {
  difun(fun);
  return 0;
}

int nrn_derivimplic_steer(int fun, _threadargsproto_) {
  return 0;
}

int nrn_kinetic_steer(int fun, SparseObj* so, double* rhs, _threadargsproto_) {
  return 0;
}
