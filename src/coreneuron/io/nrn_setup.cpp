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

#include <algorithm>
#include <vector>
#include <map>
#include <cstring>
#include <mutex>

#include "coreneuron/apps/corenrn_parameters.hpp"
#include "coreneuron/nrnconf.h"
#include "coreneuron/utils/randoms/nrnran123.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/sim/fast_imem.hpp"
#include "coreneuron/network/multisend.hpp"
#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/utils/nrnmutdec.h"
#include "coreneuron/utils/memory.h"
#include "coreneuron/io/nrn_setup.hpp"
#include "coreneuron/network/partrans.hpp"
#include "coreneuron/io/nrn_checkpoint.hpp"
#include "coreneuron/permute/node_permute.h"
#include "coreneuron/permute/cellorder.hpp"
#include "coreneuron/io/nrnsection_mapping.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"
#include "coreneuron/io/phase1.hpp"
#include "coreneuron/io/phase2.hpp"
#include "coreneuron/io/mech_report.h"
#include "coreneuron/apps/corenrn_parameters.hpp"

// callbacks into nrn/src/nrniv/nrnbbcore_write.cpp
#include "coreneuron/sim/fast_imem.hpp"
#include "coreneuron/coreneuron.hpp"


/// --> Coreneuron
bool corenrn_embedded;
int corenrn_embedded_nthread;

void (*nrn2core_group_ids_)(int*);

extern "C" {
coreneuron::nrn_partrans::SetupTransferInfo*
    (*nrn2core_get_partrans_setup_info_)(int ngroup, int cn_nthread,
                                         size_t cn_sidt_size);
}

void (*nrn2core_get_trajectory_requests_)(int tid,
                                          int& bsize,
                                          int& n_pr,
                                          void**& vpr,
                                          int& n_trajec,
                                          int*& types,
                                          int*& indices,
                                          double**& pvars,
                                          double**& varrays);

void (*nrn2core_trajectory_values_)(int tid, int n_pr, void** vpr, double t);

void (*nrn2core_trajectory_return_)(int tid, int n_pr, int vecsz, void** vpr, double t);

int (*nrn2core_all_spike_vectors_return_)(std::vector<double>& spikevec, std::vector<int>& gidvec);

void (*nrn2core_all_weights_return_)(std::vector<double*>& weights);

