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
#include "coreneuron/utils/randoms/nrnran123.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <map>
#include <cstring>
#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrnoc/fast_imem.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrniv/vrecitem.h"
#include "coreneuron/nrniv/multisend.h"
#include "coreneuron/utils/sdprintf.h"
#include "coreneuron/nrniv/nrn_assert.h"
#include "coreneuron/nrniv/nrnmutdec.h"
#include "coreneuron/nrniv/memory.h"
#include "coreneuron/nrniv/nrn_setup.h"
#include "coreneuron/nrniv/partrans.h"
#include "coreneuron/nrniv/nrnoptarg.h"
#include "coreneuron/nrniv/nrn_checkpoint.h"
#include "coreneuron/nrniv/node_permute.h"
#include "coreneuron/nrniv/cellorder.h"
#include "coreneuron/utils/reports/nrnsection_mapping.h"

// callbacks into nrn/src/nrniv/nrnbbcore_write.cpp
#include "coreneuron/nrnoc/fast_imem.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrniv/nrn2core_direct.h"
#include "coreneuron/coreneuron.hpp"


/// --> Coreneuron
int corenrn_embedded;
int corenrn_embedded_nthread;

void (*nrn2core_group_ids_)(int*);

void (*nrn2core_get_partrans_setup_info_)(int tid,
                                          int& ntar,
                                          int& nsrc,
                                          int& type,
                                          int& ix_vpre,
                                          int*& sid_target,
                                          int*& sid_src,
                                          int*& v_indices);

int (*nrn2core_get_dat1_)(int tid,
                          int& n_presyn,
                          int& n_netcon,
                          int*& output_gid,
                          int*& netcon_srcgid);

int (*nrn2core_get_dat2_1_)(int tid,
                            int& ngid,
                            int& n_real_gid,
                            int& nnode,
                            int& ndiam,
                            int& nmech,
                            int*& tml_index,
                            int*& ml_nodecount,
                            int& nidata,
                            int& nvdata,
                            int& nweight);

int (*nrn2core_get_dat2_2_)(int tid,
                            int*& v_parent_index,
                            double*& a,
                            double*& b,
                            double*& area,
                            double*& v,
                            double*& diamvec);

int (*nrn2core_get_dat2_mech_)(int tid,
                               size_t i,
                               int dsz_inst,
                               int*& nodeindices,
                               double*& data,
                               int*& pdata);

int (*nrn2core_get_dat2_3_)(int tid,
                            int nweight,
                            int*& output_vindex,
                            double*& output_threshold,
                            int*& netcon_pnttype,
                            int*& netcon_pntindex,
                            double*& weights,
                            double*& delays);

int (*nrn2core_get_dat2_corepointer_)(int tid, int& n);

int (*nrn2core_get_dat2_corepointer_mech_)(int tid,
                                           int type,
                                           int& icnt,
                                           int& dcnt,
                                           int*& iarray,
                                           double*& darray);

int (*nrn2core_get_dat2_vecplay_)(int tid, int& n);

int (*nrn2core_get_dat2_vecplay_inst_)(int tid,
                                       int i,
                                       int& vptype,
                                       int& mtype,
                                       int& ix,
                                       int& sz,
                                       double*& yvec,
                                       double*& tvec);

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
int nrn_setup_multiple = 1; /* default */
int nrn_setup_extracon = 0; /* default */
static int maxgid;
// no gid in any file can be greater than maxgid. maxgid will be set so that
// maxgid * nrn_setup_multiple < 0x7fffffff

// nrn_setup_extracon extra connections per NrnThread.
// i.e. nrn_setup_extracon * nrn_setup_multiple * nrn_nthread
// extra connections on this process.
// The targets of the connections on a NrnThread are randomly selected
// (with replacement) from the set of ProbAMPANMDA_EMS on the thread.
// (This synapse type is not strictly appropriate to be used as
// a generalized synapse with multiple input streams since some of its
// range variables store quantities that should be stream specific
// and therefore should be stored in the NetCon weight vector. But it
// might be good enough for our purposes. In any case, we'd like to avoid
// creating new POINT_PROCESS instances with all the extra complexities
// involved in adjusting the data arrays.)
// The nrn_setup_extracon value is used to allocate the appropriae
// amount of extra space for NrnThread.netcons and NrnThread.weights
//
// The most difficult problem is to augment the rank wide inputpresyn_ list.
// We wish to randomly choose source gids for the extracon NetCons from the
// set of gids not in "multiple" instance of the model the NrnThread is a
// member of. We need to take into account the possibilty of multiple
// NrnThread in multiple "multiple" instances having extra NetCon with the
// same source gid. That some of the source gids may be already be
// associated with already existing PreSyn on this rank is a minor wrinkle.
// This is done between phase1 and phase2 during the call to
// determine_inputpresyn().

// If MATRIX_LAYOUT is 1 then a,b,d,rhs,v,area is not padded using NRN_SOA_PAD
// When MATRIX_LAYOUT is 0 then mechanism pdata index values into _actual_v
// and _actual_area data need to be updated.
#if !defined(LAYOUT)
#define LAYOUT 1
#define MATRIX_LAYOUT 1
#else
#define MATRIX_LAYOUT LAYOUT
#endif

#if !defined(NRN_SOA_PAD)
// for layout 0, every range variable array must have a size which
// is a multiple of NRN_SOA_PAD doubles
#define NRN_SOA_PAD 8
#endif

#ifdef _OPENMP
static MUTDEC
#endif

    static size_t
    model_size(void);

/// Vector of maps for negative presyns
std::vector<std::map<int, PreSyn*> > neg_gid2out;
/// Maps for ouput and input presyns
std::map<int, PreSyn*> gid2out;
std::map<int, InputPreSyn*> gid2in;

/// InputPreSyn.nc_index_ to + InputPreSyn.nc_cnt_ give the NetCon*
std::vector<NetCon*> netcon_in_presyn_order_;

/// Only for setup vector of netcon source gids
std::vector<int*> netcon_srcgid;


// Wrap read_phase1 and read_phase2 calls to allow using  nrn_multithread_job.
// Args marshaled by store_phase_args are used by phase1_wrapper
// and phase2_wrapper.
static void store_phase_args(int ngroup,
                             int* gidgroups,
                             int* imult,
                             FileHandler* file_reader,
                             const char* path,
                             const char* restore_path,
                             int byte_swap) {
    ngroup_w = ngroup;
    gidgroups_w = gidgroups;
    imult_w = imult;
    file_reader_w = file_reader;
    path_w = path;
    restore_path_w = restore_path;
    byte_swap_w = (bool)byte_swap;
}

/* read files.dat file and distribute cellgroups to all mpi ranks */
void nrn_read_filesdat(int& ngrp, int*& grp, int multiple, int*& imult, const char* filesdat) {
    patstimtype = nrn_get_mechtype("PatternStim");
    if (corenrn_embedded) {
        ngrp = corenrn_embedded_nthread;
        nrn_assert(multiple == 1);
        grp = new int[ngrp + 1];
        imult = new int[ngrp + 1];
        (*nrn2core_group_ids_)(grp);
        for (int i = 0; i <= ngrp; ++i) {
            imult[i] = 0;
        }
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
        nrn_have_gaps = 1;
        if (nrnmpi_myid == 0) {
            printf("Model uses gap junctions\n");
        }
    }

    if (nrnmpi_numprocs > iNumFiles && nrnmpi_myid == 0) {
        printf(
            "Info : The number of input datasets are less than ranks, some ranks will be idle!\n");
    }

    ngrp = 0;
    grp = new int[iNumFiles * multiple / nrnmpi_numprocs + 1];
    imult = new int[iNumFiles * multiple / nrnmpi_numprocs + 1];

    // irerate over gids in files.dat
    for (int iNum = 0; iNum < iNumFiles * multiple; ++iNum) {
        int iFile;

        nrn_assert(fscanf(fp, "%d\n", &iFile) == 1);
        if ((iNum % nrnmpi_numprocs) == nrnmpi_myid) {
            grp[ngrp] = iFile;
            imult[ngrp] = iNum / iNumFiles;
            ngrp++;
        }
        if ((iNum + 1) % iNumFiles == 0) {
            // re-read file for each multiple (skipping the two header lines)
            rewind(fp);
            nrn_assert(fscanf(fp, "%*s\n") == 0);
            nrn_assert(fscanf(fp, "%*d\n") == 0);
        }
    }

    fclose(fp);
}

