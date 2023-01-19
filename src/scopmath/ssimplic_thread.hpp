#pragma once
#include "dimplic.hpp"
#include "errcodes.hpp"
#include "sparse_thread.hpp"

struct NrnThread;
double _modl_get_dt_thread(NrnThread*);
void _modl_set_dt_thread(double, NrnThread*);
namespace neuron::scopmath {
template <typename Array, typename Callable, typename... Args>
int _ss_sparse_thread(void** v,
                      int n,
                      int* s,
                      int* d,
                      Array p,
                      double* t,
                      double dt,
                      Callable fun,
                      int linflag,
                      Args&&... args) {
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
    // get the NrnThread* from args
    auto* const nt = get_first<NrnThread*>(std::forward<Args>(args)...);
    _modl_set_dt_thread(ss_dt, nt);

    if (linflag) { /*iterate linear solution*/
        err =
            sparse_thread(v, n, s, d, p, t, ss_dt, std::move(fun), 0, std::forward<Args>(args)...);
    } else {
        constexpr auto NIT = 7;
        for (i = 0; i < NIT; i++) {
            err = sparse_thread(v, n, s, d, p, t, ss_dt, fun, 1, args...);
            if (err) {
                break; /* perhaps we should re-start */
            }
            if (check_state(n, s)) {
                err = sparse_thread(v, n, s, d, p, t, ss_dt, fun, 0, args...);
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
template <typename Array, typename Callable, typename... Args>
int _ss_derivimplicit_thread(int n, int* slist, int* dlist, Array p, Callable fun, Args&&... args) {
    auto* const nt = get_first<NrnThread*>(std::forward<Args>(args)...);
    auto const dtsav = _modl_get_dt_thread(nt);
    _modl_set_dt_thread(1e-9, nt);
    auto const err = derivimplicit_thread(
        n, slist, dlist, std::move(p), std::move(fun), std::forward<Args>(args)...);
    _modl_set_dt_thread(dtsav, nt);
    return err;
}
}  // namespace neuron::scopmath
using neuron::scopmath::_ss_derivimplicit_thread;
using neuron::scopmath::_ss_sparse_thread;
