/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <string>

#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/utils/profile/profiler_interface.h"
#include "coreneuron/coreneuron.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"

namespace coreneuron {
/*
Fixed step method with threads and cache efficiency. No extracellular,
sparse matrix, multisplit, or legacy features.
*/

static void nrn_rhs(NrnThread* _nt) {
    int i1 = 0;
    int i2 = i1 + _nt->ncell;
    int i3 = _nt->end;

    double* vec_rhs = &(VEC_RHS(0));
    double* vec_d = &(VEC_D(0));
    double* vec_a = &(VEC_A(0));
    double* vec_b = &(VEC_B(0));
    double* vec_v = &(VEC_V(0));
    int* parent_index = _nt->_v_parent_index;

    nrn_pragma_acc(parallel loop present(vec_rhs [0:i3], vec_d [0:i3]) if (_nt->compute_gpu)
                       async(_nt->stream_id))
    nrn_pragma_omp(target teams distribute parallel for if(_nt->compute_gpu))
    for (int i = i1; i < i3; ++i) {
        vec_rhs[i] = 0.;
        vec_d[i] = 0.;
    }

    if (_nt->nrn_fast_imem) {
        double* fast_imem_d = _nt->nrn_fast_imem->nrn_sav_d;
        double* fast_imem_rhs = _nt->nrn_fast_imem->nrn_sav_rhs;
        nrn_pragma_acc(
            parallel loop present(fast_imem_d [i1:i3], fast_imem_rhs [i1:i3]) if (_nt->compute_gpu)
                async(_nt->stream_id))
        nrn_pragma_omp(target teams distribute parallel for if(_nt->compute_gpu))
        for (int i = i1; i < i3; ++i) {
            fast_imem_d[i] = 0.;
            fast_imem_rhs[i] = 0.;
        }
    }

    nrn_ba(_nt, BEFORE_BREAKPOINT);
    /* note that CAP has no current */
    for (auto tml = _nt->tml; tml; tml = tml->next)
        if (corenrn.get_memb_func(tml->index).current) {
            mod_f_t s = corenrn.get_memb_func(tml->index).current;
            std::string ss("cur-");
            ss += nrn_get_mechname(tml->index);
            Instrumentor::phase p(ss.c_str());
            (*s)(_nt, tml->ml, tml->index);
#ifdef DEBUG
            if (errno) {
                hoc_warning("errno set during calculation of currents", nullptr);
            }
#endif
        }

    if (_nt->nrn_fast_imem) {
        /* _nrn_save_rhs has only the contribution of electrode current
           so here we transform so it only has membrane current contribution
        */
        double* p = _nt->nrn_fast_imem->nrn_sav_rhs;
        nrn_pragma_acc(parallel loop present(p, vec_rhs) if (_nt->compute_gpu)
                           async(_nt->stream_id))
        nrn_pragma_omp(target teams distribute parallel for if(_nt->compute_gpu))
        for (int i = i1; i < i3; ++i) {
            p[i] -= vec_rhs[i];
        }
    }

    /* now the internal axial currents.
    The extracellular mechanism contribution is already done.
            rhs += ai_j*(vi_j - vi)
    */
    nrn_pragma_acc(parallel loop present(vec_rhs [0:i3],
                                         vec_d [0:i3],
                                         vec_a [0:i3],
                                         vec_b [0:i3],
                                         vec_v [0:i3],
                                         parent_index [0:i3]) if (_nt->compute_gpu)
                       async(_nt->stream_id))
    nrn_pragma_omp(target teams distribute parallel for if(_nt->compute_gpu))
    for (int i = i2; i < i3; ++i) {
        double dv = vec_v[parent_index[i]] - vec_v[i];
        /* our connection coefficients are negative so */
        nrn_pragma_acc(atomic update)
        nrn_pragma_omp(atomic update)
        vec_rhs[i] -= vec_b[i] * dv;
        nrn_pragma_acc(atomic update)
        nrn_pragma_omp(atomic update)
        vec_rhs[parent_index[i]] += vec_a[i] * dv;
    }
}

/* calculate left hand side of
cm*dvm/dt = -i(vm) + is(vi) + ai_j*(vi_j - vi)
cx*dvx/dt - cm*dvm/dt = -gx*(vx - ex) + i(vm) + ax_j*(vx_j - vx)
with a matrix so that the solution is of the form [dvm+dvx,dvx] on the right
hand side after solving.
This is a common operation for fixed step, cvode, and daspk methods
*/

static void nrn_lhs(NrnThread* _nt) {
    int i1 = 0;
    int i2 = i1 + _nt->ncell;
    int i3 = _nt->end;

    /* note that CAP has no jacob */
    for (auto tml = _nt->tml; tml; tml = tml->next)
        if (corenrn.get_memb_func(tml->index).jacob) {
            mod_f_t s = corenrn.get_memb_func(tml->index).jacob;
            std::string ss("cur-");
            ss += nrn_get_mechname(tml->index);
            Instrumentor::phase p(ss.c_str());
            (*s)(_nt, tml->ml, tml->index);
#ifdef DEBUG
            if (errno) {
                hoc_warning("errno set during calculation of jacobian", (char*) 0);
            }
#endif
        }
    /* now the cap current can be computed because any change to cm by another model
    has taken effect
    */
    /* note, the first is CAP if there are any nodes*/
    if (_nt->end && _nt->tml) {
        assert(_nt->tml->index == CAP);
        nrn_jacob_capacitance(_nt, _nt->tml->ml, _nt->tml->index);
    }

    double* vec_d = &(VEC_D(0));
    double* vec_a = &(VEC_A(0));
    double* vec_b = &(VEC_B(0));
    int* parent_index = _nt->_v_parent_index;

    if (_nt->nrn_fast_imem) {
        /* _nrn_save_d has only the contribution of electrode current
           so here we transform so it only has membrane current contribution
        */
        double* p = _nt->nrn_fast_imem->nrn_sav_d;
        nrn_pragma_acc(parallel loop present(p, vec_d) if (_nt->compute_gpu) async(_nt->stream_id))
        nrn_pragma_omp(target teams distribute parallel for if(_nt->compute_gpu))
        for (int i = i1; i < i3; ++i) {
            p[i] += vec_d[i];
        }
    }

    /* now add the axial currents */
    nrn_pragma_acc(parallel loop present(
        vec_d [0:i3], vec_a [0:i3], vec_b [0:i3], parent_index [0:i3]) if (_nt->compute_gpu)
                       async(_nt->stream_id))
    nrn_pragma_omp(target teams distribute parallel for if(_nt->compute_gpu))
    for (int i = i2; i < i3; ++i) {
        nrn_pragma_acc(atomic update)
        nrn_pragma_omp(atomic update)
        vec_d[i] -= vec_b[i];
        nrn_pragma_acc(atomic update)
        nrn_pragma_omp(atomic update)
        vec_d[parent_index[i]] -= vec_a[i];
    }
}

/* for the fixed step method */
void* setup_tree_matrix_minimal(NrnThread* _nt) {
    nrn_rhs(_nt);
    nrn_lhs(_nt);
    return nullptr;
}
}  // namespace coreneuron