// file format defined in cooperation with nrncore/src/nrniv/nrnbbcore_write.cpp
// single integers are ascii one per line. arrays are binary int or double
// Note that regardless of the gid contents of a group, since all gids are
// globally unique, a filename convention which involves the first gid
// from the group is adequate. Also note that balance is carried out from a
// per group perspective and launching a process consists of specifying
// a list of group ids (first gid of the group) for each process.
//
// <firstgid>_1.dat
// n_presyn, n_netcon
// output_gids (npresyn) with -(type+1000*index) for those acell with no gid
// netcon_srcgid (nnetcon) -(type+1000*index) refers to acell with no gid
//                         -1 means the netcon has no source (not implemented)
// Note that the negative gids are only thread unique and not process unique.
// We create a thread specific hash table for the negative gids for each thread
// when <firstgid>_1.dat is read and then destroy it after <firstgid>_2.dat
// is finished using it.  An earlier implementation which attempted to
// encode the thread number into the negative gid
// (i.e -ith - nth*(type +1000*index)) failed due to not large enough
// integer domain size.
// Note that for file transfer it is an error if a negative srcgid is
// not in the same thread as the target. This is because there it may
// not be the case that threads in a NEURON process end up on same process
// in CoreNEURON. NEURON will raise an error if this
// is the case. However, for direct memory transfer, it is allowed that
// a negative srcgid may be in a different thread than the target. So
// nrn2core_get_dat1 has a last arg netcon_negsrcgid_tid that specifies
// for the negative gids in netcon_srcgid (in that order) the source thread.
//
// <firstgid>_2.dat
// n_output n_real_output, nnode
// ndiam - 0 if no mechanism has dparam with diam semantics, or nnode
// nmech - includes artcell mechanisms
// for the nmech tml mechanisms
//   type, nodecount
// nidata, nvdata, nweight
// v_parent_index (nnode)
// actual_a, b, area, v (nnode)
// diam - if ndiam > 0. Note that only valid diam is for those nodes with diam semantics mechanisms
// for the nmech tml mechanisms
//   nodeindices (nodecount) but only if not an artificial cell
//   data (nodecount*param_size)
//   pdata (nodecount*dparam_size) but only if dparam_size > 0 on this side.
// output_vindex (n_presyn) >= 0 associated with voltages -(type+1000*index) for acell
// output_threshold (n_real_output)
// netcon_pnttype (nnetcon) <=0 if a NetCon does not have a target.
// netcon_pntindex (nnetcon)
// weights (nweight)
// delays (nnetcon)
// for the nmech tml mechanisms that have a nrn_bbcore_write method
//   type
//   icnt
//   dcnt
//   int array (number specified by the nodecount nrn_bbcore_write
//     to be intepreted by this side's nrn_bbcore_read method)
//   double array
// #VectorPlay_instances, for each of these instances
// 4 (VecPlayContinuousType)
// mtype
// index (from Memb_list.data)
// vecsize
// yvec
// tvec
//
// The critical issue requiring careful attention is that a coreneuron
// process reads many coreneuron thread files with a result that, although
// the conceptual
// total n_pre is the sum of all the n_presyn from each thread as is the
// total number of output_gid, the number of InputPreSyn instances must
// be computed here from a knowledge of all thread's netcon_srcgid after
// all thread's output_gids have been registered. We want to save the
// "individual allocation of many small objects" memory overhead by
// allocating a single InputPreSyn array for the entire process.
// For this reason cellgroup data are divided into two separate
// files with the first containing output_gids and netcon_srcgid which are
// stored in the nt.presyns array and nt.netcons array respectively
namespace coreneuron {
extern corenrn_parameters corenrn_param;

static OMP_Mutex mut;

static size_t model_size(void);

/// Vector of maps for negative presyns
std::vector<std::map<int, PreSyn*>> neg_gid2out;
/// Maps for ouput and input presyns
std::map<int, PreSyn*> gid2out;
std::map<int, InputPreSyn*> gid2in;

/// InputPreSyn.nc_index_ to + InputPreSyn.nc_cnt_ give the NetCon*
std::vector<NetCon*> netcon_in_presyn_order_;

/// Only for setup vector of netcon source gids
std::vector<int*> nrnthreads_netcon_srcgid;

/// If a nrnthreads_netcon_srcgid is negative, need to determine the thread when
/// in order to use the correct neg_gid2out[tid] map
std::vector<std::vector<int> > nrnthreads_netcon_negsrcgid_tid;

/* read files.dat file and distribute cellgroups to all mpi ranks */
void nrn_read_filesdat(int& ngrp, int*& grp, const char* filesdat) {
    patstimtype = nrn_get_mechtype("PatternStim");
    if (corenrn_embedded) {
        ngrp = corenrn_embedded_nthread;
        grp = new int[ngrp + 1];
        (*nrn2core_group_ids_)(grp);
        return;
    }

    FILE* fp = fopen(filesdat, "r");

    if (!fp) {
        nrn_fatal_error("No input file with nrnthreads, exiting...");
    }

    char version[256];
    nrn_assert(fscanf(fp, "%s\n", version) == 1);
    check_bbcore_write_version(version);

    int iNumFiles = 0;
    nrn_assert(fscanf(fp, "%d\n", &iNumFiles) == 1);

    // temporary strategem to figure out if model uses gap junctions while
    // being backward compatible
    if (iNumFiles == -1) {
        nrn_assert(fscanf(fp, "%d\n", &iNumFiles) == 1);
        nrn_have_gaps = true;
        if (nrnmpi_myid == 0) {
            printf("Model uses gap junctions\n");
        }
    }

    if (nrnmpi_numprocs > iNumFiles && nrnmpi_myid == 0) {
        printf(
            "Info : The number of input datasets are less than ranks, some ranks will be idle!\n");
    }

    ngrp = 0;
    grp = new int[iNumFiles / nrnmpi_numprocs + 1];

    // irerate over gids in files.dat
    for (int iNum = 0; iNum < iNumFiles; ++iNum) {
        int iFile;

        nrn_assert(fscanf(fp, "%d\n", &iFile) == 1);
        if ((iNum % nrnmpi_numprocs) == nrnmpi_myid) {
            grp[ngrp] = iFile;
            ngrp++;
        }
    }

    fclose(fp);
}

void netpar_tid_gid2ps(int tid, int gid, PreSyn** ps, InputPreSyn** psi) {
    /// for gid < 0 returns the PreSyn* in the thread (tid) specific map.
    *ps = nullptr;
    *psi = nullptr;

    if (gid >= 0) {
        auto gid2out_it = gid2out.find(gid);
        if (gid2out_it != gid2out.end()) {
            *ps = gid2out_it->second;
        } else {
            std::map<int, InputPreSyn*>::iterator gid2in_it;
            gid2in_it = gid2in.find(gid);
            if (gid2in_it != gid2in.end()) {
                *psi = gid2in_it->second;
            }
        }
    } else {
        auto gid2out_it = neg_gid2out[tid].find(gid);
        if (gid2out_it != neg_gid2out[tid].end()) {
            *ps = gid2out_it->second;
        }
    }
}

void determine_inputpresyn() {
    // allocate the process wide InputPreSyn array
    // all the output_gid have been registered and associated with PreSyn.
    // now count the needed InputPreSyn by filling the netpar::gid2in map
    gid2in.clear();

    // now have to fill the new table
    // do not need to worry about negative gid overlap since only use
    // it to search for PreSyn in this thread.

    std::vector<InputPreSyn*> inputpresyn_;

    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        // associate gid with InputPreSyn and increase PreSyn and InputPreSyn count
        nt.n_input_presyn = 0;
        // if single thread or file transfer then definitely empty.
        std::vector<int>& negsrcgid_tid = nrnthreads_netcon_negsrcgid_tid[ith];
        size_t i_tid = 0;
        for (int i = 0; i < nt.n_netcon; ++i) {
            int gid = nrnthreads_netcon_srcgid[ith][i];
            if (gid >= 0) {
                /// If PreSyn or InputPreSyn is already in the map
                auto gid2out_it = gid2out.find(gid);
                if (gid2out_it != gid2out.end()) {
                    /// Increase PreSyn count
                    ++gid2out_it->second->nc_cnt_;
                    continue;
                }
                auto gid2in_it = gid2in.find(gid);
                if (gid2in_it != gid2in.end()) {
                    /// Increase InputPreSyn count
                    ++gid2in_it->second->nc_cnt_;
                    continue;
                }

                /// Create InputPreSyn and increase its count
                InputPreSyn* psi = new InputPreSyn;
                ++psi->nc_cnt_;
                gid2in[gid] = psi;
                inputpresyn_.push_back(psi);
                ++nt.n_input_presyn;
            } else {
                int tid = nt.id;
                if (!negsrcgid_tid.empty()) {
                    tid = negsrcgid_tid[i_tid++];
                }
                auto gid2out_it = neg_gid2out[tid].find(gid);
                if (gid2out_it != neg_gid2out[tid].end()) {
                    /// Increase negative PreSyn count
                    ++gid2out_it->second->nc_cnt_;
                }
            }
        }
    }

