#pragma once
#include "dimplic.hpp"
#include "sparse.hpp"
void _modl_set_dt(double);
namespace neuron::scopmath {
template <typename Array, typename IndexArray>
int _ss_sparse(void** v,
               int n,
               IndexArray s,
               IndexArray d,
               Array p,
               double* t,
               double dt,
               int (*fun)(),
               double** pcoef,
               int linflag) {
    auto const check_state = [n, &p, &s]() {
        auto const s_ = [&p, s](auto arg) -> auto& {
            return p[s[arg]];
        };
        int flag = 1;
        for (int i = 0; i < n; i++) {
            if (s_(i) < -1e-6) {
                s_(i) = 0.;
                flag = 0;
            }
        }
        return flag;
    };
    int err, i;
    double ss_dt;

    ss_dt = 1e9;
    _modl_set_dt(ss_dt);

    if (linflag) { /*iterate linear solution*/
        err = sparse(v, n, s, d, p, t, ss_dt, fun, pcoef, 0);
    } else {
        constexpr auto NIT = 7;
        for (i = 0; i < NIT; i++) {
            err = sparse(v, n, s, d, p, t, ss_dt, fun, pcoef, 1);
            if (err) {
                break; /* perhaps we should re-start */
            }
            if (check_state()) {
                err = sparse(v, n, s, d, p, t, ss_dt, fun, pcoef, 0);
                break;
            }
        }
        if (i >= NIT) {
            err = 1;
        }
    }

    _modl_set_dt(dt);
    return err;
}

template <typename Array>
int _ss_derivimplicit(int _ninits,
                      int n,
                      int* slist,
                      int* dlist,
                      Array p,
                      double* pt,
                      double dt,
                      int (*fun)(),
                      double** ptemp) {
    double ss_dt{1e9};
    _modl_set_dt(ss_dt);
    int err = derivimplicit(_ninits, n, slist, dlist, p, pt, ss_dt, fun, ptemp);
    _modl_set_dt(dt);
    return err;
}
}  // namespace neuron::scopmath
using neuron::scopmath::_ss_derivimplicit;
using neuron::scopmath::_ss_sparse;
