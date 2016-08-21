/*
  hopefully a temporary expedient to work around the inability to
  pass function pointers as arguments
*/

#include "coreneuron/mech/cfile/scoplib.h"
#include "coreneuron/mech/mod2c_core_thread.h"
#include "_kinderiv.h"

int derivimplicit_thread(int n, int* slist, int* dlist,
                         DIFUN fun, _threadargsproto_) {
  difun(fun);
  return 0;
}

int nrn_derivimplic_steer(int fun, _threadargsproto_) {
  switch(fun) {
    _NRN_DERIVIMPLIC_CASES
  }
  return 0;
}

int nrn_newton_steer(int fun, _threadargsproto_) {
  switch(fun) {
    _NRN_DERIVIMPLIC_CB_CASES
  }
  return 0;
}

int nrn_kinetic_steer(int fun, SparseObj* so, double* rhs, _threadargsproto_) {
  switch(fun) {
    _NRN_KINETIC_CASES
  }
  return 0;
}