    // now, we can opportunistically create the NetCon* pointer array
    // to save some memory overhead for
    // "large number of small array allocation" by
    // counting the number of NetCons each PreSyn and InputPreSyn point to.
    // Conceivably the nt.netcons could become a process global array
    // in which case the NetCon* pointer array could become an integer index
    // array. More speculatively, the index array could be eliminated itself
    // if the process global NetCon array were ordered properly but that
    // would interleave NetCon from different threads. Not a problem for
    // serial threads but the reordering would propagate to nt.pntprocs
    // if the NetCon data pointers are also replaced by integer indices.

    // First, allocate the pointer array.
    int n_nc = 0;
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        n_nc += nrn_threads[ith].n_netcon;
    }
    netcon_in_presyn_order_.resize(n_nc);
    n_nc = 0;

    // fill the indices with the offset values and reset the nc_cnt_
    // such that we use the nc_cnt_ in the following loop to assign the NetCon
    // to the right place
    // for PreSyn
    int offset = 0;
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        for (int i = 0; i < nt.n_presyn; ++i) {
            PreSyn& ps = nt.presyns[i];
            ps.nc_index_ = offset;
            offset += ps.nc_cnt_;
            ps.nc_cnt_ = 0;
        }
    }
    // for InputPreSyn
    for (auto psi: inputpresyn_) {
        psi->nc_index_ = offset;
        offset += psi->nc_cnt_;
        psi->nc_cnt_ = 0;
    }

    inputpresyn_.clear();

    // with gid to InputPreSyn and PreSyn maps we can setup the multisend
    // target lists.
    if (use_multisend_) {
#if NRN_MULTISEND
        nrn_multisend_setup();
#endif
    }

    // fill the netcon_in_presyn_order and recompute nc_cnt_
    // note that not all netcon_in_presyn will be filled if there are netcon
    // with no presyn (ie. nrnthreads_netcon_srcgid[nt.id][i] = -1) but that is ok since they are
    // only used via ps.nc_index_ and ps.nc_cnt_;
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        // if single thread or file transfer then definitely empty.
        std::vector<int>& negsrcgid_tid = nrnthreads_netcon_negsrcgid_tid[ith];
        size_t i_tid = 0;
        for (int i = 0; i < nt.n_netcon; ++i) {
            NetCon* nc = nt.netcons + i;
            int gid = nrnthreads_netcon_srcgid[ith][i];
            int tid = ith;
            if (!negsrcgid_tid.empty() && gid < -1) {
              tid = negsrcgid_tid[i_tid++];
            }
            PreSyn* ps;
            InputPreSyn* psi;
            netpar_tid_gid2ps(tid, gid, &ps, &psi);
            if (ps) {
                netcon_in_presyn_order_[ps->nc_index_ + ps->nc_cnt_] = nc;
                ++ps->nc_cnt_;
                ++n_nc;
            } else if (psi) {
                netcon_in_presyn_order_[psi->nc_index_ + psi->nc_cnt_] = nc;
                ++psi->nc_cnt_;
                ++n_nc;
            }
        }
    }

    /// Resize the vector to its actual size of the netcons put in it
    netcon_in_presyn_order_.resize(n_nc);
}

