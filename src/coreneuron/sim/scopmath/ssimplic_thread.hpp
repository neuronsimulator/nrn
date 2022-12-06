/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/
#pragma once
#include "coreneuron/mechanism/mech/mod2c_core_thread.hpp"

namespace coreneuron {

#if defined(scopmath_ssimplic_s)
#error "naming clash on ssimplic_thread.hpp-internal macros"
#endif
#define scopmath_ssimplic_s(arg) _p[s[arg] * _STRIDE]
static int check_state(int n, int* s, _threadargsproto_) {
    bool flag{true};
    for (int i = 0; i < n; i++) {
        if (scopmath_ssimplic_s(i) < -1e-6) {
            scopmath_ssimplic_s(i) = 0.;
            flag = false;
        }
    }
    return flag;
}
#undef scopmath_ssimplic_s

template <typename SPFUN>
int _ss_sparse_thread(SparseObj* so,
                      int n,
                      int* s,
                      int* d,
                      double* t,
                      double dt,
                      SPFUN fun,
                      int linflag,
                      _threadargsproto_) {
    int err;
    double ss_dt{1e9};
    _nt->_dt = ss_dt;

    if (linflag) { /*iterate linear solution*/
        err = sparse_thread(so, n, s, d, t, ss_dt, fun, 0, _threadargs_);
    } else {
        int ii{7};
        err = 0;
        while (ii) {
            err = sparse_thread(so, n, s, d, t, ss_dt, fun, 1, _threadargs_);
            if (!err) {
                if (check_state(n, s, _threadargs_)) {
                    err = sparse_thread(so, n, s, d, t, ss_dt, fun, 0, _threadargs_);
                }
            }
            --ii;
            if (!err) {
                ii = 0;
            }
        }
    }

    _nt->_dt = dt;
    return err;
}

template <typename DIFUN>
int _ss_derivimplicit_thread(int n, int* slist, int* dlist, DIFUN fun, _threadargsproto_) {
    double const dtsav{_nt->_dt};
    _nt->_dt = 1e-9;
    int err = fun(_threadargs_);
    _nt->_dt = dtsav;
    return err;
}
}  // namespace coreneuron