static void read_phase1(int* output_gid, int imult, NrnThread& nt);

static void* direct_phase1(NrnThread* n) {
    NrnThread& nt = *n;
    int* output_gid;
    int valid =
        (*nrn2core_get_dat1_)(nt.id, nt.n_presyn, nt.n_netcon, output_gid, netcon_srcgid[nt.id]);
    if (valid) {
        read_phase1(output_gid, 0, nt);
    }
    return nullptr;
}

void read_phase1(FileHandler& F, int imult, NrnThread& nt) {
    assert(!F.fail());
    nt.n_presyn = F.read_int();  /// Number of PreSyn-s in NrnThread nt
    nt.n_netcon = F.read_int();  /// Number of NetCon-s in NrnThread nt

    int* output_gid = F.read_array<int>(nt.n_presyn);
    // the extra netcon_srcgid will be filled in later
    netcon_srcgid[nt.id] = new int[nt.n_netcon + nrn_setup_extracon];
    F.read_array<int>(netcon_srcgid[nt.id], nt.n_netcon);
    F.close();

    read_phase1(output_gid, imult, nt);
}

static void read_phase1(int* output_gid, int imult, NrnThread& nt) {
    int zz = imult * maxgid;  // offset for each gid
    // offset the (non-negative) gids according to multiple
    // make sure everything fits into gid space.
    for (int i = 0; i < nt.n_presyn; ++i) {
        if (output_gid[i] >= 0) {
            nrn_assert(output_gid[i] < maxgid);
            output_gid[i] += zz;
        }
    }

    nt.presyns = new PreSyn[nt.n_presyn];
    nt.netcons = new NetCon[nt.n_netcon + nrn_setup_extracon];
    nt.presyns_helper = (PreSynHelper*)ecalloc_align(nt.n_presyn, sizeof(PreSynHelper));

    int* nc_srcgid = netcon_srcgid[nt.id];
    for (int i = 0; i < nt.n_netcon; ++i) {
        if (nc_srcgid[i] >= 0) {
            nrn_assert(nc_srcgid[i] < maxgid);
            nc_srcgid[i] += zz;
        }
    }

    for (int i = 0; i < nt.n_presyn; ++i) {
        int gid = output_gid[i];
        if (gid == -1)
            continue;

        // Note that the negative (type, index)
        // coded information goes into the neg_gid2out[tid] hash table.
        // See netpar.cpp for the netpar_tid_... function implementations.
        // Both that table and the process wide gid2out table can be deleted
        // before the end of setup

        MUTLOCK
        /// Put gid into the gid2out hash table with correspondent output PreSyn
        /// Or to the negative PreSyn map
        PreSyn* ps = nt.presyns + i;
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
            gid2out[gid] = ps;
            ps->gid_ = gid;
            ps->output_index_ = gid;
        } else {
            nrn_assert(neg_gid2out[nt.id].find(gid) == neg_gid2out[nt.id].end());
            neg_gid2out[nt.id][gid] = ps;
        }
        MUTUNLOCK

        if (gid < 0) {
            nt.presyns[i].output_index_ = -1;
        }
    }

    delete[] output_gid;

    if (nrn_setup_extracon > 0) {
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
        int sidoffset;  // how much to increment the corresponding positive gid
        // like ring connectivity
        if (imult > 0) {
            sidoffset = -maxgid;
        } else if (nrn_setup_multiple > 1) {
            sidoffset = (nrn_setup_multiple - 1) * maxgid;
        } else {
            sidoffset = 0;
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
            nc_srcgid[i + nt.n_netcon] = sid;
        }
        // finally increment the n_netcon
        nt.n_netcon += nrn_setup_extracon;
    }
}