/// Clean up
void nrn_setup_cleanup() {
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        if (nrnthreads_netcon_srcgid[ith])
            delete[] nrnthreads_netcon_srcgid[ith];
    }
    nrnthreads_netcon_srcgid.clear();
    nrnthreads_netcon_negsrcgid_tid.clear();
    neg_gid2out.clear();
}

void nrn_setup(const char* filesdat,
               bool is_mapping_needed,
               bool run_setup_cleanup,
               const char* datpath,
               const char* restore_path,
               double* mindelay) {
    double time = nrn_wtime();

    int ngroup;
    int* gidgroups;
    nrn_read_filesdat(ngroup, gidgroups, filesdat);
    UserParams userParams(ngroup,
                          gidgroups,
                          datpath,
                          strlen(restore_path) == 0 ? datpath : restore_path);
   

    // temporary bug work around. If any process has multiple threads, no
    // process can have a single thread. So, for now, if one thread, make two.
    // Fortunately, empty threads work fine.
    // Allocate NrnThread* nrn_threads of size ngroup (minimum 2)
    // Note that rank with 0 dataset/cellgroup works fine
    nrn_threads_create(userParams.ngroup <= 1 ? 2 : userParams.ngroup);

    // from nrn_has_net_event create pnttype2presyn for use in phase2.
    auto& memb_func = corenrn.get_memb_funcs();
    auto& pnttype2presyn = corenrn.get_pnttype2presyn();
    auto& nrn_has_net_event_ = corenrn.get_has_net_event();
    pnttype2presyn.clear();
    pnttype2presyn.resize(memb_func.size(), -1);
    for (size_t i = 0; i < nrn_has_net_event_.size(); ++i) {
        pnttype2presyn[nrn_has_net_event_[i]] = i;
    }

    nrnthread_chkpnt = new NrnThreadChkpnt[nrn_nthread];

    if (nrn_nthread > 1) {
        // NetCvode construction assumed one thread. Need nrn_nthread instances
        // of NetCvodeThreadData. Here since possible checkpoint restore of
        // tqueue at end of phase2.
        nrn_p_construct();
    }

    if (use_solve_interleave) {
        create_interleave_info();
    }

    /// Reserve vector of maps of size ngroup for negative gid-s
    /// std::vector< std::map<int, PreSyn*> > neg_gid2out;
    neg_gid2out.resize(userParams.ngroup);

    // bug fix. gid2out is cumulative over all threads and so do not
    // know how many there are til after phase1
    // A process's complete set of output gids and allocation of each thread's
    // nt.presyns and nt.netcons arrays.
    // Generates the gid2out map which is needed
    // to later count the required number of InputPreSyn
    /// gid2out - map of output presyn-s
    /// std::map<int, PreSyn*> gid2out;
    gid2out.clear();

    nrnthreads_netcon_srcgid.resize(nrn_nthread);
    for (int i = 0; i < nrn_nthread; ++i)
        nrnthreads_netcon_srcgid[i] = nullptr;

    // Gap junctions used to be done first in the sense of reading files
    // and calling gap_mpi_setup. But during phase2, gap_thread_setup and
    // gap_indices_permute were called after NrnThread.data was in its final
    // layout and mechanism permutation was determined. This is no longer
    // ideal as it necessitates keeping setup_info_ in existence to the end
    // of phase2.  So gap junction setup is deferred to after phase2.

    nrnthreads_netcon_negsrcgid_tid.resize(nrn_nthread);
    if (!corenrn_embedded) {
        coreneuron::phase_wrapper<coreneuron::phase::one>(userParams);
    } else {
        nrn_multithread_job([](NrnThread* n) {
            Phase1 p1; 
            p1.read_direct(n->id);
            NrnThread& nt = *n;
            p1.populate(nt, mut);
        });
    }

    // from the gid2out map and the nrnthreads_netcon_srcgid array,
    // fill the gid2in, and from the number of entries,
    // allocate the process wide InputPreSyn array
    determine_inputpresyn();

    // read the rest of the gidgroup's data and complete the setup for each
    // thread.
    /* nrn_multithread_job supports serial, pthread, and openmp. */
    coreneuron::phase_wrapper<coreneuron::phase::two>(userParams, corenrn_embedded);

    // gap junctions
    // Gaps are done after phase2, in order to use layout and permutation
    // information via calls to stdindex2ptr.
    if (nrn_have_gaps) {
        nrn_partrans::transfer_thread_data_ = new nrn_partrans::TransferThreadData[nrn_nthread];
        if (!corenrn_embedded) {
            nrn_partrans::setup_info_ = new nrn_partrans::SetupTransferInfo[
                nrn_nthread];
            coreneuron::phase_wrapper<coreneuron::gap>(userParams);
        } else {
            nrn_partrans::setup_info_ = (*nrn2core_get_partrans_setup_info_)(
                userParams.ngroup, nrn_nthread, sizeof(nrn_partrans::sgid_t));
        }

        nrn_multithread_job(nrn_partrans::gap_data_indices_setup);
        nrn_partrans::gap_mpi_setup(userParams.ngroup);

        // Whether allocated in NEURON or here, delete here.
        delete [] nrn_partrans::setup_info_;
        nrn_partrans::setup_info_ = nullptr;
    }

    if (is_mapping_needed)
        coreneuron::phase_wrapper<coreneuron::phase::three>(userParams);

    *mindelay = set_mindelay(*mindelay);

    if (run_setup_cleanup)  // if run_setup_cleanup==false, user must call nrn_setup_cleanup() later
        nrn_setup_cleanup();

#if INTERLEAVE_DEBUG
    mk_cell_indices();
#endif

    /// Allocate memory for fast_imem calculation
    nrn_fast_imem_alloc();

    /// Generally, tables depend on a few parameters. And if those parameters change,
    /// then the table needs to be recomputed. This is obviously important in NEURON
    /// since the user can change those parameters at any time. However, there is no
    /// c example for CoreNEURON so can't see what it looks like in that context.
    /// Boils down to setting up a function pointer of the function _check_table_thread(),
    /// which is only executed by StochKV.c.
    nrn_mk_table_check();  // was done in nrn_thread_memblist_setup in multicore.c

    model_size();
    delete[] userParams.gidgroups;

    if (nrnmpi_myid == 0 && !corenrn_param.is_quiet()) {
        printf(" Setup Done   : %.2lf seconds \n", nrn_wtime() - time);
    }
    if (corenrn_param.count_mechs) {
        write_mech_report();
    }
}

