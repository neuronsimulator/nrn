/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <map>
#include <vector>

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/network/partrans.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"

namespace coreneuron {
using namespace coreneuron::nrn_partrans;

nrn_partrans::SetupTransferInfo* nrn_partrans::setup_info_;

class SidInfo {
  public:
    std::vector<int> tids_;
    std::vector<int> indices_;
};

}  // namespace coreneuron
#if NRNLONGSGID
#define sgid_alltoallv nrnmpi_long_alltoallv
#else
#define sgid_alltoallv nrnmpi_int_alltoallv
#endif

#define HAVEWANT_t sgid_t
#define HAVEWANT_alltoallv sgid_alltoallv
#define HAVEWANT2Int std::map<sgid_t, int>
#include "coreneuron/network/have2want.h"

namespace coreneuron {
using namespace coreneuron::nrn_partrans;

void nrn_partrans::gap_mpi_setup(int ngroup) {
    // printf("%d gap_mpi_setup ngroup=%d\n", nrnmpi_myid, ngroup);

    // count total_nsrc, total_ntar and allocate.
    // Possible either or both are 0 on this process.
    size_t total_nsrc = 0, total_ntar = 0;
    for (int tid = 0; tid < ngroup; ++tid) {
        auto& si = setup_info_[tid];
        total_nsrc += si.src_sid.size();
        total_ntar += si.tar_sid.size();
    }

    // have and want arrays (add 1 to guarantee new ... is an array.)
    sgid_t* have = new sgid_t[total_nsrc + 1];
    sgid_t* want = new sgid_t[total_ntar + 1];

    // map from source sid to (tid, index), ie.  NrnThread[tid]._data[index].
    // and target sid to lists of (tid, index) for memb_list
    // also count the map sizes and fill have and want arrays
    std::map<sgid_t, SidInfo> src2info;
    std::map<sgid_t, SidInfo> tar2info;

    int src2info_size = 0, tar2info_size = 0;  // number of unique sids
    for (int tid = 0; tid < ngroup; ++tid) {
        auto& si = setup_info_[tid];
        // Sgid has unique source.

        for (size_t i = 0; i < si.src_sid.size(); ++i) {
            sgid_t sid = si.src_sid[i];
            SidInfo sidinfo;
            sidinfo.tids_.push_back(tid);
            sidinfo.indices_.push_back(i);
            src2info[sid] = sidinfo;
            have[src2info_size] = sid;
            src2info_size++;
        }
        // Possibly many targets of same sid
        // Only want unique sids. From each, can obtain all its targets.
        for (size_t i = 0; i < si.tar_sid.size(); ++i) {
            sgid_t sid = si.tar_sid[i];
            if (tar2info.find(sid) == tar2info.end()) {
                tar2info[sid] = SidInfo();
                want[tar2info_size] = sid;
                tar2info_size++;
            }
            SidInfo& sidinfo = tar2info[sid];
            sidinfo.tids_.push_back(tid);
            sidinfo.indices_.push_back(i);
        }
    }

    // 2) Call the have_to_want function.
    sgid_t* send_to_want;
    sgid_t* recv_from_have;

    have_to_want(have, src2info_size, want, tar2info_size, send_to_want, outsrccnt_, outsrcdspl_,
                 recv_from_have, insrccnt_, insrcdspl_, default_rendezvous);

    int nhost = nrnmpi_numprocs;

    // sanity check. all the sgids we are asked to send, we actually have
    for (int i = 0; i < outsrcdspl_[nhost]; ++i) {
        sgid_t sgid = send_to_want[i];
        assert(src2info.find(sgid) != src2info.end());
    }

    // sanity check. all the sgids we receive, we actually need.
    for (int i = 0; i < insrcdspl_[nhost]; ++i) {
        sgid_t sgid = recv_from_have[i];
        assert(tar2info.find(sgid) != tar2info.end());
    }

#if DEBUG
  printf("%d mpi outsrccnt_, outsrcdspl_, insrccnt, insrcdspl_\n", nrnmpi_myid);
  for (int i = 0; i < nrnmpi_numprocs; ++i) {
    printf("%d : %d %d %d %d\n", nrnmpi_myid, outsrccnt_[i], outsrcdspl_[i],
      insrccnt_[i], insrcdspl_[i]);
  }
#endif

    // clean up a little
    delete[] have;
    delete[] want;

    insrc_buf_ = new double[insrcdspl_[nhost]];
    outsrc_buf_ = new double[outsrcdspl_[nhost]];

    // for i: src_gather[i] = NrnThread._data[src_indices[i]]
    // for j: outsrc_buf[outsrc_indices[j]] = src_gather[gather2outsrc_indices[j]]
    // src_indices point into NrnThread._data
    // Many outsrc_indices elements can point to the same src_gather element
    // but only if an sgid src datum is destined for multiple ranks.
    for (int i = 0; i < outsrcdspl_[nhost]; ++i) {
        sgid_t sgid = send_to_want[i];
        SidInfo& sidinfo = src2info[sgid];
        // only one item in the lists.
        int tid = sidinfo.tids_[0];
        int setup_info_index = sidinfo.indices_[0];

        auto& si = setup_info_[tid];
        auto& ttd = transfer_thread_data_[tid];

        // Note that src_index points into NrnThread.data, as it has already
        // been transformed using original src_type and src_index via
        // stdindex2ptr.
        // For copying into outsrc_buf from src_gather. This is from
        // NrnThread._data, fixup to "from src_gather" below.
        ttd.gather2outsrc_indices.push_back(si.src_index[setup_info_index]);
        ttd.outsrc_indices.push_back(i);
    }

    // Need to know src_gather index given NrnThread._data index
    // to compute gather2outsrc_indices. And the update outsrc_indices so that
    // for a given thread
    // for j: outsrc_buf[outsrc_indices[j]] = src_gather[gather2outsrc_indices[j]]
    for (int tid = 0; tid < ngroup; ++tid) {
        NrnThread& nt = nrn_threads[tid];
        auto& ttd = transfer_thread_data_[tid];
        std::map<int, int> data2gather_indices;
        for (size_t i = 0; i < ttd.src_indices.size(); ++i) {
            data2gather_indices[ttd.src_indices[i]] = i;
        }

        for (size_t i = 0; i < ttd.outsrc_indices.size(); ++i) {
            ttd.gather2outsrc_indices[i] = data2gather_indices[ttd.gather2outsrc_indices[i]];
        }
    }

    // Which insrc_indices point into which NrnThread.data
    // An sgid occurs at most once in the process recv_from_have.
    // But it might get distributed to more than one thread and to
    // several targets in a thread (specified by tar2info)
    // insrc_indices is parallel to tar_indices and has size ntar of the thread.
    // insrc_indices[i] is the index into insrc_buf
    // tar_indices[i] is the index into NrnThread.data
    // i.e. NrnThead._data[tar_indices[i]] = insrc_buf[insrc_indices[i]]
    for (int i = 0; i < insrcdspl_[nhost]; ++i) {
        sgid_t sgid = recv_from_have[i];
        SidInfo& sidinfo = tar2info[sgid];
        // there may be several items in the lists.
        for (size_t j = 0; j < sidinfo.tids_.size(); ++j) {
            int tid = sidinfo.tids_[j];
            int index = sidinfo.indices_[j];

            transfer_thread_data_[tid].insrc_indices[index] = i;
        }
    }

#if DEBUG
  // things look ok so far?
  for (int tid=0; tid < ngroup; ++tid) {
    nrn_partrans::SetupTransferInfo& si = setup_info_[tid];
    nrn_partrans::TransferThreadData& ttd = transfer_thread_data_[tid];
    for (size_t i=0; i < si.src_sid.size(); ++i) {
      printf("%d %d src sid=%d v_index=%d %g\n", nrnmpi_myid, tid, si.src_sid[i], ttd.src_indices[i], nrn_threads[tid]._data[ttd.src_indices[i]]);
    }
    for (size_t i=0; i < ttd.tar_indices.size(); ++i) {
      printf("%d %d src sid=i%z tar_index=%d %g\n", nrnmpi_myid, tid, i,
        ttd.tar_indices[i], nrn_threads[tid]._data[ttd.tar_indices[i]]);
    }
  }
#endif

    delete[] send_to_want;
    delete[] recv_from_have;
}

/**
 *  For now, until conceptualization of the ordering is clear,
 *  just replace src setup_info_ indices values with stdindex2ptr determined
 *  index into NrnThread._data
**/
void nrn_partrans::gap_data_indices_setup(NrnThread* n) {
    NrnThread& nt = *n;
    auto& ttd = transfer_thread_data_[nt.id];
    auto& sti = setup_info_[nt.id];

    ttd.src_gather.resize(sti.src_sid.size());
    ttd.src_indices.resize(sti.src_sid.size());
    ttd.insrc_indices.resize(sti.tar_sid.size());
    ttd.tar_indices.resize(sti.tar_sid.size());

    // For copying into src_gather from NrnThread._data
    for (size_t i = 0; i < sti.src_sid.size(); ++i) {
        double* d = stdindex2ptr(sti.src_type[i], sti.src_index[i], nt);
        sti.src_index[i] = int(d - nt._data);
    }

    // For copying into NrnThread._data from insrc_buf.
    for (size_t i = 0; i < sti.tar_sid.size(); ++i) {
        double* d = stdindex2ptr(sti.tar_type[i], sti.tar_index[i], nt);
        // todo : this should be revisited once nt._data will be broken
        // into mechanism specific data
        sti.tar_index[i] = int(d - nt._data);
    }

    // Here we could reorder sti.src_... according to NrnThread._data index
    // order

    // copy into TransferThreadData
    ttd.src_indices = sti.src_index;
    ttd.tar_indices = sti.tar_index;
}

void nrn_partrans::gap_cleanup() {
    if (transfer_thread_data_) {
        delete [] transfer_thread_data_; transfer_thread_data_ = nullptr;
    }
    if (insrc_buf_) {
        delete [] insrc_buf_; insrc_buf_ = nullptr;
        delete [] insrccnt_; insrccnt_ = nullptr;
        delete [] insrcdspl_; insrcdspl_ = nullptr;   
        delete [] outsrc_buf_; outsrc_buf_ = nullptr;  
        delete [] outsrccnt_; outsrccnt_ = nullptr;
        delete [] outsrcdspl_; outsrcdspl_ = nullptr;
    }
}

}  // namespace coreneuron
