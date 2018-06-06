#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrniv/partrans.h"
#include <map>
#include <vector>
namespace coreneuron {
using namespace coreneuron::nrn_partrans;

nrn_partrans::SetupInfo* nrn_partrans::setup_info_;

class SidData {
  public:
    std::vector<int> tids_;
    std::vector<int> indices_;
};

}  // namespace coreneuron
#if NRNLOGSGID
#define sgid_alltoallv nrnmpi_long_alltoallv
#else
#define sgid_alltoallv nrnmpi_int_alltoallv
#endif

#define HAVEWANT_t sgid_t
#define HAVEWANT_alltoallv sgid_alltoallv
#define HAVEWANT2Int std::map<sgid_t, int>
#include "coreneuron/nrniv/have2want.h"

namespace coreneuron {
using namespace coreneuron::nrn_partrans;
nrn_partrans::TransferThreadData::TransferThreadData() {
    halfgap_ml = NULL;
    nsrc = 0;
    ntar = 0;
    insrc_indices = NULL;
    v_indices = NULL;
    outbuf_indices = NULL;
    v_gather = NULL;
}

nrn_partrans::TransferThreadData::~TransferThreadData() {
    if (insrc_indices) {
        delete[] insrc_indices;
    }
    if (v_indices) {
        delete[] v_indices;
    }
    if (outbuf_indices) {
        delete[] outbuf_indices;
    }
    if (v_gather) {
        delete[] v_gather;
    }
}

void nrn_partrans::gap_mpi_setup(int ngroup) {
    // printf("%d gap_mpi_setup ngroup=%d\n", nrnmpi_myid, ngroup);

    // This can happen until bug is fixed. ie. if one process has more than
    // one thread then all processes must have more than one thread
    if (ngroup < nrn_nthread) {
        transfer_thread_data_[ngroup].nsrc = 0;
        transfer_thread_data_[ngroup].halfgap_ml = NULL;
    }

    // create and fill halfgap_info using first available...
    halfgap_info = new HalfGap_Info;
    HalfGap_Info& hgi = *halfgap_info;
    for (int tid = 0; tid < ngroup; ++tid) {
        nrn_partrans::SetupInfo& si = setup_info_[tid];
        if (si.ntar) {
            hgi.ix_vpre = si.ix_vpre;
            hgi.type = si.type;
            hgi.sz = nrn_prop_param_size_[hgi.type];
            hgi.layout = nrn_mech_data_layout_[hgi.type];
        }
    }

    // count total_nsrc, total_ntar and allocate (total_ntar too large but...)
    int total_nsrc = 0, total_ntar = 0;
    for (int tid = 0; tid < ngroup; ++tid) {
        nrn_partrans::SetupInfo& si = setup_info_[tid];
        total_nsrc += si.nsrc;
        total_ntar += si.ntar;
    }

    // have and want arrays
    sgid_t* have = new sgid_t[total_nsrc];
    sgid_t* want = new sgid_t[total_ntar];  // more than needed

    // map from sid_src to (tid, index) into v_indices
    // and sid_target to lists of (tid, index) for memb_list
    // also count the map sizes and fill have and want arrays
    std::map<sgid_t, SidData> src2data;
    std::map<sgid_t, SidData> tar2data;
    int src2data_size = 0, tar2data_size = 0;  // number of unique sids
    for (int tid = 0; tid < ngroup; ++tid) {
        SetupInfo& si = setup_info_[tid];
        for (int i = 0; i < si.nsrc; ++i) {
            sgid_t sid = si.sid_src[i];
            SidData sd;
            sd.tids_.push_back(tid);
            sd.indices_.push_back(i);
            src2data[sid] = sd;
            have[src2data_size] = sid;
            src2data_size++;
        }
        for (int i = 0; i < si.ntar; ++i) {
            sgid_t sid = si.sid_target[i];
            if (tar2data.find(sid) == tar2data.end()) {
                SidData sd;
                tar2data[sid] = sd;
                want[tar2data_size] = sid;
                tar2data_size++;
            }
            SidData& sd = tar2data[sid];
            sd.tids_.push_back(tid);
            sd.indices_.push_back(i);
        }
    }

    // 2) Call the have_to_want function.
    sgid_t* send_to_want;
    sgid_t* recv_from_have;

    have_to_want(have, src2data_size, want, tar2data_size, send_to_want, outsrccnt_, outsrcdspl_,
                 recv_from_have, insrccnt_, insrcdspl_, default_rendezvous);

    int nhost = nrnmpi_numprocs;

    // sanity check. all the sgids we are asked to send, we actually have
    for (int i = 0; i < outsrcdspl_[nhost]; ++i) {
        sgid_t sgid = send_to_want[i];
        assert(src2data.find(sgid) != src2data.end());
    }

    // sanity check. all the sgids we receive, we actually need.
    for (int i = 0; i < insrcdspl_[nhost]; ++i) {
        sgid_t sgid = recv_from_have[i];
        assert(tar2data.find(sgid) != tar2data.end());
    }

#if 0
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

    // count and allocate transfer_thread_data arrays.
    for (int tid = 0; tid < ngroup; ++tid) {
        transfer_thread_data_[tid].nsrc = 0;
    }
    for (int i = 0; i < outsrcdspl_[nhost]; ++i) {
        sgid_t sgid = send_to_want[i];
        SidData& sd = src2data[sgid];
        // only one item in the lists.
        int tid = sd.tids_[0];
        transfer_thread_data_[tid].nsrc += 1;
    }
    for (int tid = 0; tid < ngroup; ++tid) {
        nrn_partrans::SetupInfo& si = setup_info_[tid];
        nrn_partrans::TransferThreadData& ttd = transfer_thread_data_[tid];
        ttd.v_indices = new int[ttd.nsrc];
        ttd.v_gather = new double[ttd.nsrc];
        ttd.outbuf_indices = new int[ttd.nsrc];
        ttd.nsrc = 0;  // recount below as filled
        ttd.ntar = si.ntar;
        ttd.insrc_indices = new int[si.ntar];
    }

    // fill thread actual_v to send arrays. (offsets and layout later).
    for (int i = 0; i < outsrcdspl_[nhost]; ++i) {
        sgid_t sgid = send_to_want[i];
        SidData& sd = src2data[sgid];
        // only one item in the lists.
        int tid = sd.tids_[0];
        int index = sd.indices_[0];

        nrn_partrans::SetupInfo& si = setup_info_[tid];
        nrn_partrans::TransferThreadData& ttd = transfer_thread_data_[tid];

        ttd.v_indices[ttd.nsrc] = si.v_indices[index];
        ttd.outbuf_indices[ttd.nsrc] = i;
        ttd.nsrc += 1;
    }

    // fill thread receive to vpre arrays. (offsets and layout later).
    for (int i = 0; i < insrcdspl_[nhost]; ++i) {
        sgid_t sgid = recv_from_have[i];
        SidData& sd = tar2data[sgid];
        // there may be several items in the lists.
        for (unsigned j = 0; j < sd.tids_.size(); ++j) {
            int tid = sd.tids_[j];
            int index = sd.indices_[j];

            transfer_thread_data_[tid].insrc_indices[index] = i;
        }
    }

#if 0
  // things look ok so far?
  for (int tid=0; tid < ngroup; ++tid) {
    nrn_partrans::SetupInfo& si = setup_info_[tid];
    nrn_partrans::TransferThreadData& ttd = transfer_thread_data_[tid];
    for (int i=0; i < si.nsrc; ++i) {
      printf("%d %d src sid=%d v_index=%d\n", nrnmpi_myid, tid, si.sid_src[i], si.v_indices[i]);
    }
    for (int i=0; i < si.ntar; ++i) {
      printf("%d %d tar sid=%d i=%d\n", nrnmpi_myid, tid, si.sid_target[i], i);
    }
    for (int i=0; i < ttd.nsrc; ++i) {
      printf("%d %d src i=%d v_index=%d\n", nrnmpi_myid, tid, i, ttd.v_indices[i]);
    }
    for (int i=0; i < ttd.ntar; ++i) {
      printf("%d %d tar i=%d insrc_index=%d\n", nrnmpi_myid, tid, i, ttd.insrc_indices[i]);
    }
  }
#endif

    // cleanup
    for (int tid = 0; tid < ngroup; ++tid) {
        SetupInfo& si = setup_info_[tid];
        delete[] si.sid_src;
        delete[] si.v_indices;
        delete[] si.sid_target;
    }
    delete[] send_to_want;
    delete[] recv_from_have;
    delete[] setup_info_;
}

void nrn_partrans::gap_thread_setup(NrnThread& nt) {
    // printf("%d gap_thread_setup tid=%d\n", nrnmpi_myid, nt.id);
    nrn_partrans::TransferThreadData& ttd = transfer_thread_data_[nt.id];

    ttd.halfgap_ml = nt._ml_list[halfgap_info->type];
#if 0
  int ntar = ttd.halfgap_ml->nodecount;
  assert(ntar == ttd.ntar);
  int sz =halfgap_info->sz;

  for (int i=0; i < ntar; ++i) {
    ttd.insrc_indices[i] += sz;
  }
#endif
}

void nrn_partrans::gap_indices_permute(NrnThread& nt) {
    printf("nrn_partrans::gap_indices_permute\n");
    nrn_partrans::TransferThreadData& ttd = transfer_thread_data_[nt.id];
    // sources
    if (ttd.nsrc > 0 && nt._permute) {
        int n = ttd.nsrc;
        int* iv = ttd.v_indices;
        int* ip = nt._permute;
        // iv starts out as indices into unpermuted node array. That node
        // was permuted to index ip
        for (int i = 0; i < n; ++i) {
            iv[i] = ip[iv[i]];
        }
    }
    // now the outsrc_buf_ is invariant under any node permutation,
    // and, consequently, so is the insrc_buf_.

    // targets
    if (ttd.halfgap_ml && ttd.halfgap_ml->_permute) {
        int n = ttd.halfgap_ml->nodecount;
        int* ip = ttd.halfgap_ml->_permute;
        int* isi = ttd.insrc_indices;
        // halfgap has been permuted according to ip.
        // so old index value needs to be put into the new location.
        int* oldisi = new int[n];
        for (int i = 0; i < n; ++i) {
            oldisi[i] = isi[i];
        }
        for (int i = 0; i < n; ++i) {
            isi[ip[i]] = oldisi[i];
        }
        delete[] oldisi;
    }
}
}  // namespace coreneuron