void setup_ThreadData(NrnThread& nt) {
    for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
        Memb_func& mf = corenrn.get_memb_func(tml->index);
        Memb_list* ml = tml->ml;
        if (mf.thread_size_) {
            ml->_thread = (ThreadDatum*)ecalloc_align(mf.thread_size_, sizeof(ThreadDatum));
            if (mf.thread_mem_init_) {
                {
                    const std::lock_guard<OMP_Mutex> lock(mut);
                    (*mf.thread_mem_init_)(ml->_thread);
                }
            }
        } else {
            ml->_thread = nullptr;
        }
    }
}

void read_phasegap(NrnThread& nt, UserParams& userParams) {
    auto& si = nrn_partrans::setup_info_[nt.id];
    size_t ntar = 0;
    size_t nsrc = 0;

    auto& F = userParams.file_reader[nt.id];
    if (F.fail()) {
        return;
    }

    int chkpntsave = F.checkpoint();
    F.checkpoint(0);

    int sidt_size = F.read_int();
    assert(sidt_size == int(sizeof(nrn_partrans::sgid_t)));
    ntar = size_t(F.read_int());
    nsrc = size_t(F.read_int());

    si.src_sid.resize(nsrc);
    si.src_type.resize(nsrc);
    si.src_index.resize(nsrc);
    if (nsrc) {
        F.read_array<nrn_partrans::sgid_t>(si.src_sid.data(), nsrc);
        F.read_array<int>(si.src_type.data(), nsrc);
        F.read_array<int>(si.src_index.data(), nsrc);
    }

    si.tar_sid.resize(ntar);
    si.tar_type.resize(ntar);
    si.tar_index.resize(ntar);
    if (ntar) {
        F.read_array<nrn_partrans::sgid_t>(si.tar_sid.data(), ntar);
        F.read_array<int>(si.tar_type.data(), ntar);
        F.read_array<int>(si.tar_index.data(), ntar);
    }

#if DEBUG
  printf("%d read_phasegap tid=%d nsrc=%d ntar=%d\n",
    nrnmpi_myid, nt.id, nsrc, ntar);
  for (int i=0; i < si.nsrc; ++i) {
    printf("src %z %d %d\n", size_t(si.src_sid[i]), si.src_type[i], si.src_index[i]);
  }
  for (int i=0; i <si.ntar; ++i) {
    printf("tar %z %d %d\n", size_t(si.src_sid[i]), si.src_type[i], si.src_index[i]);
  }
#endif
}

