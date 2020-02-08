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

#include "coreneuron/nrnconf.h"
#include "coreneuron/network/netpar.hpp"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/sim/fast_imem.hpp"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/utils/profile/profiler_interface.h"
#include "coreneuron/coreneuron.hpp"

namespace coreneuron {

void nrn_finitialize(int setv, double v) {
    Instrumentor::phase_begin("finitialize");
    t = 0.;
    dt2thread(-1.);
    nrn_thread_table_check();
    clear_event_queue();
    nrn_spike_exchange_init();
#if VECTORIZE
    nrn_play_init(); /* Vector.play */
                     /// Play events should be executed before initializing events
    for (int i = 0; i < nrn_nthread; ++i) {
        nrn_deliver_events(nrn_threads + i); /* The play events at t=0 */
    }
    if (setv) {
        for (auto _nt = nrn_threads; _nt < nrn_threads + nrn_nthread; ++_nt) {
            double* vec_v = &(VEC_V(0));
// clang-format off
            #pragma acc parallel loop present(      \
                _nt[0:1], vec_v[0:_nt->end])        \
                if (_nt->compute_gpu)
            // clang-format on
            for (int i = 0; i < _nt->end; ++i) {
                vec_v[i] = v;
            }
        }
    }

    if (nrn_have_gaps) {
        nrnmpi_v_transfer();
        for (int i = 0; i < nrn_nthread; ++i) {
            nrnthread_v_transfer(nrn_threads + i);
        }
    }

    for (int i = 0; i < nrn_nthread; ++i) {
        nrn_ba(nrn_threads + i, BEFORE_INITIAL);
    }
    /* the INITIAL blocks are ordered so that mechanisms that write
       concentrations are after ions and before mechanisms that read
       concentrations.
    */
    /* the memblist list in NrnThread is already so ordered */
    for (int i = 0; i < nrn_nthread; ++i) {
        NrnThread* nt = nrn_threads + i;
        for (auto tml = nt->tml; tml; tml = tml->next) {
            mod_f_t s = corenrn.get_memb_func(tml->index).initialize;
            if (s) {
                (*s)(nt, tml->ml, tml->index);
            }
        }
    }
#endif

    init_net_events();
    for (int i = 0; i < nrn_nthread; ++i) {
        nrn_ba(nrn_threads + i, AFTER_INITIAL);
    }
    for (int i = 0; i < nrn_nthread; ++i) {
        nrn_deliver_events(nrn_threads + i); /* The INITIAL sent events at t=0 */
    }
    for (int i = 0; i < nrn_nthread; ++i) {
        setup_tree_matrix_minimal(nrn_threads + i);
        if (nrn_use_fast_imem) {
            nrn_calc_fast_imem(nrn_threads + i);
        }
    }
    for (int i = 0; i < nrn_nthread; ++i) {
        nrn_ba(nrn_threads + i, BEFORE_STEP);
    }
    for (int i = 0; i < nrn_nthread; ++i) {
        nrn_deliver_events(nrn_threads + i); /* The record events at t=0 */
    }
#if NRNMPI
    nrn_spike_exchange(nrn_threads);
#endif
    nrncore2nrn_send_init();
    for (int i = 0; i < nrn_nthread; ++i) {
        nrncore2nrn_send_values(nrn_threads + i);
    }
    Instrumentor::phase_end("finitialize");
}
}  // namespace coreneuron
