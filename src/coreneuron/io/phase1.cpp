/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <cassert>
#include <mutex>

#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/io/phase1.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"

int (*nrn2core_get_dat1_)(int tid,
                          int& n_presyn,
                          int& n_netcon,
                          int*& output_gid,
                          int*& netcon_srcgid,
                          std::vector<int>& netcon_negsrcgid_tid);

namespace coreneuron {
void Phase1::read_file(FileHandler& F) {
    assert(!F.fail());
    int n_presyn = F.read_int();  /// Number of PreSyn-s in NrnThread nt
    int n_netcon = F.read_int();  /// Number of NetCon-s in NrnThread nt

    this->output_gids = F.read_vector<int>(n_presyn);
    this->netcon_srcgids = F.read_vector<int>(n_netcon);
    // For file mode transfer, it is not allowed that negative gids exist
    // in different threads. So this->netcon_tids remains clear.

    F.close();
}

void Phase1::read_direct(int thread_id) {
    int* output_gids;
    int* netcon_srcgid;
    int n_presyn;
    int n_netcon;

    // TODO : check error codes for NEURON - CoreNEURON communication
    int valid = (*nrn2core_get_dat1_)(
        thread_id, n_presyn, n_netcon, output_gids, netcon_srcgid, this->netcon_negsrcgid_tid);
    if (!valid) {
        return;
    }

    this->output_gids = std::vector<int>(output_gids, output_gids + n_presyn);
    delete[] output_gids;
    this->netcon_srcgids = std::vector<int>(netcon_srcgid, netcon_srcgid + n_netcon);
    delete[] netcon_srcgid;
}

void Phase1::populate(NrnThread& nt, OMP_Mutex& mut) {
    nt.n_presyn = this->output_gids.size();
    nt.n_netcon = this->netcon_srcgids.size();

    nrnthreads_netcon_srcgid[nt.id] = new int[nt.n_netcon];
    std::copy(this->netcon_srcgids.begin(),
              this->netcon_srcgids.end(),
              nrnthreads_netcon_srcgid[nt.id]);

    // netcon_negsrcgid_tid is empty if file transfer or single thread
    coreneuron::nrnthreads_netcon_negsrcgid_tid[nt.id] = this->netcon_negsrcgid_tid;

    nt.netcons = new NetCon[nt.n_netcon];
    nt.presyns_helper = (PreSynHelper*) ecalloc_align(nt.n_presyn, sizeof(PreSynHelper));

    nt.presyns = new PreSyn[nt.n_presyn];
    PreSyn* ps = nt.presyns;
    /// go through all presyns
    for (auto& gid: this->output_gids) {
        if (gid == -1) {
            ++ps;
            continue;
        }

        {
            const std::lock_guard<OMP_Mutex> lock(mut);
            // Note that the negative (type, index)
            // coded information goes into the neg_gid2out[tid] hash table.
            // See netpar.cpp for the netpar_tid_... function implementations.
            // Both that table and the process wide gid2out table can be deleted
            // before the end of setup

            /// Put gid into the gid2out hash table with correspondent output PreSyn
            /// Or to the negative PreSyn map
            if (gid >= 0) {
                char m[200];
                if (gid2in.find(gid) != gid2in.end()) {
                    sprintf(m, "gid=%d already exists as an input port", gid);
                    hoc_execerror(m,
                                  "Setup all the output ports on this process before using them as "
                                  "input ports.");
                }
                if (gid2out.find(gid) != gid2out.end()) {
                    sprintf(m, "gid=%d already exists on this process as an output port", gid);
                    hoc_execerror(m, 0);
                }
                ps->gid_ = gid;
                ps->output_index_ = gid;
                gid2out[gid] = ps;
            } else {
                nrn_assert(neg_gid2out[nt.id].find(gid) == neg_gid2out[nt.id].end());
                ps->output_index_ = -1;
                neg_gid2out[nt.id][gid] = ps;
            }
        }  // end of the mutex

        ++ps;
    }
}

}  // namespace coreneuron