// This function is related to nrn_dblpntr2nrncore in Neuron to determine which values should
// be transferred from CoreNeuron. Types correspond to the value to be transferred based on
// mech_type enum or non-artificial cell mechanisms.
// take into account alignment, layout, permutation
// only voltage, i_membrane_ or mechanism data index allowed. (mtype 0 means time)
double* stdindex2ptr(int mtype, int index, NrnThread& nt) {
    if (mtype == voltage) {  // voltage
        int v0 = nt._actual_v - nt._data;
        int ix = index;  // relative to _actual_v
        nrn_assert((ix >= 0) && (ix < nt.end));
        if (nt._permute) {
            node_permute(&ix, 1, nt._permute);
        }
        return nt._data + (v0 + ix);                // relative to nt._data
    } else if (mtype == i_membrane_) {              // membrane current from fast_imem calculation
            int i_mem = nt.nrn_fast_imem->nrn_sav_rhs - nt._data;
            int ix = index;  // relative to nrn_fast_imem->nrn_sav_rhs
            nrn_assert((ix >= 0) && (ix < nt.end));
            if (nt._permute) {
                node_permute(&ix, 1, nt._permute);
            }
            return nt._data + (i_mem + ix);         // relative to nt._data
    } else if (mtype > 0 && mtype < corenrn.get_memb_funcs().size()) {  //
        Memb_list* ml = nt._ml_list[mtype];
        nrn_assert(ml);
        int ix = nrn_param_layout(index, mtype, ml);
        if (ml->_permute) {
            ix = nrn_index_permute(ix, mtype, ml);
        }
        return ml->data + ix;
    } else if (mtype == 0) {  // time
        return &nt._t;
    } else {
        printf("stdindex2ptr does not handle mtype=%d\n", mtype);
        nrn_assert(0);
    }
    return nullptr;
}

// from i to (icnt, isz)
void nrn_inverse_i_layout(int i, int& icnt, int cnt, int& isz, int sz, int layout) {
    if (layout == 1) {
        icnt = i / sz;
        isz = i % sz;
    } else if (layout == 0) {
        int padded_cnt = nrn_soa_padded_size(cnt, layout);
        icnt = i % padded_cnt;
        isz = i / padded_cnt;
    } else {
        assert(0);
    }
}

/**
 * Cleanup global ion map created during mechanism registration
 *
 * In case of coreneuron standalone execution nrn_ion_global_map
 * can be deleted at the end of execution. But in case embedded
 * run via neuron, mechanisms are registered only once i.e. during
 * first call to coreneuron. This is why we call cleanup only in
 * case of standalone coreneuron execution via nrniv-core or
 * special-core.
 *
 * @todo coreneuron should have finalise callback which can be
 * called from NEURON for final memory cleanup including global
 * state like registered mechanisms and ions map.
 */
void nrn_cleanup_ion_map() {
    for (int i = 0; i < nrn_ion_global_map_size; i++) {
        free_memory(nrn_ion_global_map[i]);
    }
    free_memory(nrn_ion_global_map);
    nrn_ion_global_map = nullptr;
    nrn_ion_global_map_size = 0;
}

/* nrn_threads_free() presumes all NrnThread and NrnThreadMembList data is
 * allocated with malloc(). This is not the case here, so let's try and fix
 * things up first. */

