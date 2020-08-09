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
                          int*& netcon_srcgid);

namespace coreneuron {
void Phase1::read_file(FileHandler& F) {
    assert(!F.fail());
    int n_presyn = F.read_int();  /// Number of PreSyn-s in NrnThread nt
    int n_netcon = F.read_int();  /// Number of NetCon-s in NrnThread nt

    this->output_gids = F.read_vector<int>(n_presyn);
    this->netcon_srcgids = F.read_vector<int>(n_netcon);

    F.close();
}

void Phase1::read_direct(int thread_id) {
    int* output_gids;
    int* netcon_srcgid;
    int n_presyn;
    int n_netcon;

    // TODO : check error codes for NEURON - CoreNEURON communication
    int valid =
        (*nrn2core_get_dat1_)(thread_id, n_presyn, n_netcon, output_gids, netcon_srcgid);
    if (!valid) {
        return;
    }

    this->output_gids = std::vector<int>(output_gids, output_gids + n_presyn);
    delete[] output_gids;
    this->netcon_srcgids = std::vector<int>(netcon_srcgid, netcon_srcgid + n_netcon);
    delete[] netcon_srcgid;
}

void Phase1::shift_gids(int imult) {
    // maxgid is the maxgid to have the different multiple in the same gid space.
    int maxgid = 0x7fffffff / nrn_setup_multiple;
    // this value is the beginning of the new cluster of gids.
    // the correct cluster is choose with imult.
    int offset_gids = imult * maxgid;  // offset for each gid

    // offset the (non-negative) gids according to multiple
    // make sure everything fits into gid space.
    for (auto& gid: this->output_gids) {
        if (gid >= 0) {
            nrn_assert(gid < maxgid);
            gid += offset_gids;
        }
    }

    for (auto& srcgid: this->netcon_srcgids) {
        if (srcgid >= 0) {
            nrn_assert(srcgid < maxgid);
            srcgid += offset_gids;
        }
    }
}

void Phase1::add_extracon(NrnThread& nt, int imult) {
    int maxgid = 0x7fffffff / nrn_setup_multiple;
    if (nrn_setup_extracon <= 0) {
        return;
    }

    // very simplistic
    // Use this threads positive source gids - zz in nt.netcon order as the
    // source gids for extracon.
    // The edge cases are:
    // The 0th duplicate uses uses source gids for the last duplicate.
    // If there are fewer positive source gids than extracon, then keep
    // rotating through the nt.netcon .
    // If there are no positive source gids, use a source gid of -1.
    // Would not be difficult to modify so that random positive source was
    // used, and/or random connect to another duplicate.
    // Note that we increment the nt.n_netcon at the end of this function.
    int sidoffset = 0;  // how much to increment the corresponding positive gid
    // like ring connectivity
    if (imult > 0) {
        sidoffset = -maxgid;
    } else if (nrn_setup_multiple > 1) {
        sidoffset = (nrn_setup_multiple - 1) * maxgid;
    }
    // set up the extracon srcgid_
    int* nc_srcgid = netcon_srcgid[nt.id];
    int j = 0;  // rotate through the n_netcon netcon_srcgid
    for (int i = 0; i < nrn_setup_extracon; ++i) {
        int sid = -1;
        for (int k = 0; k < nt.n_netcon; ++k) {
            // potentially rotate j through the entire n_netcon but no further
            sid = nc_srcgid[j];
            j = (j + 1) % nt.n_netcon;
            if (sid >= 0) {
                break;
            }
        }
        if (sid < 0) {  // only connect to real cells.
            sid = -1;
        } else {
            sid += sidoffset;
        }
        nc_srcgid[nt.n_netcon + i] = sid;
    }
    // finally increment the n_netcon
    nt.n_netcon += nrn_setup_extracon;
}

#ifdef _OPENMP
void Phase1::populate(NrnThread& nt, int imult, OMP_Mutex& mut) {
#else
void Phase1::populate(NrnThread& nt, int imult) {
#endif
    nt.n_presyn = this->output_gids.size();
    nt.n_netcon = this->netcon_srcgids.size();

    shift_gids(imult);

    netcon_srcgid[nt.id] = new int[nt.n_netcon + nrn_setup_extracon];
    std::copy(this->netcon_srcgids.begin(), this->netcon_srcgids.end(),
              netcon_srcgid[nt.id]);

    nt.netcons = new NetCon[nt.n_netcon + nrn_setup_extracon];
    nt.presyns_helper = (PreSynHelper*)ecalloc_align(nt.n_presyn, sizeof(PreSynHelper));

    nt.presyns = new PreSyn[nt.n_presyn];
    PreSyn* ps = nt.presyns;
    /// go through all presyns
    for (auto& gid: this->output_gids) {
        if (gid == -1) {
            ++ps;
            continue;
        }

        {
#ifdef _OPENMP
            const std::lock_guard<OMP_Mutex> lock(mut);
#endif
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
                    hoc_execerror(
                        m,
                        "Setup all the output ports on this process before using them as input ports.");
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
        } // end of the mutex

        ++ps;
    }

    add_extracon(nt, imult);
}

}  // namespace coreneuron
