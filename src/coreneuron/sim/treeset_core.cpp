/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
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

// clang-format off
    #pragma acc parallel loop present(          \
        vec_rhs[0:i3], vec_d[0:i3])             \
        if (_nt->compute_gpu) async(_nt->stream_id)
    // clang-format on
    for (int i = i1; i < i3; ++i) {
        vec_rhs[i] = 0.;
        vec_d[i] = 0.;
    }

    if (_nt->nrn_fast_imem) {
        for (int i = i1; i < i3; ++i) {
            _nt->nrn_fast_imem->nrn_sav_rhs[i] = 0.;
            _nt->nrn_fast_imem->nrn_sav_d[i] = 0.;
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
        for (int i = i1; i < i3; ++i) {
            p[i] -= vec_rhs[i];
        }
    }

/* now the internal axial currents.
The extracellular mechanism contribution is already done.
        rhs += ai_j*(vi_j - vi)
*/
// clang-format off
    #pragma acc parallel loop present(          \
        vec_rhs[0:i3], vec_d[0:i3],             \
        vec_a[0:i3], vec_b[0:i3],               \
        vec_v[0:i3], parent_index[0:i3])        \
        if (_nt->compute_gpu) async(_nt->stream_id)
    for (int i = i2; i < i3; ++i) {
        double dv = vec_v[parent_index[i]] - vec_v[i];
        /* our connection coefficients are negative so */
        #pragma acc atomic update
        vec_rhs[i] -= vec_b[i] * dv;
        #pragma acc atomic update
        vec_rhs[parent_index[i]] += vec_a[i] * dv;
    }
    // clang-format on
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
                hoc_warning("errno set during calculation of jacobian", (char*)0);
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
        for (int i = i1; i < i3; ++i) {
            p[i] += vec_d[i];
        }
    }

/* now add the axial currents */
// clang-format off
    #pragma acc parallel loop present(          \
        vec_d[0:i3], vec_a[0:i3],               \
        vec_b[0:i3], parent_index[0:i3])        \
        if (_nt->compute_gpu) async(_nt->stream_id)
    for (int i = i2; i < i3; ++i) {
        #pragma acc atomic update
        vec_d[i] -= vec_b[i];
        #pragma acc atomic update
        vec_d[parent_index[i]] -= vec_a[i];
    }
    // clang-format on
}

/* for the fixed step method */
void* setup_tree_matrix_minimal(NrnThread* _nt) {
    nrn_rhs(_nt);
    nrn_lhs(_nt);
    // update_matrix_from_gpu(_nt);

    return nullptr;
}
}  // namespace coreneuron