void nrn_cleanup() {
    clear_event_queue();  // delete left-over TQItem
    gid2in.clear();
    gid2out.clear();

    // clean nrnthread_chkpnt
    if (nrnthread_chkpnt) {
        delete[] nrnthread_chkpnt;
        nrnthread_chkpnt = nullptr;
    }

    // clean NrnThreads
    for (int it = 0; it < nrn_nthread; ++it) {
        NrnThread* nt = nrn_threads + it;
        NrnThreadMembList* next_tml = nullptr;
        delete_trajectory_requests(*nt);
        for (NrnThreadMembList* tml = nt->tml; tml; tml = next_tml) {
            Memb_list* ml = tml->ml;

            ml->data = nullptr;  // this was pointing into memory owned by nt
            free_memory(ml->pdata);
            ml->pdata = nullptr;
            free_memory(ml->nodeindices);
            ml->nodeindices = nullptr;
            if (ml->_permute) {
                delete[] ml->_permute;
                ml->_permute = nullptr;
            }

            if (ml->_thread) {
                free_memory(ml->_thread);
                ml->_thread = nullptr;
            }

            NetReceiveBuffer_t* nrb = ml->_net_receive_buffer;
            if (nrb) {
                if (nrb->_size) {
                    free_memory(nrb->_pnt_index);
                    free_memory(nrb->_weight_index);
                    free_memory(nrb->_nrb_t);
                    free_memory(nrb->_nrb_flag);
                    free_memory(nrb->_displ);
                    free_memory(nrb->_nrb_index);
                }
                free_memory(nrb);
            }

            NetSendBuffer_t* nsb = ml->_net_send_buffer;
            if (nsb) {
                delete nsb;
            }

            if (tml->dependencies)
                free(tml->dependencies);

            next_tml = tml->next;
            free_memory(tml->ml);
            free_memory(tml);
        }

        nt->_actual_rhs = nullptr;
        nt->_actual_d = nullptr;
        nt->_actual_a = nullptr;
        nt->_actual_b = nullptr;

        free_memory(nt->_v_parent_index);
        nt->_v_parent_index = nullptr;

        free_memory(nt->_data);
        nt->_data = nullptr;

        free(nt->_idata);
        nt->_idata = nullptr;

        free_memory(nt->_vdata);
        nt->_vdata = nullptr;

        if (nt->_permute) {
            delete[] nt->_permute;
            nt->_permute = nullptr;
        }

        if (nt->presyns_helper) {
            free_memory(nt->presyns_helper);
            nt->presyns_helper = nullptr;
        }

        if (nt->pntprocs) {
            free_memory(nt->pntprocs);
            nt->pntprocs = nullptr;
        }

        if (nt->presyns) {
            delete[] nt->presyns;
            nt->presyns = nullptr;
        }

        if (nt->pnt2presyn_ix) {
            for (size_t i = 0; i < corenrn.get_has_net_event().size(); ++i) {
                if (nt->pnt2presyn_ix[i]) {
                    free(nt->pnt2presyn_ix[i]);
                }
            }
            free_memory(nt->pnt2presyn_ix);
        }

        if (nt->netcons) {
            delete[] nt->netcons;
            nt->netcons = nullptr;
        }

        if (nt->weights) {
            free_memory(nt->weights);
            nt->weights = nullptr;
        }

        if (nt->_shadow_rhs) {
            free_memory(nt->_shadow_rhs);
            nt->_shadow_rhs = nullptr;
        }

        if (nt->_shadow_d) {
            free_memory(nt->_shadow_d);
            nt->_shadow_d = nullptr;
        }

        if (nt->_net_send_buffer_size) {
            free_memory(nt->_net_send_buffer);
            nt->_net_send_buffer = nullptr;
            nt->_net_send_buffer_size = 0;
        }

        if (nt->_watch_types) {
            free(nt->_watch_types);
            nt->_watch_types = nullptr;
        }

        // mapping information is available only for non-empty NrnThread
        if (nt->mapping && nt->ncell) {
            delete ((NrnThreadMappingInfo*)nt->mapping);
        }

        free_memory(nt->_ml_list);

        if (nt->nrn_fast_imem) {
            fast_imem_free();
        }
    }

#if NRN_MULTISEND
    nrn_multisend_cleanup();
#endif

    netcon_in_presyn_order_.clear();

    nrn_threads_free();

    if (!corenrn.get_pnttype2presyn().empty()) {
        corenrn.get_pnttype2presyn().clear();
    }

    destroy_interleave_info();

    nrn_partrans::gap_cleanup();
}

void delete_trajectory_requests(NrnThread& nt) {
    if (nt.trajec_requests) {
        TrajectoryRequests* tr = nt.trajec_requests;
        if (tr->n_trajec) {
            delete[] tr->vpr;
            if (tr->scatter) {
                delete[] tr->scatter;
            }
            if (tr->varrays) {
                delete[] tr->varrays;
            }
            delete[] tr->gather;
        }
        delete nt.trajec_requests;
        nt.trajec_requests = nullptr;
    }
}

void read_phase1(NrnThread& nt, UserParams& userParams) {
    Phase1 p1;
    p1.read_file(userParams.file_reader[nt.id]);
    
    // Protect gid2in, gid2out and neg_gid2out
    p1.populate(nt, mut);
}

void read_phase2(NrnThread& nt, UserParams& userParams) {
    Phase2 p2;
    if (corenrn_embedded) {
        p2.read_direct(nt.id, nt);
    } else {
        p2.read_file(userParams.file_reader[nt.id], nt);
    }
    p2.populate(nt, userParams);
}

/** read mapping information for neurons */
void read_phase3(NrnThread& nt, UserParams& userParams) {
    /** restore checkpoint state (before restoring queue items */
    auto& F = userParams.file_reader[nt.id];
    F.restore_checkpoint();

    /** mapping information for all neurons in single NrnThread */
    NrnThreadMappingInfo* ntmapping = new NrnThreadMappingInfo();

    int count = 0;

    F.read_mapping_cell_count(&count);

    /** number of cells in mapping file should equal to cells in NrnThread */
    nrn_assert(count == nt.ncell);

    /** for every neuron */
    for (int i = 0; i < nt.ncell; i++) {
        int gid, nsec, nseg, nseclist;

        // read counts
        F.read_mapping_count(&gid, &nsec, &nseg, &nseclist);

        CellMapping* cmap = new CellMapping(gid);

        // read section-segment mapping for every section list
        for (int j = 0; j < nseclist; j++) {
            SecMapping* smap = new SecMapping();
            F.read_mapping_info(smap);
            cmap->add_sec_map(smap);
        }

        ntmapping->add_cell_mapping(cmap);
    }

    // make number #cells match with mapping size
    nrn_assert((int)ntmapping->size() == nt.ncell);

    // set pointer in NrnThread
    nt.mapping = (void*)ntmapping;
}