void netpar_tid_gid2ps(int tid, int gid, PreSyn** ps, InputPreSyn** psi) {
    /// for gid < 0 returns the PreSyn* in the thread (tid) specific map.
    *ps = nullptr;
    *psi = nullptr;
    std::map<int, PreSyn*>::iterator gid2out_it;
    if (gid >= 0) {
        gid2out_it = gid2out.find(gid);
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
        gid2out_it = neg_gid2out[tid].find(gid);
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
    std::map<int, PreSyn*>::iterator gid2out_it;
    std::map<int, InputPreSyn*>::iterator gid2in_it;

    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        // associate gid with InputPreSyn and increase PreSyn and InputPreSyn count
        nt.n_input_presyn = 0;
        for (int i = 0; i < nt.n_netcon; ++i) {
            int gid = netcon_srcgid[ith][i];
            if (gid >= 0) {
                /// If PreSyn or InputPreSyn is already in the map
                gid2out_it = gid2out.find(gid);
                if (gid2out_it != gid2out.end()) {
                    /// Increase PreSyn count
                    ++gid2out_it->second->nc_cnt_;
                    continue;
                }
                gid2in_it = gid2in.find(gid);
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
                gid2out_it = neg_gid2out[nt.id].find(gid);
                if (gid2out_it != neg_gid2out[nt.id].end()) {
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
    for (size_t i = 0; i < inputpresyn_.size(); ++i) {
        InputPreSyn* psi = inputpresyn_[i];
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
    // with no presyn (ie. netcon_srcgid[nt.id][i] = -1) but that is ok since they are
    // only used via ps.nc_index_ and ps.nc_cnt_;
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        NrnThread& nt = nrn_threads[ith];
        for (int i = 0; i < nt.n_netcon; ++i) {
            NetCon* nc = nt.netcons + i;
            int gid = netcon_srcgid[ith][i];
            PreSyn* ps;
            InputPreSyn* psi;
            netpar_tid_gid2ps(ith, gid, &ps, &psi);
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
        if (netcon_srcgid[ith])
            delete[] netcon_srcgid[ith];
    }
    netcon_srcgid.clear();
    neg_gid2out.clear();
}

void nrn_setup(const char* filesdat,
               bool is_mapping_needed,
               int byte_swap,
               bool run_setup_cleanup) {
    /// Number of local cell groups
    int ngroup = 0;

    /// Array of cell group numbers (indices)
    int* gidgroups = nullptr;

    /// Array of duplicate indices. Normally, with nrn_setup_multiple=1,
    //   they are ngroup values of 0.
    int* imult = nullptr;

    double time = nrn_wtime();

    maxgid = 0x7fffffff / nrn_setup_multiple;
    nrn_read_filesdat(ngroup, gidgroups, nrn_setup_multiple, imult, filesdat);

    if (!MUTCONSTRUCTED) {
        MUTCONSTRUCT(1)
    }
    // temporary bug work around. If any process has multiple threads, no
    // process can have a single thread. So, for now, if one thread, make two.
    // Fortunately, empty threads work fine.
    // Allocate NrnThread* nrn_threads of size ngroup (minimum 2)
    // Note that rank with 0 dataset/cellgroup works fine
    nrn_threads_create(ngroup <= 1 ? 2 : ngroup);

#if 1 || CHKPNTDEBUG  // only required for NrnThreadChkpnt.file_id
    nrnthread_chkpnt = new NrnThreadChkpnt[nrn_nthread];
#endif

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
    neg_gid2out.resize(ngroup);

    // bug fix. gid2out is cumulative over all threads and so do not
    // know how many there are til after phase1
    // A process's complete set of output gids and allocation of each thread's
    // nt.presyns and nt.netcons arrays.
    // Generates the gid2out map which is needed
    // to later count the required number of InputPreSyn
    /// gid2out - map of output presyn-s
    /// std::map<int, PreSyn*> gid2out;
    gid2out.clear();

    netcon_srcgid.resize(nrn_nthread);
    for (int i = 0; i < nrn_nthread; ++i)
        netcon_srcgid[i] = nullptr;

    FileHandler* file_reader = new FileHandler[ngroup];

    std::string datapath = nrnopt_get_str("--datpath");
    std::string restore_path = nrnopt_get_str("--restore");

    // if not restoring then phase2 files will be read from dataset directory
    if (!restore_path.length()) {
        restore_path = datapath;
    }

    /* nrn_multithread_job supports serial, pthread, and openmp. */
    store_phase_args(ngroup, gidgroups, imult, file_reader, datapath.c_str(), restore_path.c_str(),
                     byte_swap);

    // gap junctions
    if (nrn_have_gaps) {
        assert(nrn_setup_multiple == 1);
        nrn_partrans::transfer_thread_data_ = new nrn_partrans::TransferThreadData[nrn_nthread];
        nrn_partrans::setup_info_ = new nrn_partrans::SetupInfo[ngroup];
        if (!corenrn_embedded) {
            coreneuron::phase_wrapper<coreneuron::gap>();
        } else {
            nrn_assert(sizeof(nrn_partrans::sgid_t) == sizeof(int));
            for (int i = 0; i < ngroup; ++i) {
                nrn_partrans::SetupInfo& si = nrn_partrans::setup_info_[i];
                (*nrn2core_get_partrans_setup_info_)(i, si.ntar, si.nsrc, si.type, si.ix_vpre,
                                                     si.sid_target, si.sid_src, si.v_indices);
            }
        }
        nrn_partrans::gap_mpi_setup(ngroup);
    }

    if (!corenrn_embedded) {
        coreneuron::phase_wrapper<(coreneuron::phase)1>();  /// If not the xlc compiler, it should
                                                            /// be coreneuron::phase::one
    } else {
        nrn_multithread_job(direct_phase1);
    }

    // from the gid2out map and the netcon_srcgid array,
    // fill the gid2in, and from the number of entries,
    // allocate the process wide InputPreSyn array
    determine_inputpresyn();

    // read the rest of the gidgroup's data and complete the setup for each
    // thread.
    /* nrn_multithread_job supports serial, pthread, and openmp. */
    coreneuron::phase_wrapper<(coreneuron::phase)2>(corenrn_embedded);

    if (is_mapping_needed)
        coreneuron::phase_wrapper<(coreneuron::phase)3>();

    double mindelay = set_mindelay(nrnopt_get_dbl("--mindelay"));
    nrnopt_modify_dbl("--mindelay", mindelay);

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

    delete[] file_reader;

    model_size();
    delete[] gidgroups;
    delete[] imult;

    if (nrnmpi_myid == 0) {
        printf(" Setup Done   : %.2lf seconds \n", nrn_wtime() - time);
    }
}

void setup_ThreadData(NrnThread& nt) {
    for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
        Memb_func& mf = corenrn.get_memb_func(tml->index);
        Memb_list* ml = tml->ml;
        if (mf.thread_size_) {
            ml->_thread = (ThreadDatum*)ecalloc_align(mf.thread_size_, sizeof(ThreadDatum));
            if (mf.thread_mem_init_) {
                MUTLOCK (*mf.thread_mem_init_)(ml->_thread);
                MUTUNLOCK
            }
        } else {
            ml->_thread = nullptr;
        }
    }
}

void read_phasegap(FileHandler& F, int imult, NrnThread& nt) {
    nrn_assert(imult == 0);
    nrn_partrans::SetupInfo& si = nrn_partrans::setup_info_[nt.id];
    si.ntar = 0;
    si.nsrc = 0;

    if (F.fail()) {
        return;
    }

    int chkpntsave = F.checkpoint();
    F.checkpoint(0);

    si.ntar = F.read_int();
    si.nsrc = F.read_int();
    si.type = F.read_int();
    si.ix_vpre = F.read_int();
    si.sid_target = F.read_array<int>(si.ntar);
    si.sid_src = F.read_array<int>(si.nsrc);
    si.v_indices = F.read_array<int>(si.nsrc);

    F.checkpoint(chkpntsave);

#if 0
  printf("%d read_phasegap tid=%d type=%d %s ix_vpre=%d nsrc=%d ntar=%d\n",
    nrnmpi_myid, nt.id, si.type, memb_func[si.type].sym, si.ix_vpre,
    si.nsrc, si.ntar);
  for (int i=0; i < si.nsrc; ++i) {
    printf("sid_src %d %d\n", si.sid_src[i], si.v_indices[i]);
  }
  for (int i=0; i <si. ntar; ++i) {
    printf("sid_tar %d %d\n", si.sid_target[i], i);
  }
#endif
}

int nrn_soa_padded_size(int cnt, int layout) {
    return soa_padded_size<NRN_SOA_PAD>(cnt, layout);
}

static size_t nrn_soa_byte_align(size_t i) {
    if (LAYOUT == 0) {
        size_t dbl_align = NRN_SOA_BYTE_ALIGN / sizeof(double);
        size_t rem = i % dbl_align;
        if (rem) {
            i += dbl_align - rem;
        }
        assert((i * sizeof(double)) % NRN_SOA_BYTE_ALIGN == 0);
    }
    return i;
}

// file data is AoS. ie.
// organized as cnt array instances of mtype each of size sz.
// So input index i refers to i_instance*sz + i_item offset
// Return the corresponding SoA index -- taking into account the
// alignment requirements. Ie. i_instance + i_item*align_cnt.

int nrn_param_layout(int i, int mtype, Memb_list* ml) {
    int layout = corenrn.get_mech_data_layout()[mtype];
    if (layout == 1) {
        return i;
    }
    assert(layout == 0);
    int sz = corenrn.get_prop_param_size()[mtype];
    int cnt = ml->nodecount;
    int i_cnt = i / sz;
    int i_sz = i % sz;
    return nrn_i_layout(i_cnt, cnt, i_sz, sz, layout);
}

int nrn_i_layout(int icnt, int cnt, int isz, int sz, int layout) {
    if (layout == 1) {
        return icnt * sz + isz;
    } else if (layout == 0) {
        int padded_cnt = nrn_soa_padded_size(cnt, layout);  // may want to factor out to save time
        return icnt + isz * padded_cnt;
    }
    assert(0);
    return 0;
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

template <typename T>
inline void mech_layout(FileHandler& F, T* data, int cnt, int sz, int layout) {
    if (layout == 1) { /* AoS */
        if (corenrn_embedded) {
            return;
        }
        F.read_array<T>(data, cnt * sz);
    } else if (layout == 0) { /* SoA */
        int align_cnt = nrn_soa_padded_size(cnt, layout);
        T* d;
        if (corenrn_embedded) {
            d = new T[cnt * sz];
            for (int i = 0; i < cnt; ++i) {
                for (int j = 0; j < sz; ++j) {
                    d[i * sz + j] = data[i * sz + j];
                }
            }
        } else {
            d = F.read_array<T>(cnt * sz);
        }
        for (int i = 0; i < cnt; ++i) {
            for (int j = 0; j < sz; ++j) {
                data[i + j * align_cnt] = d[i * sz + j];
            }
        }
        delete[] d;
    }
}

/* nrn_threads_free() presumes all NrnThread and NrnThreadMembList data is
 * allocated with malloc(). This is not the case here, so let's try and fix
 * things up first. */

void nrn_cleanup(bool clean_ion_global_map) {
    clear_event_queue();  // delete left-over TQItem
    gid2in.clear();
    gid2out.clear();

    // clean nrnthread_chkpnt
    if (nrnthread_chkpnt) {
        delete[] nrnthread_chkpnt;
        nrnthread_chkpnt = nullptr;
    }

    // clean ezOpt parser allocated memory (if any)
    nrnopt_delete();

    // clean ions global maps
    if (clean_ion_global_map) {
        for (int i = 0; i < nrn_ion_global_map_size; i++)
            free_memory(nrn_ion_global_map[i]);
        free_memory(nrn_ion_global_map);
        nrn_ion_global_map = nullptr;
        nrn_ion_global_map_size = 0;
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
                if (nsb->_size) {
                    free_memory(nsb->_sendtype);
                    free_memory(nsb->_vdata_index);
                    free_memory(nsb->_pnt_index);
                    free_memory(nsb->_weight_index);
                    free_memory(nsb->_nsb_t);
                    free_memory(nsb->_nsb_flag);
                }
                free_memory(nsb);
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
            for (int i = 0; i < corenrn.get_has_net_event().size(); ++i) {
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

void read_phase2(FileHandler& F, int imult, NrnThread& nt) {
    bool direct = corenrn_embedded ? true : false;
    if (!direct) {
        assert(!F.fail());  // actually should assert that it is open
    }
    nrn_assert(imult >= 0);  // avoid imult unused warning
#if 1 || CHKPNTDEBUG
    NrnThreadChkpnt& ntc = nrnthread_chkpnt[nt.id];
    ntc.file_id = gidgroups_w[nt.id];
#endif
    NrnThreadMembList* tml;

    int n_outputgid, ndiam, nmech, *tml_index, *ml_nodecount;
    if (direct) {
        int nidata, nvdata;
        (*nrn2core_get_dat2_1_)(nt.id, n_outputgid, nt.ncell, nt.end, ndiam, nmech, tml_index,
                                ml_nodecount, nidata, nvdata, nt.n_weight);
        nt._nidata = nidata;
        nt._nvdata = nvdata;
    } else {
        n_outputgid = F.read_int();
        nt.ncell = F.read_int();
        nt.end = F.read_int();
        ndiam = F.read_int();  // 0 if not needed, else nt.end
        nmech = F.read_int();
        tml_index = new int[nmech];
        ml_nodecount = new int[nmech];
        int diff_mech_count = 0;
        for (int i = 0; i < nmech; ++i) {
            tml_index[i] = F.read_int();
            ml_nodecount[i] = F.read_int();
            if (std::any_of(corenrn.get_different_mechanism_type().begin(),
                            corenrn.get_different_mechanism_type().end(),
                [&](int e) { return e == tml_index[i]; })) {
                if (nrnmpi_myid == 0) {
                    printf("Error: %s is a different MOD file than used by NEURON!\n",
                           nrn_get_mechname(tml_index[i]));
                }
                diff_mech_count++;
            }
        }

        if (diff_mech_count > 0) {
            if (nrnmpi_myid == 0) {
                printf(
                    "Error : NEURON and CoreNEURON must use same mod files for compatibility, %d different mod file(s) found. Re-compile special and special-core!\n",
                    diff_mech_count);
            }
            nrn_abort(1);
        }

        nt._nidata = F.read_int();
        nt._nvdata = F.read_int();
        nt.n_weight = F.read_int();
    }

#if CHKPNTDEBUG
    ntc.n_outputgids = n_outputgid;
    ntc.nmech = nmech;
#endif
    if (!direct) {
        nrn_assert(n_outputgid > 0);  // avoid n_outputgid unused warning
    }

    /// Checkpoint in coreneuron is defined for both phase 1 and phase 2 since they are written
    /// together
    // printf("ncell=%d end=%d nmech=%d\n", nt.ncell, nt.end, nmech);
    // printf("nart=%d\n", nart);
    NrnThreadMembList* tml_last = nullptr;
    nt._ml_list = (Memb_list**)ecalloc_align(corenrn.get_memb_funcs().size(), sizeof(Memb_list*));

#if CHKPNTDEBUG
    ntc.mlmap = new Memb_list_chkpnt*[memb_func.size()];
    for (int i = 0; i < _memb_func.size(); ++i) {
        ntc.mlmap[i] = nullptr;
    }
#endif

    int shadow_rhs_cnt = 0;
    nt.shadow_rhs_cnt = 0;

    nt.stream_id = 0;
    nt.compute_gpu = 0;
    auto& nrn_prop_param_size_ = corenrn.get_prop_param_size();
    auto& nrn_prop_dparam_size_ = corenrn.get_prop_dparam_size();

/* read_phase2 is being called from openmp region
 * and hence we can set the stream equal to current thread id.
 * In fact we could set gid as stream_id when we will have nrn threads
 * greater than number of omp threads.
 */
#if defined(_OPENMP)
    nt.stream_id = omp_get_thread_num();
#endif
    auto& memb_func = corenrn.get_memb_funcs();
    for (int i = 0; i < nmech; ++i) {
        tml = (NrnThreadMembList*)emalloc_align(sizeof(NrnThreadMembList));
        tml->ml = (Memb_list*)ecalloc_align(1, sizeof(Memb_list));
        tml->ml->_net_receive_buffer = nullptr;
        tml->ml->_net_send_buffer = nullptr;
        tml->ml->_permute = nullptr;
        tml->next = nullptr;
        tml->index = tml_index[i];
        if (memb_func[tml->index].alloc == nullptr) {
            hoc_execerror(memb_func[tml->index].sym, "mechanism does not exist");
        }
        tml->ml->nodecount = ml_nodecount[i];
        if (!memb_func[tml->index].sym) {
            printf("%s (type %d) is not available\n", nrn_get_mechname(tml->index), tml->index);
            exit(1);
        }
        tml->ml->_nodecount_padded =
            nrn_soa_padded_size(tml->ml->nodecount, corenrn.get_mech_data_layout()[tml->index]);
        if (memb_func[tml->index].is_point && corenrn.get_is_artificial()[tml->index] == 0) {
            // Avoid race for multiple PointProcess instances in same compartment.
            if (tml->ml->nodecount > shadow_rhs_cnt) {
                shadow_rhs_cnt = tml->ml->nodecount;
            }
        }

        nt._ml_list[tml->index] = tml->ml;
#if CHKPNTDEBUG
        Memb_list_chkpnt* mlc = new Memb_list_chkpnt;
        ntc.mlmap[tml->index] = mlc;
#endif
        // printf("index=%d nodecount=%d membfunc=%s\n", tml->index, tml->ml->nodecount,
        // memb_func[tml->index].sym?memb_func[tml->index].sym:"None");
        if (nt.tml) {
            tml_last->next = tml;
        } else {
            nt.tml = tml;
        }
        tml_last = tml;
    }
    delete[] tml_index;
    delete[] ml_nodecount;

    if (shadow_rhs_cnt) {
        nt._shadow_rhs =
            (double*)ecalloc_align(nrn_soa_padded_size(shadow_rhs_cnt, 0), sizeof(double));
        nt._shadow_d =
            (double*)ecalloc_align(nrn_soa_padded_size(shadow_rhs_cnt, 0), sizeof(double));
        nt.shadow_rhs_cnt = shadow_rhs_cnt;
    }

    nt._data = nullptr;    // allocated below after padding
    nt.mapping = nullptr;  // section segment mapping

    if (nt._nidata)
        nt._idata = (int*)ecalloc(nt._nidata, sizeof(int));
    else
        nt._idata = nullptr;
    // see patternstim.cpp
    int extra_nv = (&nt == nrn_threads) ? nrn_extra_thread0_vdata : 0;
    if (nt._nvdata + extra_nv)
        nt._vdata = (void**)ecalloc_align(nt._nvdata + extra_nv, sizeof(void*));
    else
        nt._vdata = nullptr;
    // printf("_nidata=%d _nvdata=%d\n", nt._nidata, nt._nvdata);

    // The data format begins with the matrix data
    int ne = nrn_soa_padded_size(nt.end, MATRIX_LAYOUT);
    size_t offset = 6 * ne;

    if (ndiam) {
        // in the rare case that a mechanism has dparam with diam semantics
        // then actual_diam array added after matrix in nt._data
        // Generally wasteful since only a few diam are pointed to.
        // Probably better to move the diam semantics to the p array of the mechanism
        offset += ne;
    }

    // Memb_list.data points into the nt.data array.
    // Also count the number of Point_process
    int npnt = 0;
    for (tml = nt.tml; tml; tml = tml->next) {
        Memb_list* ml = tml->ml;
        int type = tml->index;
        int layout = corenrn.get_mech_data_layout()[type];
        int n = ml->nodecount;
        int sz = nrn_prop_param_size_[type];
        offset = nrn_soa_byte_align(offset);
        ml->data = (double*)0 + offset;  // adjust below since nt._data not allocated
        offset += nrn_soa_padded_size(n, layout) * sz;
        if (corenrn.get_pnt_map()[type] > 0) {
            npnt += n;
        }
    }
    nt.pntprocs = (Point_process*)ecalloc_align(
        npnt, sizeof(Point_process));  // includes acell with and without gid
    nt.n_pntproc = npnt;
    // printf("offset=%ld\n", offset);
    nt._ndata = offset;

    // now that we know the effect of padding, we can allocate data space,
    // fill matrix, and adjust Memb_list data pointers
    nt._data = (double*)ecalloc_align(nt._ndata, sizeof(double));
    nt._actual_rhs = nt._data + 0 * ne;
    nt._actual_d = nt._data + 1 * ne;
    nt._actual_a = nt._data + 2 * ne;
    nt._actual_b = nt._data + 3 * ne;
    nt._actual_v = nt._data + 4 * ne;
    nt._actual_area = nt._data + 5 * ne;
    nt._actual_diam = ndiam ? nt._data + 6 * ne : nullptr;
    for (tml = nt.tml; tml; tml = tml->next) {
        Memb_list* ml = tml->ml;
        ml->data = nt._data + (ml->data - (double*)0);
    }

    // matrix info
    nt._v_parent_index = (int*)ecalloc_align(nt.end, sizeof(int));
    if (direct) {
        (*nrn2core_get_dat2_2_)(nt.id, nt._v_parent_index, nt._actual_a, nt._actual_b,
                                nt._actual_area, nt._actual_v, nt._actual_diam);
    } else {
        F.read_array<int>(nt._v_parent_index, nt.end);
        F.read_array<double>(nt._actual_a, nt.end);
        F.read_array<double>(nt._actual_b, nt.end);
        F.read_array<double>(nt._actual_area, nt.end);
        F.read_array<double>(nt._actual_v, nt.end);
        if (ndiam) {
            F.read_array<double>(nt._actual_diam, nt.end);
        }
    }
#if CHKPNTDEBUG
    ntc.parent = new int[nt.end];
    memcpy(ntc.parent, nt._v_parent_index, nt.end * sizeof(int));
    ntc.area = new double[nt.end];
    memcpy(ntc.area, nt._actual_area, nt.end * sizeof(double));
#endif

    int synoffset = 0;
    std::vector<int> pnt_offset(memb_func.size());

    // All the mechanism data and pdata.
    // Also fill in the pnt_offset
    // Complete spec of Point_process except for the acell presyn_ field.
    int itml = 0;
    int dsz_inst = 0;
    for (tml = nt.tml, itml = 0; tml; tml = tml->next, ++itml) {
        int type = tml->index;
        Memb_list* ml = tml->ml;
        int is_art = corenrn.get_is_artificial()[type];
        int n = ml->nodecount;
        int szp = nrn_prop_param_size_[type];
        int szdp = nrn_prop_dparam_size_[type];
        int layout = corenrn.get_mech_data_layout()[type];

        if (!is_art && !direct) {
            ml->nodeindices = (int*)ecalloc_align(ml->nodecount, sizeof(int));
        } else {
            ml->nodeindices = nullptr;
        }
        if (szdp) {
            ml->pdata = (int*)ecalloc_align(nrn_soa_padded_size(n, layout) * szdp, sizeof(int));
        }

        if (direct) {
            (*nrn2core_get_dat2_mech_)(nt.id, itml, dsz_inst, ml->nodeindices, ml->data, ml->pdata);
        } else {
            if (!is_art) {
                F.read_array<int>(ml->nodeindices, ml->nodecount);
            }
        }
        if (szdp) {
            ++dsz_inst;
        }

        mech_layout<double>(F, ml->data, n, szp, layout);

        if (szdp) {
            mech_layout<int>(F, ml->pdata, n, szdp, layout);
#if CHKPNTDEBUG  // Not substantive. Only for debugging.
            Memb_list_ckpnt* mlc = ntc.mlmap[type];
            mlc->pdata_not_permuted = (int*)coreneuron::ecalloc_align(n * szdp, sizeof(int));
            if (layout == 1) {  // AoS just copy
                for (int i = 0; i < n; ++i) {
                    for (int j = 0; j < szdp; ++j) {
                        mlc->pdata_not_permuted[i * szdp + j] = ml->pdata[i * szdp + j];
                    }
                }
            } else if (layout == 0) {  // SoA transpose and unpad
                int align_cnt = nrn_soa_padded_size(n, layout);
                for (int i = 0; i < n; ++i) {
                    for (int j = 0; j < szdp; ++j) {
                        mlc->pdata_not_permuted[i * szdp + j] = ml->pdata[i + j * align_cnt];
                    }
                }
            }
#endif
        } else {
            ml->pdata = nullptr;
        }
        if (corenrn.get_pnt_map()[type] > 0) {  // POINT_PROCESS mechanism including acell
            int cnt = ml->nodecount;
            Point_process* pnt = nullptr;
            pnt = nt.pntprocs + synoffset;
            pnt_offset[type] = synoffset;
            synoffset += cnt;
            for (int i = 0; i < cnt; ++i) {
                Point_process* pp = pnt + i;
                pp->_type = type;
                pp->_i_instance = i;
                nt._vdata[ml->pdata[nrn_i_layout(i, cnt, 1, szdp, layout)]] = pp;
                pp->_tid = nt.id;
            }
        }
    }

    if (nrn_have_gaps == 1) {
        nrn_partrans::gap_thread_setup(nt);
    }

    // Some pdata may index into data which has been reordered from AoS to
    // SoA. The four possibilities are if semantics is -1 (area), -5 (pointer),
    // -9 (diam),
    // or 0-999 (ion variables). Note that pdata has a layout and the
    // type block in nt.data into which it indexes, has a layout.
    for (tml = nt.tml; tml; tml = tml->next) {
        int type = tml->index;
        int layout = corenrn.get_mech_data_layout()[type];
        int* pdata = tml->ml->pdata;
        int cnt = tml->ml->nodecount;
        int szdp = nrn_prop_dparam_size_[type];
        int* semantics = memb_func[type].dparam_semantics;

        // ignore ARTIFICIAL_CELL (has useless area pointer with semantics=-1)
        if (corenrn.get_is_artificial()[type]) {
            continue;
        }

        if (szdp) {
            if (!semantics)
                continue;  // temporary for HDFReport, Binreport which will be skipped in
                           // bbcore_write of HBPNeuron
            nrn_assert(semantics);
        }

        for (int i = 0; i < szdp; ++i) {
            int s = semantics[i];
            if (s == -1) {  // area
                int area0 = nt._actual_area - nt._data;
                for (int iml = 0; iml < cnt; ++iml) {
                    int* pd = pdata + nrn_i_layout(iml, cnt, i, szdp, layout);
                    int ix = *pd;  // relative to beginning of _actual_area
                    nrn_assert((ix >= 0) && (ix < nt.end));
                    *pd = area0 + ix;  // relative to nt._data
                }
            } else if (s == -9) {  // diam
                int diam0 = nt._actual_diam - nt._data;
                for (int iml = 0; iml < cnt; ++iml) {
                    int* pd = pdata + nrn_i_layout(iml, cnt, i, szdp, layout);
                    int ix = *pd;  // relative to beginning of _actual_diam
                    nrn_assert((ix >= 0) && (ix < nt.end));
                    *pd = diam0 + ix;  // relative to nt._data
                }
            } else if (s == -5) {  // pointer assumes a pointer to membrane voltage
                int v0 = nt._actual_v - nt._data;
                for (int iml = 0; iml < cnt; ++iml) {
                    int* pd = pdata + nrn_i_layout(iml, cnt, i, szdp, layout);
                    int ix = *pd;  // relative to _actual_v
                    nrn_assert((ix >= 0) && (ix < nt.end));
                    *pd = v0 + ix;  // relative to nt._data
                }
            } else if (s >= 0 && s < 1000) {  // ion
                int etype = s;
                int elayout = corenrn.get_mech_data_layout()[etype];
                /* if ion is SoA, must recalculate pdata values */
                /* if ion is AoS, have to deal with offset */
                Memb_list* eml = nt._ml_list[etype];
                int edata0 = eml->data - nt._data;
                int ecnt = eml->nodecount;
                int esz = nrn_prop_param_size_[etype];
                for (int iml = 0; iml < cnt; ++iml) {
                    int* pd = pdata + nrn_i_layout(iml, cnt, i, szdp, layout);
                    int ix = *pd;  // relative to the ion data
                    nrn_assert((ix >= 0) && (ix < ecnt * esz));
                    /* Original pd order assumed ecnt groups of esz */
                    *pd = edata0 + nrn_param_layout(ix, etype, eml);
                }
            }
        }
    }

    /* if desired, apply the node permutation. This involves permuting
       at least the node parameter arrays for a, b, and area (and diam) and all
       integer vector values that index into nodes. This could have been done
       when originally filling the arrays with AoS ordered data, but can also
       be done now, after the SoA transformation. The latter has the advantage
       that the present order is consistent with all the layout values. Note
       that after this portion of the permutation, a number of other node index
       vectors will be read and will need to be permuted as well in subsequent
       sections of this function.
    */
    if (use_interleave_permute) {
        nt._permute = interleave_order(nt.id, nt.ncell, nt.end, nt._v_parent_index);
    }
    if (nt._permute) {
        int* p = nt._permute;
        permute_data(nt._actual_a, nt.end, p);
        permute_data(nt._actual_b, nt.end, p);
        permute_data(nt._actual_area, nt.end, p);
        permute_data(nt._actual_v, nt.end,
                     p);  // need if restore or finitialize does not initialize voltage
        if (nt._actual_diam) {
            permute_data(nt._actual_diam, nt.end, p);
        }
        // index values change as well as ordering
        permute_ptr(nt._v_parent_index, nt.end, p);
        node_permute(nt._v_parent_index, nt.end, p);

#if 0
for (int i=0; i < nt.end; ++i) {
  printf("parent[%d] = %d\n", i, nt._v_parent_index[i]);
}
#endif

        // specify the ml->_permute and sort the nodeindices
        for (tml = nt.tml; tml; tml = tml->next) {
            if (tml->ml->nodeindices) {  // not artificial
                permute_nodeindices(tml->ml, p);
            }
        }

        // permute mechanism data, pdata (and values)
        for (tml = nt.tml; tml; tml = tml->next) {
            if (tml->ml->nodeindices) {  // not artificial
                permute_ml(tml->ml, tml->index, nt);
            }
        }

        // permute the Point_process._i_instance
        for (int i = 0; i < nt.n_pntproc; ++i) {
            Point_process& pp = nt.pntprocs[i];
            Memb_list* ml = nt._ml_list[pp._type];
            if (ml->_permute) {
                pp._i_instance = ml->_permute[pp._i_instance];
            }
        }
    }

    if (nrn_have_gaps == 1 && use_interleave_permute) {
        nrn_partrans::gap_indices_permute(nt);
    }

    /* here we setup the mechanism dependencies. if there is a mechanism dependency
     * then we allocate an array for tml->dependencies otherwise set it to nullptr.
     * In order to find out the "real" dependencies i.e. dependent mechanism
     * exist at the same compartment, we compare the nodeindices of mechanisms
     * returned by nrn_mech_depend.
     */

    /* temporary array for dependencies */
    int* mech_deps = (int*)ecalloc(memb_func.size(), sizeof(int));

    for (tml = nt.tml; tml; tml = tml->next) {
        /* initialize to null */
        tml->dependencies = nullptr;
        tml->ndependencies = 0;

        /* get dependencies from the models */
        int deps_cnt = nrn_mech_depend(tml->index, mech_deps);

        /* if dependencies, setup dependency array */
        if (deps_cnt) {
            /* store "real" dependencies in the vector */
            std::vector<int> actual_mech_deps;

            Memb_list* ml = tml->ml;
            int* nodeindices = ml->nodeindices;

            /* iterate over dependencies */
            for (int j = 0; j < deps_cnt; j++) {
                /* memb_list of dependency mechanism */
                Memb_list* dml = nt._ml_list[mech_deps[j]];

                /* dependency mechanism may not exist in the model */
                if (!dml)
                    continue;

                /* take nodeindices for comparison */
                int* dnodeindices = dml->nodeindices;

                /* set_intersection function needs temp vector to push the common values */
                std::vector<int> node_intersection;

                /* make sure they have non-zero nodes and find their intersection */
                if ((ml->nodecount > 0) && (dml->nodecount > 0)) {
                    std::set_intersection(nodeindices, nodeindices + ml->nodecount, dnodeindices,
                                          dnodeindices + dml->nodecount,
                                          std::back_inserter(node_intersection));
                }

                /* if they intersect in the nodeindices, it's real dependency */
                if (!node_intersection.empty()) {
                    actual_mech_deps.push_back(mech_deps[j]);
                }
            }

            /* copy actual_mech_deps to dependencies */
            if (!actual_mech_deps.empty()) {
                tml->ndependencies = actual_mech_deps.size();
                tml->dependencies = (int*)ecalloc(actual_mech_deps.size(), sizeof(int));
                memcpy(tml->dependencies, &actual_mech_deps[0],
                       sizeof(int) * actual_mech_deps.size());
            }
        }
    }

    /* free temp dependency array */
    free(mech_deps);

    /// Fill the BA lists
    std::vector<BAMech*> bamap(memb_func.size());
    for (int i = 0; i < BEFORE_AFTER_SIZE; ++i) {
        BAMech* bam;
        NrnThreadBAList *tbl, **ptbl;
        for (int ii = 0; ii < memb_func.size(); ++ii) {
            bamap[ii] = (BAMech*)0;
        }
        for (bam = corenrn.get_bamech()[i]; bam; bam = bam->next) {
            bamap[bam->type] = bam;
        }
        /* unnecessary but keep in order anyway */
        ptbl = nt.tbl + i;
        for (tml = nt.tml; tml; tml = tml->next) {
            if (bamap[tml->index]) {
                Memb_list* ml = tml->ml;
                tbl = (NrnThreadBAList*)emalloc(sizeof(NrnThreadBAList));
                tbl->next = (NrnThreadBAList*)0;
                tbl->bam = bamap[tml->index];
                tbl->ml = ml;
                *ptbl = tbl;
                ptbl = &(tbl->next);
            }
        }
    }

    // for fast watch statement checking
    // setup a list of types that have WATCH statement
    {
        int sz = 0;  // count the types with WATCH
        for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            if (corenrn.get_watch_check()[tml->index]) {
                ++sz;
            }
        }
        if (sz) {
            nt._watch_types = (int*)ecalloc(sz + 1, sizeof(int));  // nullptr terminated
            sz = 0;
            for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
                if (corenrn.get_watch_check()[tml->index]) {
                    nt._watch_types[sz++] = tml->index;
                }
            }
        }
    }
    auto& pnttype2presyn = corenrn.get_pnttype2presyn();
    auto& nrn_has_net_event_ = corenrn.get_has_net_event();
    // from nrn_has_net_event create pnttype2presyn.
    if (pnttype2presyn.empty()) {
        pnttype2presyn.resize(memb_func.size(), -1);
    }
    for (int i = 0; i < nrn_has_net_event_.size(); ++i) {
        pnttype2presyn[nrn_has_net_event_[i]] = i;
    }
    // create the nt.pnt2presyn_ix array of arrays.
    nt.pnt2presyn_ix = (int**)ecalloc(nrn_has_net_event_.size(), sizeof(int*));
    for (int i = 0; i < nrn_has_net_event_.size(); ++i) {
        Memb_list* ml = nt._ml_list[nrn_has_net_event_[i]];
        if (ml && ml->nodecount > 0) {
            nt.pnt2presyn_ix[i] = (int*)ecalloc(ml->nodecount, sizeof(int));
        }
    }

    // Real cells are at the beginning of the nt.presyns followed by
    // acells (with and without gids mixed together)
    // Here we associate the real cells with voltage pointers and
    // acell PreSyn with the Point_process.
    // nt.presyns order same as output_vindex order
    int *output_vindex, *pnttype, *pntindex;
    double *output_threshold, *delay;
    if (direct) {
        (*nrn2core_get_dat2_3_)(nt.id, nt.n_weight, output_vindex, output_threshold, pnttype,
                                pntindex, nt.weights, delay);
    }
    if (!direct) {
        output_vindex = F.read_array<int>(nt.n_presyn);
    }
#if CHKPNTDEBUG
    ntc.output_vindex = new int[nt.n_presyn];
    memcpy(ntc.output_vindex, output_vindex, nt.n_presyn * sizeof(int));
#endif
    if (nt._permute) {
        // only indices >= 0 (i.e. _actual_v indices) will be changed.
        node_permute(output_vindex, nt.n_presyn, nt._permute);
    }
    if (!direct) {
        output_threshold = F.read_array<double>(nt.ncell);
    }
#if CHKPNTDEBUG
    ntc.output_threshold = new double[nt.ncell];
    memcpy(ntc.output_threshold, output_threshold, nt.ncell * sizeof(double));
#endif
    for (int i = 0; i < nt.n_presyn; ++i) {  // real cells
        PreSyn* ps = nt.presyns + i;

        int ix = output_vindex[i];
        if (ix == -1 && i < nt.ncell) {  // real cell without a presyn
            continue;
        }
        if (ix < 0) {
            ix = -ix;
            int index = ix / 1000;
            int type = ix - index * 1000;
            Point_process* pnt = nt.pntprocs + (pnt_offset[type] + index);
            ps->pntsrc_ = pnt;
            // pnt->_presyn = ps;
            int ip2ps = pnttype2presyn[pnt->_type];
            if (ip2ps >= 0) {
                nt.pnt2presyn_ix[ip2ps][pnt->_i_instance] = i;
            }
            if (ps->gid_ < 0) {
                ps->gid_ = -1;
            }
        } else {
            assert(ps->gid_ > -1);
            ps->thvar_index_ = ix;  // index into _actual_v
            assert(ix < nt.end);
            ps->threshold_ = output_threshold[i];
        }
    }
    delete[] output_vindex;
    delete[] output_threshold;

    // initial net_send_buffer size about 1% of number of presyns
    // nt._net_send_buffer_size = nt.ncell/100 + 1;
    // but, to avoid reallocation complexity on GPU ...
    nt._net_send_buffer_size = nt.ncell;
    nt._net_send_buffer = (int*)ecalloc_align(nt._net_send_buffer_size, sizeof(int));

    // do extracon later as the target and weight info
    // is not directly in the file
    int nnetcon = nt.n_netcon - nrn_setup_extracon;

    int nweight = nt.n_weight;
    // printf("nnetcon=%d nweight=%d\n", nnetcon, nweight);
    // it may happen that Point_process structures will be made unnecessary
    // by factoring into NetCon.

    // Make NetCon.target_ point to proper Point_process. Only the NetCon
    // with pnttype[i] > 0 have a target.
    if (!direct) {
        pnttype = F.read_array<int>(nnetcon);
    }
    if (!direct) {
        pntindex = F.read_array<int>(nnetcon);
    }
#if CHKPNTDEBUG
    ntc.pnttype = new int[nnetcon];
    ntc.pntindex = new int[nnetcon];
    memcpy(ntc.pnttype, pnttype, nnetcon * sizeof(int));
    memcpy(ntc.pntindex, pntindex, nnetcon * sizeof(int));
#endif
    for (int i = 0; i < nnetcon; ++i) {
        int type = pnttype[i];
        if (type > 0) {
            int index = pnt_offset[type] + pntindex[i];  /// Potentially uninitialized pnt_offset[],
                                                         /// check for previous assignments
            Point_process* pnt = nt.pntprocs + index;
            NetCon& nc = nt.netcons[i];
            nc.target_ = pnt;
            nc.active_ = true;
        }
    }

    int extracon_target_type = -1;
    int extracon_target_nweight = 0;
    if (nrn_setup_extracon > 0) {
        // Fill in the extracon target_ and active_.
        // Simplistic.
        // Rotate through the pntindex and use only pnttype for ProbAMPANMDA_EMS
        // (which happens to have a weight vector length of 5.)
        // Edge case: if there is no such synapse, let the target_ be nullptr
        //   and the netcon be inactive.
        // Same pattern as algorithm for extracon netcon_srcgid above in phase1.
        extracon_target_type = nrn_get_mechtype("ProbAMPANMDA_EMS");
        assert(extracon_target_type > 0);
        extracon_target_nweight = corenrn.get_pnt_receive_size()[extracon_target_type];
        int j = 0;
        for (int i = 0; i < nrn_setup_extracon; ++i) {
            int active = 0;
            for (int k = 0; k < nnetcon; ++k) {
                if (pnttype[j] == extracon_target_type) {
                    active = 1;
                    break;
                }
                j = (j + 1) % nnetcon;
            }
            NetCon& nc = nt.netcons[i + nnetcon];
            nc.active_ = active;
            if (active) {
                nc.target_ = nt.pntprocs + (pnt_offset[extracon_target_type] + pntindex[j]);
            } else {
                nc.target_ = nullptr;
            }
        }
    }

    delete[] pntindex;

    // weights in netcons order in groups defined by Point_process target type.
    nt.n_weight += nrn_setup_extracon * extracon_target_nweight;
    if (!direct) {
        nt.weights = (double*)ecalloc_align(nt.n_weight, sizeof(double));
        F.read_array<double>(nt.weights, nweight);
    }

    int iw = 0;
    for (int i = 0; i < nnetcon; ++i) {
        NetCon& nc = nt.netcons[i];
        nc.u.weight_index_ = iw;
        if (pnttype[i] != 0) {
            iw += corenrn.get_pnt_receive_size()[pnttype[i]];
        } else {
            iw += 1;
        }
    }
    assert(iw == nweight);
    delete[] pnttype;

    // delays in netcons order
    if (!direct) {
        delay = F.read_array<double>(nnetcon);
    }
#if CHKPNTDEBUG
    ntc.delay = new double[nnetcon];
    memcpy(ntc.delay, delay, nnetcon * sizeof(double));
#endif
    for (int i = 0; i < nnetcon; ++i) {
        NetCon& nc = nt.netcons[i];
        nc.delay_ = delay[i];
    }
    delete[] delay;

    if (nrn_setup_extracon > 0) {
        // simplistic. delay is 1 and weight is 0.001
        for (int i = 0; i < nrn_setup_extracon; ++i) {
            NetCon& nc = nt.netcons[nnetcon + i];
            nc.delay_ = 1.0;
            nc.u.weight_index_ = nweight + i * extracon_target_nweight;
            nt.weights[nc.u.weight_index_] = 2.0;  // this value 2.0 is extracted from .dat files
        }
    }

    // BBCOREPOINTER information
    if (direct) {
        (*nrn2core_get_dat2_corepointer_)(nt.id, npnt);
    } else {
        npnt = F.read_int();
    }
#if CHKPNTDEBUG
    ntc.nbcp = npnt;
    ntc.bcpicnt = new int[npnt];
    ntc.bcpdcnt = new int[npnt];
    ntc.bcptype = new int[npnt];
#endif
    for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
        int type = tml->index;
        if (!corenrn.get_bbcore_read()[type]) {
            continue;
        }
        int* iArray = nullptr;
        double* dArray = nullptr;
        int icnt, dcnt;
        if (direct) {
            (*nrn2core_get_dat2_corepointer_mech_)(nt.id, type, icnt, dcnt, iArray, dArray);
        } else {
            type = F.read_int();
            icnt = F.read_int();
            dcnt = F.read_int();
            if (icnt) {
                iArray = F.read_array<int>(icnt);
            }
            if (dcnt) {
                dArray = F.read_array<double>(dcnt);
            }
        }
        if (!corenrn.get_bbcore_write()[type] && nrn_checkpoint_arg_exists) {
            fprintf(
                stderr,
                "Checkpoint is requested involving BBCOREPOINTER but there is no bbcore_write function for %s\n",
                memb_func[type].sym);
            assert(corenrn.get_bbcore_write()[type]);
        }
#if CHKPNTDEBUG
        ntc.bcptype[i] = type;
        ntc.bcpicnt[i] = icnt;
        ntc.bcpdcnt[i] = dcnt;
#endif
        int ik = 0;
        int dk = 0;
        Memb_list* ml = nt._ml_list[type];
        int dsz = nrn_prop_param_size_[type];
        int pdsz = nrn_prop_dparam_size_[type];
        int cntml = ml->nodecount;
        int layout = corenrn.get_mech_data_layout()[type];
        for (int j = 0; j < cntml; ++j) {
            int jp = j;
            if (ml->_permute) {
                jp = ml->_permute[j];
            }
            double* d = ml->data;
            Datum* pd = ml->pdata;
            d += nrn_i_layout(jp, cntml, 0, dsz, layout);
            pd += nrn_i_layout(jp, cntml, 0, pdsz, layout);
            int aln_cntml = nrn_soa_padded_size(cntml, layout);
            (*corenrn.get_bbcore_read()[type])(dArray, iArray, &dk, &ik, 0, aln_cntml, d, pd, ml->_thread,
                                      &nt, 0.0);
        }
        assert(dk == dcnt);
        assert(ik == icnt);
        if (ik) {
            delete[] iArray;
        }
        if (dk) {
            delete[] dArray;
        }
    }

    // VecPlayContinuous instances
    // No attempt at memory efficiency
    int n;
    if (direct) {
        (*nrn2core_get_dat2_vecplay_)(nt.id, n);
    } else {
        n = F.read_int();
    }
    nt.n_vecplay = n;
    if (n) {
        nt._vecplay = new void*[n];
    } else {
        nt._vecplay = nullptr;
    }
#if CHKPNTDEBUG
    ntc.vecplay_ix = new int[n];
    ntc.vtype = new int[n];
    ntc.mtype = new int[n];
#endif
    for (int i = 0; i < n; ++i) {
        int vtype, mtype, ix, sz;
        double *yvec1, *tvec1;
        if (direct) {
            (*nrn2core_get_dat2_vecplay_inst_)(nt.id, i, vtype, mtype, ix, sz, yvec1, tvec1);
        } else {
            vtype = F.read_int();
            mtype = F.read_int();
            ix = F.read_int();
            sz = F.read_int();
        }
        nrn_assert(vtype == VecPlayContinuousType);
#if CHKPNTDEBUG
        ntc.vtype[i] = vtype;
#endif
#if CHKPNTDEBUG
        ntc.mtype[i] = mtype;
#endif
        Memb_list* ml = nt._ml_list[mtype];
#if CHKPNTDEBUG
        ntc.vecplay_ix[i] = ix;
#endif
        IvocVect* yvec = vector_new(sz);
        IvocVect* tvec = vector_new(sz);
        if (direct) {
            double* py = vector_vec(yvec);
            double* pt = vector_vec(tvec);
            for (int j = 0; j < sz; ++j) {
                py[j] = yvec1[j];
                pt[j] = tvec1[j];
            }
            // yvec1 and tvec1 are not deleted as that space is within
            // NEURON Vector
        } else {
            F.read_array<double>(vector_vec(yvec), sz);
            F.read_array<double>(vector_vec(tvec), sz);
        }
        ix = nrn_param_layout(ix, mtype, ml);
        if (ml->_permute) {
            ix = nrn_index_permute(ix, mtype, ml);
        }
        nt._vecplay[i] = new VecPlayContinuous(ml->data + ix, yvec, tvec, nullptr, nt.id);
    }
    if (!direct) {
        // store current checkpoint state to continue reading mapping
        F.record_checkpoint();

        // If not at end of file, then this must be a checkpoint and restore tqueue.
        if (!F.eof()) {
            checkpoint_restore_tqueue(nt, F);
        }
    }

    // NetReceiveBuffering
    for (auto& net_buf_receive : corenrn.get_net_buf_receive()) {
        int type = net_buf_receive.second;
        // Does this thread have this type.
        Memb_list* ml = nt._ml_list[type];
        if (ml) {  // needs a NetReceiveBuffer
            NetReceiveBuffer_t* nrb =
                (NetReceiveBuffer_t*)ecalloc_align(1, sizeof(NetReceiveBuffer_t));
            ml->_net_receive_buffer = nrb;
            nrb->_pnt_offset = pnt_offset[type];

            // begin with a size of 5% of the number of instances
            nrb->_size = ml->nodecount;
            // or at least 8
            if (nrb->_size < 8) {
                nrb->_size = 8;
            }
            // but not more than nodecount
            if (nrb->_size > ml->nodecount) {
                nrb->_size = ml->nodecount;
            }

            nrb->_pnt_index = (int*)ecalloc_align(nrb->_size, sizeof(int));
            nrb->_displ = (int*)ecalloc_align(nrb->_size + 1, sizeof(int));
            nrb->_nrb_index = (int*)ecalloc_align(nrb->_size, sizeof(int));
            nrb->_weight_index = (int*)ecalloc_align(nrb->_size, sizeof(int));
            nrb->_nrb_t = (double*)ecalloc_align(nrb->_size, sizeof(double));
            nrb->_nrb_flag = (double*)ecalloc_align(nrb->_size, sizeof(double));
        }
    }

    // NetSendBuffering
    for (int type : corenrn.get_net_buf_send_type()) {
        // Does this thread have this type.
        Memb_list* ml = nt._ml_list[type];
        if (ml) {  // needs a NetSendBuffer
            NetSendBuffer_t* nsb = (NetSendBuffer_t*)ecalloc_align(1, sizeof(NetSendBuffer_t));
            ml->_net_send_buffer = nsb;

            // begin with a size equal to twice number of instances
            // at present there is no provision for dynamically increasing this.
            nsb->_size = ml->nodecount * 2;
            nsb->_cnt = 0;

            nsb->_sendtype = (int*)ecalloc_align(nsb->_size, sizeof(int));
            nsb->_vdata_index = (int*)ecalloc_align(nsb->_size, sizeof(int));
            nsb->_pnt_index = (int*)ecalloc_align(nsb->_size, sizeof(int));
            nsb->_weight_index = (int*)ecalloc_align(nsb->_size, sizeof(int));
            // when == 1, NetReceiveBuffer_t is newly allocated (i.e. we need to free previous copy
            // and recopy new data
            nsb->reallocated = 1;
            nsb->_nsb_t = (double*)ecalloc_align(nsb->_size, sizeof(double));
            nsb->_nsb_flag = (double*)ecalloc_align(nsb->_size, sizeof(double));
        }
    }
}

/** read mapping information for neurons */
void read_phase3(FileHandler& F, int imult, NrnThread& nt) {
    (void)imult;

    /** restore checkpoint state (before restoring queue items */
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
    printf("%s %d psize=%d ppsize=%d cnt=%d nbyte=%ld\n", memb_func[i].sym, i,
           crnrn.get_prop_param_size()[i],
           crnrn.get_prop_dparam_size()[i], tml->ml->nodecount, nbyte);
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
    printf(" gid2in table bytes=~%ld size=%d\n", nbyte, gid2in->size());
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
    NrnThreadMembList* tml;
    size_t nccnt = 0;

    for (int i = 0; i < nrn_nthread; ++i) {
        NrnThread& nt = nrn_threads[i];
        size_t nb_nt = 0;  // per thread
        nccnt += nt.n_netcon;

        // Memb_list size
        int nmech = 0;
        for (tml = nt.tml; tml; tml = tml->next) {
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
