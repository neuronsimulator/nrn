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

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
// solver CVode stub to allow cvode as dll for mswindows version.

#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/utils/vrecitem.h"

#include "coreneuron/gpu/nrn_acc_manager.hpp"

namespace coreneuron {

// for fixed step thread
// check thresholds and deliver all (including binqueue) events
// up to t+dt/2
void deliver_net_events(NrnThread* nt) {
    if (net_cvode_instance) {
        net_cvode_instance->check_thresh(nt);
        net_cvode_instance->deliver_net_events(nt);
    }
}

// deliver events (but not binqueue)  up to nt->_t
void nrn_deliver_events(NrnThread* nt) {
    double tsav = nt->_t;
    if (net_cvode_instance) {
        net_cvode_instance->deliver_events(tsav, nt);
    }
    nt->_t = tsav;

    /*before executing on gpu, we have to update the NetReceiveBuffer_t on GPU */
    update_net_receive_buffer(nt);

    for (auto& net_buf_receive : corenrn.get_net_buf_receive()) {
        (*net_buf_receive.first)(nt);
    }
}

void clear_event_queue() {
    if (net_cvode_instance) {
        net_cvode_instance->clear_events();
    }
}

void init_net_events() {
    if (net_cvode_instance) {
        net_cvode_instance->init_events();
    }

#if defined(_OPENACC)
    /* weight vectors could be updated (from INITIAL block of NET_RECEIVE, update those on GPU's */
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread* nt = nrn_threads + ith;
        double* weights = nt->weights;
        int n_weight = nt->n_weight;
        if (n_weight) {
            // clang-format off
            #pragma acc update device(weights[0 : n_weight]) if (nt->compute_gpu)
            // clang-format on
        }
    }
#endif
}

void nrn_play_init() {
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread* nt = nrn_threads + ith;
        for (int i = 0; i < nt->n_vecplay; ++i) {
            ((PlayRecord*)nt->_vecplay[i])->play_init();
        }
    }
}

void fixed_play_continuous(NrnThread* nt) {
    for (int i = 0; i < nt->n_vecplay; ++i) {
        ((PlayRecord*)nt->_vecplay[i])->continuous(nt->_t);
    }
}

}  // namespace coreneuron

extern "C" int at_time(coreneuron::NrnThread* nt, double te) {
    double x = te - 1e-11;
    if (x <= nt->_t && x > (nt->_t - nt->_dt)) {
        return 1;
    }
    return 0;
}