static size_t memb_list_size(NrnThreadMembList* tml) {
    size_t sz_ntml = sizeof(NrnThreadMembList);
    size_t sz_ml = sizeof(Memb_list);
    size_t szi = sizeof(int);
    size_t nbyte = sz_ntml + sz_ml;
    nbyte += tml->ml->nodecount * szi;
    nbyte += corenrn.get_prop_dparam_size()[tml->index] * tml->ml->nodecount * sizeof(Datum);
#ifdef DEBUG
    int i = tml->index;
    printf("%s %d psize=%d ppsize=%d cnt=%d nbyte=%ld\n", corenrn.get_memb_func(i).sym, i,
           corenrn.get_prop_param_size()[i],
           corenrn.get_prop_dparam_size()[i], tml->ml->nodecount, nbyte);
#endif
    return nbyte;
}

/// Approximate count of number of bytes for the gid2out map
size_t output_presyn_size(void) {
    if (gid2out.empty()) {
        return 0;
    }
    size_t nbyte =
        sizeof(gid2out) + sizeof(int) * gid2out.size() + sizeof(PreSyn*) * gid2out.size();
#ifdef DEBUG
    printf(" gid2out table bytes=~%ld size=%d\n", nbyte, gid2out.size());
#endif
    return nbyte;
}

size_t input_presyn_size(void) {
    if (gid2in.empty()) {
        return 0;
    }
    size_t nbyte =
        sizeof(gid2in) + sizeof(int) * gid2in.size() + sizeof(InputPreSyn*) * gid2in.size();
#ifdef DEBUG
    printf(" gid2in table bytes=~%ld size=%d\n", nbyte, gid2in.size());
#endif
    return nbyte;
}

size_t model_size(void) {
    size_t nbyte = 0;
    size_t szd = sizeof(double);
    size_t szi = sizeof(int);
    size_t szv = sizeof(void*);
    size_t sz_th = sizeof(NrnThread);
    size_t sz_ps = sizeof(PreSyn);
    size_t sz_psi = sizeof(InputPreSyn);
    size_t sz_nc = sizeof(NetCon);
    size_t sz_pp = sizeof(Point_process);
    size_t nccnt = 0;

    for (int i = 0; i < nrn_nthread; ++i) {
        NrnThread& nt = nrn_threads[i];
        size_t nb_nt = 0;  // per thread
        nccnt += nt.n_netcon;

        // Memb_list size
        int nmech = 0;
        for (auto tml = nt.tml; tml; tml = tml->next) {
            nb_nt += memb_list_size(tml);
            ++nmech;
        }

        // basic thread size includes mechanism data and G*V=I matrix
        nb_nt += sz_th;
        nb_nt += nt._ndata * szd + nt._nidata * szi + nt._nvdata * szv;
        nb_nt += nt.end * szi;  // _v_parent_index

#ifdef DEBUG
        printf("ncell=%d end=%d nmech=%d\n", nt.ncell, nt.end, nmech);
        printf("ndata=%ld nidata=%ld nvdata=%ld\n", nt._ndata, nt._nidata, nt._nvdata);
        printf("nbyte so far %ld\n", nb_nt);
        printf("n_presyn = %d sz=%ld nbyte=%ld\n", nt.n_presyn, sz_ps, nt.n_presyn * sz_ps);
        printf("n_input_presyn = %d sz=%ld nbyte=%ld\n", nt.n_input_presyn, sz_psi,
               nt.n_input_presyn * sz_psi);
        printf("n_pntproc=%d sz=%ld nbyte=%ld\n", nt.n_pntproc, sz_pp, nt.n_pntproc * sz_pp);
        printf("n_netcon=%d sz=%ld nbyte=%ld\n", nt.n_netcon, sz_nc, nt.n_netcon * sz_nc);
        printf("n_weight = %d\n", nt.n_weight);
#endif

        // spike handling
        nb_nt += nt.n_pntproc * sz_pp + nt.n_netcon * sz_nc + nt.n_presyn * sz_ps +
                 nt.n_input_presyn * sz_psi + nt.n_weight * szd;
        nbyte += nb_nt;
#ifdef DEBUG
        printf("%d thread %d total bytes %ld\n", nrnmpi_myid, i, nb_nt);
#endif
    }

#ifdef DEBUG
    printf("%d netcon pointers %ld  nbyte=%ld\n", nrnmpi_myid, nccnt, nccnt * sizeof(NetCon*));
#endif
    nbyte += nccnt * sizeof(NetCon*);
    nbyte += output_presyn_size();
    nbyte += input_presyn_size();

#ifdef DEBUG
    printf("nrnran123 size=%ld cnt=%ld nbyte=%ld\n", nrnran123_state_size(),
           nrnran123_instance_count(), nrnran123_instance_count() * nrnran123_state_size());
#endif

    nbyte += nrnran123_instance_count() * nrnran123_state_size();

#ifdef DEBUG
    printf("%d total bytes %ld\n", nrnmpi_myid, nbyte);
#endif

    return nbyte;
}

}  // namespace coreneuron
