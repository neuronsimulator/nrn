#pragma once
#include "dimplic.hpp"
#include "errcodes.hpp"
#include "hocdec.h" // Datum
#include "sparse_thread.hpp"

struct NrnThread;
double _modl_get_dt_thread(NrnThread*);
void _modl_set_dt_thread(double, NrnThread*);
namespace neuron::scopmath {
template <typename Array>
int _ss_sparse_thread(void** v,
                      int n,
                      int* s,
                      int* d,
                      Array p,
                      double* t,
                      double dt,
                      int (*fun)(void*, double*, double*, Datum*, Datum*, NrnThread*),
                      int linflag,
                      Datum* ppvar,
                      Datum* thread,
                      NrnThread* nt) {
    auto const check_state = [&p](int n, int* s) {
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
    auto const ss_dt = 1e9;
    _modl_set_dt_thread(ss_dt, nt);

    if (linflag) { /*iterate linear solution*/
        err = sparse_thread(v, n, s, d, p, t, ss_dt, fun, 0, ppvar, thread, nt);
    } else {
        constexpr auto NIT = 7;
        for (i = 0; i < NIT; i++) {
            err = sparse_thread(v, n, s, d, p, t, ss_dt, fun, 1, ppvar, thread, nt);
            if (err) {
                break; /* perhaps we should re-start */
            }
            if (check_state(n, s)) {
                err = sparse_thread(v, n, s, d, p, t, ss_dt, fun, 0, ppvar, thread, nt);
                break;
            }
        }
        if (i >= NIT) {
            err = 1;
        }
    }

    _modl_set_dt_thread(dt, nt);
    return err;
}
template <typename Array>
int _ss_derivimplicit_thread(int n,
                             int* slist,
                             int* dlist,
                             Array p,
                             int (*fun)(double*, Datum*, Datum*, NrnThread*),
                             Datum* ppvar,
                             Datum* thread,
                             NrnThread* nt) {
    auto const dtsav = _modl_get_dt_thread(nt);
    _modl_set_dt_thread(1e-9, nt);
    auto const err = derivimplicit_thread(n, slist, dlist, p, fun, ppvar, thread, nt);
    _modl_set_dt_thread(dtsav, nt);
    return err;
}
}  // namespace neuron::scopmath
using neuron::scopmath::_ss_derivimplicit_thread;
using neuron::scopmath::_ss_sparse_thread;
