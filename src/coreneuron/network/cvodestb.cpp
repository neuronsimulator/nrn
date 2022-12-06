/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
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

    for (auto& net_buf_receive: corenrn.get_net_buf_receive()) {
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

#ifdef CORENEURON_ENABLE_GPU
    /* weight vectors could be updated (from INITIAL block of NET_RECEIVE, update those on GPU's */
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread* nt = nrn_threads + ith;
        double* weights = nt->weights;
        int n_weight = nt->n_weight;
        if (n_weight && nt->compute_gpu) {
            nrn_pragma_acc(update device(weights [0:n_weight]))
            nrn_pragma_omp(target update to(weights [0:n_weight]))
        }
    }
#endif
}

void nrn_play_init() {
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread* nt = nrn_threads + ith;
        for (int i = 0; i < nt->n_vecplay; ++i) {
            ((PlayRecord*) nt->_vecplay[i])->play_init();
        }
    }
}

void fixed_play_continuous(NrnThread* nt) {
    for (int i = 0; i < nt->n_vecplay; ++i) {
        ((PlayRecord*) nt->_vecplay[i])->continuous(nt->_t);
    }
}

}  // namespace coreneuron
