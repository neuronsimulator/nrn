/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "coreneuron/mechanism/mech/cfile/scoplib.h"
#include "coreneuron/mechanism/mech/mod2c_core_thread.hpp"
#include "coreneuron/sim/scopmath/errcodes.h"

namespace coreneuron {
#define s_(arg) _p[s[arg] * _STRIDE]

#pragma acc routine seq
static int check_state(int, int*, _threadargsproto_);

int _ss_sparse_thread(SparseObj* v,
                      int n,
                      int* s,
                      int* d,
                      double* t,
                      double dt,
                      SPFUN fun,
                      int linflag,
                      _threadargsproto_) {
    int err;
    double ss_dt = 1e9;
    _modl_set_dt_thread(ss_dt, _nt);

    if (linflag) { /*iterate linear solution*/
        err = sparse_thread(v, n, s, d, t, ss_dt, fun, 0, _threadargs_);
    } else {
#define NIT 7
        int i = NIT;
        err = 0;
        while (i) {
            err = sparse_thread(v, n, s, d, t, ss_dt, fun, 1, _threadargs_);
            if (!err) {
                if (check_state(n, s, _threadargs_)) {
                    err = sparse_thread(v, n, s, d, t, ss_dt, fun, 0, _threadargs_);
                }
            }
            --i;
            if (!err) {
                i = 0;
            }
        }
    }

    _modl_set_dt_thread(dt, _nt);
    return err;
}

int _ss_derivimplicit_thread(int n, int* slist, int* dlist, DIFUN fun, _threadargsproto_) {
    double dtsav = _modl_get_dt_thread(_nt);
    _modl_set_dt_thread(1e-9, _nt);

    int err = derivimplicit_thread(n, slist, dlist, fun, _threadargs_);

    _modl_set_dt_thread(dtsav, _nt);
    return err;
}

static int check_state(int n, int* s, _threadargsproto_) {
    bool flag = true;
    for (int i = 0; i < n; i++) {
        if (s_(i) < -1e-6) {
            s_(i) = 0.;
            flag = false;
        }
    }
    return flag ? 1 : 0;
}

void _modl_set_dt_thread(double dt, NrnThread* nt) {
    nt->_dt = dt;
}
double _modl_get_dt_thread(NrnThread* nt) {
    return nt->_dt;
}
}  // namespace coreneuron
