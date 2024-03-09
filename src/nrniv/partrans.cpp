#include <../../nrnconf.h>
#include "partrans.h"  // sgid_t and SetupTransferInfo for CoreNEURON

#include "neuron/container/data_handle.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <InterViews/resource.h>
#include <nrnoc2iv.h>
#include <nrniv_mf.h>
#include <nrnmpi.h>
#include <mymath.h>
#include <stdint.h>

#include <complex>
#include <unordered_map>  // Replaces NrnHash for MapSgid2Int
#include <utility>
#include <vector>

#if NRNMPI
#include "have2want.hpp"
#endif


#if NRNLONGSGID
#if NRNMPI
static void sgid_alltoallv(Data<sgid_t>& s, Data<sgid_t>& r) {
    if (nrn_sparse_partrans > 0) {
        nrnmpi_long_alltoallv_sparse(s.data.data(),
                                     s.cnt.data(),
                                     s.displ.data(),
                                     r.data.data(),
                                     r.cnt.data(),
                                     r.displ.data());
    } else {
        nrnmpi_long_alltoallv(s.data.data(),
                              s.cnt.data(),
                              s.displ.data(),
                              r.data.data(),
                              r.cnt.data(),
                              r.displ.data());
    }
}
#endif  // NRNMPI
#else   // not NRNLONGSGID
#if NRNMPI
static void sgid_alltoallv(Data<sgid_t>& s, Data<sgid_t>& r) {
    if (nrn_sparse_partrans > 0) {
        nrnmpi_int_alltoallv_sparse(s.data.data(),
                                    s.cnt.data(),
                                    s.displ.data(),
                                    r.data.data(),
                                    r.cnt.data(),
                                    r.displ.data());
    } else {
        nrnmpi_int_alltoallv(s.data.data(),
                             s.cnt.data(),
                             s.displ.data(),
                             r.data.data(),
                             r.cnt.data(),
                             r.displ.data());
    }
}
#endif  // NRNMPI
#endif  // not NRNLONGSGID

extern const char* bbcore_write_version;
// see lengthy comment in ../nrnoc/fadvance.cpp
// nrnmpi_v_transfer requires existence of nrnthread_v_transfer even if there
// is only one thread.
// Thread 0 does the nrnmpi_v_transfer into incoming_src_buf.
// Data destined for targets in thread owned memory
// is copied to the proper place by each thread via nrnthread_v_transfer
// MPI_Alltoallv is used to transfer interprocessor data.
// The basic assumption is that this will be mostly used for gap junctions in which
// most often one source voltage goes to one target or at least only a few targets.
// Note that the source data for nrnthread_v_transfer is in incoming_src_buf,
// source locations owned by thread, and source locations owned by other threads.

/*

16-5-2014
Gap junctions with extracellular require that the voltage source be v + vext.

A solution to the v+vext problem is to create a thread specific source
value buffer just for extracellular nodes.
 NODEV(_nd) + _nd->extnode->v[0] .
 That is, if there is no extracellular the ttd.sv and and poutsrc_
pointers stay exactly the same.  Whereas, if there is extracellular,
those would point into the source value buffer of the correct thread.
 Of course, it is necessary that the source value buffer be computed
prior to either parallel transfer or mpi_transfer.  Note that some
sources that are needed by another thread may not be needed by mpi and
vice versa.  For the fixed step method, mpi transfer occurs prior to
thread transfer.  For global variable step methods (cvode and lvardt do
not work with extracellular anyway):
 1) for multisplit, mpi between ms_part3 and ms_part4
 2) not multisplit, mpi between transfer_part1 and transfer_part2
 with thread transfer in ms_part4 and transfer_part2.
 Therefore it is possible to do the v+vext computation at the beginning
of mpi transfer except that mpi_transfer is not called if there is only
one process.  Also, this would be very cache inefficient for threads
since mpi transfer is done only by thread 0.

Therefore we need yet another callback.
 void* nrnthread_vi_compute(NrnThread*)
 called, if needed, at the end of update(nt) and before mpi transfer in
nrn_finitialize.

*/

/*

29-9-2016

Want to allow a source to be any range variable at a node or in a
Point_process.  e.g an ion concentration.  The v+vext change restricted
sources to voltage partly in order to simplify pointer recalculation for
cache efficiency.  If the source is not voltage, the calling context of
pc.source_var must be sufficient to easily store enough info to update
the pointer for cache efficiency or changed number of threads.  ie.  in
general, (Node*, mechtype, parray_index).  If the variable is a member
of a Point_process, it would be simple to pass it as the third arg and
store the reference.  However, we reject that due to not handling the
typical case of concentration.  So we impose the limitation that
structural changes that destroy the Node or move the pointer to another
Node generally require clearing and re-declaring the source, sid, target
relations, (nseg change, point process relocation).  Which is, in fact,
the current limitation on voltage sources.  The substantive
implementation change is to provide a safe pointer update for when we
know the Node*.  An extra map<sgid_t ssid, pair<int mechtype, int
parray_index> > should do nicely.  with voltages NOT being in the map.
Also a decent error message can be generated.  Finally, it is fairly
clear how to make this work with coreneuron by perhaps adding a triple
of sid, mechtype, parray_index integer vectors to the <gidgroup>_gap.dat
file.

*/

extern void (*nrnthread_v_transfer_)(NrnThread*);  // before nonvint and BEFORE INITIAL
extern void (*nrnthread_vi_compute_)(NrnThread*);
extern void (*nrnmpi_v_transfer_)();  // before nrnthread_v_transfer and after update. Called by
                                      // thread 0.
extern void (*nrn_mk_transfer_thread_data_)();
#if NRNMPI
extern double nrnmpi_transfer_wait_;
#endif

struct TransferThreadData {
    int cnt;
    std::vector<neuron::container::data_handle<double>> tv;  // pointers to the
                                                             // ParallelContext.target_var
    std::vector<neuron::container::data_handle<double>> sv;  // pointers to the
                                                             // ParallelContext.source_var (or into
                                                             // MPI target buffer)
};
static TransferThreadData* transfer_thread_data_;
static int n_transfer_thread_data_;

// for the case where we need vi = v + vext as the source voltage
struct SourceViBuf {
    int cnt;
    std::vector<Node*> nd;
    std::vector<double> val;
};
static std::vector<SourceViBuf> source_vi_buf_;
typedef std::unordered_map<sgid_t, int> MapSgid2Int;
typedef std::vector<Node*> NodePList;
#define PPList partrans_PPList
typedef std::vector<Point_process*> PPList;
typedef std::vector<int> IntList;
typedef std::vector<sgid_t> SgidList;

static double* insrc_buf_;  // Receives the interprocessor data destined for other threads.
static double* outsrc_buf_;
static std::vector<neuron::container::data_handle<double>> poutsrc_;  // prior to mpi copy src value
                                                                      // to proper place in
                                                                      // outsrc_buf_
static int* poutsrc_indices_;                                         // for recalc pointers
static int insrc_buf_size_;
static std::vector<int> insrccnt_;
static std::vector<int> insrcdspl_;
static int outsrc_buf_size_;
static std::vector<int> outsrccnt_;
static std::vector<int> outsrcdspl_;
static MapSgid2Int sid2insrc_;  // received interprocessor sid data is
// associated with which insrc_buf index. Created by nrnmpi_setup_transfer
// and used by mk_ttd

// ordered by calls to nrnmpi_target_var()
static std::vector<neuron::container::data_handle<double>> targets_;  // list of target variables
static SgidList sgid2targets_;                                        // list of target sgid
static PPList target_pntlist_;  // list of target Point_process

// ordered by calls to nrnmpi_source_var()
static NodePList visources_;        // list of source Node*, (multiples possible)
static SgidList sgids_;             // source gids
static MapSgid2Int sgid2srcindex_;  // sgid2srcindex[sgids[i]] == i

// source ssid -> (type,parray_index)
static std::unordered_map<sgid_t, std::pair<int, neuron::container::field_index>>
    non_vsrc_update_info_;

static int max_targets_;
static bool is_setup_;

// deleted when setup_transfer called
// defined persistently when pargap_jacobi_setup(0) called.
static int imped_current_type_count_;
static int* imped_current_type_;
static Memb_list** imped_current_ml_;

static void delete_imped_info() {
    if (imped_current_type_count_) {
        imped_current_type_count_ = 0;
        delete[] imped_current_type_;
        delete[] imped_current_ml_;
    }
}

// pv2node extended to any range variable in the section
// This helper searches over all the mechanisms in the node.
// If h refers to a RANGE variable inside a mechanism in the Node, store the mechanism type, the
// index of the RANGE variable, and the array index into that RANGE variable.
static bool non_vsrc_setinfo(sgid_t ssid,
                             Node* nd,
                             neuron::container::data_handle<double> const& h) {
    for (Prop* p = nd->prop; p; p = p->next) {
        for (auto i = 0; i < p->param_num_vars(); ++i) {
            for (auto j = 0; j < p->param_array_dimension(i); ++j) {
                if (h == p->param_handle(i, j)) {
                    non_vsrc_update_info_[ssid] = {p->_type, {i, j}};
                    return true;
                }
            }
        }
    }
    return false;
}

static neuron::container::data_handle<double> non_vsrc_update(Node* nd,
                                                              int type,
                                                              neuron::container::field_index ix) {
    for (Prop* p = nd->prop; p; p = p->next) {
        if (type == p->_type) {
            return p->param_handle(ix);
        }
    }
    hoc_execerr_ext("partrans update: could not find parameter index (%d, %d) of %s",
                    ix.field,
                    ix.array_index,
                    memb_func[type].sym->name);
}

// Find the Node associated with the voltage.
// Easy if v in the currently accessed section.
// Extended to any pointer to range variable in the section.
// If not a voltage save pv associated with mechtype, p_array_index
// in non_vsrc_update_info_
static Node* pv2node(sgid_t ssid, neuron::container::data_handle<double> const& v) {
    Section* const sec = chk_access();
    if (auto* const nd = sec->parentnode; nd) {
        if (v == nd->v_handle() || non_vsrc_setinfo(ssid, nd, v)) {
            return nd;
        }
    }
    for (int i = 0; i < sec->nnode; ++i) {
        auto* const nd = sec->pnode[i];
        if (v == nd->v_handle() || non_vsrc_setinfo(ssid, nd, v)) {
            return nd;
        }
    }
    hoc_execerr_ext("Pointer to src is not in the currently accessed section %s", secname(sec));
}

static void thread_transfer(NrnThread* _nt);
void nrnmpi_source_var() {
    nrnthread_v_transfer_ = thread_transfer;  // otherwise can't check is_setup_
    is_setup_ = false;
    // Get the source variable pointer and promote it to a data_handle if
    // possible (i.e. if it is a Node voltage, for the moment)
    auto const psv = hoc_hgetarg<double>(1);
    auto const sgid = []() -> sgid_t {
        double const x{*hoc_getarg(2)};
        if (x < 0) {
            hoc_execerr_ext("source_var sgid must be >= 0: arg 2 is %g\n", x);
        }
        return x;
    }();
    auto const [_, inserted] = sgid2srcindex_.emplace(sgid, visources_.size());
    if (!inserted) {
        hoc_execerr_ext("source var sgid %lld already in use.", (long long) sgid);
    }
    visources_.push_back(pv2node(sgid, psv));
    sgids_.push_back(sgid);
    // printf("nrnmpi_source_var %p source_val=%g sgid=%ld\n", psv, *psv, (long)sgid);
}

void nrnmpi_target_var() {
    Point_process* pp{};
    Object* ob{};
    int iarg{1};
    nrnthread_v_transfer_ = thread_transfer;  // otherwise can't check is_setup_
    is_setup_ = false;
    if (hoc_is_object_arg(iarg)) {
        ob = *hoc_objgetarg(iarg++);
        pp = ob2pntproc(ob);
    }
    auto ptv = hoc_hgetarg<double>(iarg++);
    double x = *hoc_getarg(iarg++);
    if (x < 0) {
        hoc_execerr_ext("target_var sgid must be >= 0: arg %d is %g\n", iarg - 1, x);
    }
    if (pp && !pp->prop->owns(ptv)) {
        hoc_execerr_ext("Target ref not in %s", hoc_object_name(ob));
    }
    auto const sgid = static_cast<sgid_t>(x);
    targets_.push_back(ptv);
    target_pntlist_.push_back(pp);
    sgid2targets_.push_back(sgid);
    // printf("nrnmpi_target_var %p target_val=%g sgid=%ld\n", ptv, *ptv, (long)sgid);
}

static void rm_ttd() {
    if (!transfer_thread_data_) {
        return;
    }
    delete[] std::exchange(transfer_thread_data_, nullptr);
    n_transfer_thread_data_ = 0;
    nrnthread_v_transfer_ = nullptr;
}

static void rm_svibuf() {
    if (source_vi_buf_.empty()) {
        return;
    }
    source_vi_buf_.clear();
    nrnthread_vi_compute_ = nullptr;
}

static void thread_vi_compute(NrnThread* _nt);
static std::unordered_map<Node*, double*> mk_svibuf() {
    rm_svibuf();
    if (visources_.empty()) {
        return {};
    }
    // any use of extracellular?
    int has_ecell = 0;
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        if (nrn_threads[tid]._ecell_memb_list) {
            has_ecell = 1;
            break;
        }
    }
    if (!has_ecell) {
        return {};
    }

    source_vi_buf_.resize(nrn_nthread);

    for (int tid = 0; tid < nrn_nthread; ++tid) {
        source_vi_buf_[tid].cnt = 0;
    }
    // count
    for (size_t i = 0; i < visources_.size(); ++i) {
        Node* nd = visources_[i];
        auto const it = non_vsrc_update_info_.find(sgids_[i]);
        if (nd->extnode && it == non_vsrc_update_info_.end()) {
            assert(nd->_nt >= nrn_threads && nd->_nt < (nrn_threads + nrn_nthread));
            ++source_vi_buf_[nd->_nt->id].cnt;
        }
    }
    // allocate
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        SourceViBuf& svib = source_vi_buf_[tid];
        svib.nd.resize(svib.cnt);
        svib.val.resize(svib.cnt);
        svib.cnt = 0;  // recount on fill
    }
    // fill
    for (size_t i = 0; i < visources_.size(); ++i) {
        Node* nd = visources_[i];
        auto const it = non_vsrc_update_info_.find(sgids_[i]);
        if (nd->extnode && it == non_vsrc_update_info_.end()) {
            int tid = nd->_nt->id;
            SourceViBuf& svib = source_vi_buf_[tid];
            svib.nd[svib.cnt++] = nd;
        }
    }
    // now the only problem is how to get TransferThreadData and poutsrc_
    // to point to the proper SourceViBuf given that sgid2srcindex
    // only gives us the Node* and we dont want to search linearly
    // (during setup) everytime we we want to associate.
    // We can do the poutsrc_ now by creating a temporary Node* to
    // double* map .. The TransferThreadData can be done later
    // in mk_ttd using the same map and then deleted.
    std::unordered_map<Node*, double*> ndvi2pd{1000};
    // TODO can this be handle-ified?
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        SourceViBuf& svib = source_vi_buf_[tid];
        assert(svib.nd.size() == svib.cnt);
        assert(svib.val.size() == svib.cnt);
        for (int i = 0; i < svib.cnt; ++i) {
            Node* nd = svib.nd[i];
            ndvi2pd[nd] = &svib.val[i];  // pointer to a vector owned by source_vi_buf_
        }
    }
    for (int i = 0; i < outsrc_buf_size_; ++i) {
        int isrc = poutsrc_indices_[i];
        Node* nd = visources_[isrc];
        auto const it = non_vsrc_update_info_.find(sgids_[isrc]);
        if (nd->extnode && it == non_vsrc_update_info_.end()) {
            auto const search = ndvi2pd.find(nd);
            nrn_assert(search != ndvi2pd.end());
            // olupton 2022-11-28: looks like search->second is always a pointer to a private vector
            // in source_vi_buf_, so do_not_search makes sense.
            poutsrc_[i] = neuron::container::data_handle<double>{neuron::container::do_not_search,
                                                                 search->second};
        }
    }
    nrnthread_vi_compute_ = thread_vi_compute;
    return ndvi2pd;
}

static void mk_ttd() {
    int i, j, tid, n;
    auto ndvi2pd = mk_svibuf();
    rm_ttd();
    if (targets_.empty()) {
        // some MPI transfer code paths require that all ranks
        // have a nrn_thread_v_transfer.
        // As mentioned in http://static.msi.umn.edu/tutorial/scicomp/general/MPI/content3_new.html
        // "Communications may, or may not, be synchronized,
        // depending on how the vendor chose to implement them."
        // In particular the BG/Q (and one other machine) is sychronizing.
        // (but see: http://www-01.ibm.com/support/docview.wss?uid=isg1IZ58190 )
        if (nrnmpi_numprocs > 1 && max_targets_) {
            nrnthread_v_transfer_ = thread_transfer;
        }
        return;
    }
    n = targets_.size();
    if (nrn_nthread > 1)
        for (i = 0; i < n; ++i) {
            Point_process* pp = target_pntlist_[i];
            int sgid = sgid2targets_[i];
            if (!pp) {
                hoc_execerr_ext(
                    "Do not know the POINT_PROCESS target for source id %lld\n"
                    "For multiple threads, the target pointer must reference a range variable\n"
                    "of a POINT_PROCESS. Note that even for a single thread, it is\n"
                    "fastest to supply a reference to the POINT_PROCESS as the first arg.",
                    (long long) sgid);
            }
        }
    transfer_thread_data_ = new TransferThreadData[nrn_nthread];
    for (tid = 0; tid < nrn_nthread; ++tid) {
        transfer_thread_data_[tid].cnt = 0;
    }
    n_transfer_thread_data_ = nrn_nthread;
    // how many targets in each thread
    if (nrn_nthread == 1) {
        transfer_thread_data_[0].cnt = target_pntlist_.size();
    } else {
        for (i = 0; i < n; ++i) {
            assert(target_pntlist_[i]);
            tid = ((NrnThread*) target_pntlist_[i]->_vnt)->id;
            ++transfer_thread_data_[tid].cnt;
        }
    }
    // allocate
    for (tid = 0; tid < nrn_nthread; ++tid) {
        TransferThreadData& ttd = transfer_thread_data_[tid];
        if (ttd.cnt) {
            ttd.tv.resize(ttd.cnt);
            ttd.sv.resize(ttd.cnt);
        }
        ttd.cnt = 0;
    }
    // count again and fill pointers
    for (i = 0; i < n; ++i) {
        if (nrn_nthread == 1) {
            tid = 0;
        } else {
            tid = ((NrnThread*) target_pntlist_[i]->_vnt)->id;
        }
        TransferThreadData& ttd = transfer_thread_data_[tid];
        j = ttd.cnt++;
        ttd.tv.at(j) = targets_.at(i);
        // perhaps inter- or intra-thread, perhaps interprocessor
        // if inter- or intra-thread, perhaps SourceViBuf
        sgid_t sid = sgid2targets_[i];
        // cannot figure out how to get iterator and test within if
        bool err = true;
        auto search = sgid2srcindex_.find(sid);
        if (search != sgid2srcindex_.end()) {
            err = false;
            Node* nd = visources_[search->second];
            auto it = non_vsrc_update_info_.find(sid);
            if (it != non_vsrc_update_info_.end()) {
                ttd.sv[j] = non_vsrc_update(nd, it->second.first, it->second.second);
            } else if (nd->extnode) {
                auto search = ndvi2pd.find(nd);
                nrn_assert(search != ndvi2pd.end());
                // olupton 2022-11-28: looks like search->second is always a pointer to a private
                // vector in source_vi_buf_, so do_not_search makes sense.
                ttd.sv[j] = neuron::container::data_handle<double>{neuron::container::do_not_search,
                                                                   search->second};
            } else {
                ttd.sv[j] = nd->v_handle();
            }
        } else {
            auto search = sid2insrc_.find(sid);
            if (search != sid2insrc_.end()) {
                err = false;
                // olupton 2022-11-28: insrc_buf_ is not part of the global model data structure, so
                // do_not_search is appropriate
                ttd.sv[j] = neuron::container::data_handle<double>{neuron::container::do_not_search,
                                                                   insrc_buf_ + search->second};
            }
        }
        if (err == true) {
            hoc_execerr_ext("No source_var for target_var sid = %lld\n", (long long) sid);
        }
    }
    nrnthread_v_transfer_ = thread_transfer;
}

static void thread_vi_compute(NrnThread* _nt) {
    // vi+vext needed by either mpi or thread transfer copied into
    // the source value buffer for this thread. Note that relevant
    // poutsrc_ and ttd[_nt->id].sv items
    // point into this source value buffer
    if (source_vi_buf_.empty()) {
        return;
    }
    SourceViBuf& svb = source_vi_buf_[_nt->id];
    for (int i = 0; i < svb.cnt; ++i) {
        Node* nd = svb.nd[i];
        assert(nd->extnode);
        svb.val[i] = NODEV(nd) + nd->extnode->v[0];
    }
}

static void mpi_transfer() {
    int i, n = outsrc_buf_size_;
    for (i = 0; i < n; ++i) {
        outsrc_buf_[i] = *poutsrc_[i];
    }
#if NRNMPI
    if (nrnmpi_numprocs > 1) {
        double wt = nrnmpi_wtime();
        if (nrn_sparse_partrans > 0) {
            nrnmpi_dbl_alltoallv_sparse(outsrc_buf_,
                                        outsrccnt_.data(),
                                        outsrcdspl_.data(),
                                        insrc_buf_,
                                        insrccnt_.data(),
                                        insrcdspl_.data());
        } else {
            nrnmpi_dbl_alltoallv(outsrc_buf_,
                                 outsrccnt_.data(),
                                 outsrcdspl_.data(),
                                 insrc_buf_,
                                 insrccnt_.data(),
                                 insrcdspl_.data());
        }
        nrnmpi_transfer_wait_ += nrnmpi_wtime() - wt;
        errno = 0;
    }
#endif
    // insrc_buf_ will get transferred to targets by thread_transfer
}

static void thread_transfer(NrnThread* _nt) {
    if (!is_setup_) {
        hoc_execerror("ParallelContext.setup_transfer()", "needs to be called.");
    }
    if (targets_.empty()) {
        return;
    }
    // an edited old comment prior to allowing simultaneous threads and mpi.
    // for threads we do direct transfers under the assumption
    // that v is being transferred and they were set in a
    // previous multithread job. For the fixed step method this
    // call is from nonvint which in the same thread job as update
    // and that is the case even with multisplit. So we really
    // need to break the job between update and nonvint. Too bad.
    // For global cvode, things are ok except if the source voltage
    // is at a zero area node since nocap_v_part2 is a part
    // of this job and in fact the v does not get updated til
    // the next job in nocap_v_part3. Again, too bad. But it is
    // quite ambiguous, stability wise,
    // to have a gap junction in a zero area node, anyway, since
    // the system is then truly a DAE.
    // For now we presume we have dealt with these matters and
    // do the transfer.
    assert(n_transfer_thread_data_ == nrn_nthread);
    TransferThreadData& ttd = transfer_thread_data_[_nt->id];
    for (int i = 0; i < ttd.cnt; ++i) {
        *(ttd.tv[i]) = *(ttd.sv[i]);
    }
}

// The simplest conceivable transfer is to use MPI_Allgatherv and send
// all sources to all machines. More complicated and possibly more efficient
// in terms of total received buffer size
// would be to use MPI_Alltoallv in which distinct data is sent and received.
// Most transfer are one to one, at most one to a few, so now we use alltoallv.
// The old comment read: "
// We begin with MPI_Allgatherv. We try
// to save a trivial copy by making
// outgoing_source_buf a pointer into the incoming_source_buf.
// "  But this was a mistake as many mpi implementations do not allow overlap
// of send and receive buffers.

void nrnmpi_setup_transfer() {
#if !NRNMPI
    if (nrnmpi_numprocs > 1) {
        hoc_execerror(
            "To use ParallelContext.setup_transfer when nhost > 1, NEURON must be configured with "
            "--with-paranrn",
            0);
    }
#endif
    int nhost = nrnmpi_numprocs;
    is_setup_ = true;
    delete_imped_info();
    delete[] std::exchange(insrc_buf_, nullptr);
    delete[] std::exchange(outsrc_buf_, nullptr);
    outsrc_buf_size_ = 0;
    sid2insrc_.clear();
    poutsrc_.clear();
    delete[] std::exchange(poutsrc_indices_, nullptr);
#if NRNMPI
    // if there are no targets anywhere, we do not need to do anything
    max_targets_ = nrnmpi_int_allmax(targets_.size());
    if (max_targets_ == 0) {
        return;
    }
    if (nrnmpi_numprocs > 1) {
        insrccnt_.clear();
        insrccnt_.shrink_to_fit();
        insrcdspl_.clear();
        insrcdspl_.shrink_to_fit();
        outsrccnt_.clear();
        outsrccnt_.shrink_to_fit();
        outsrcdspl_.clear();
        outsrcdspl_.shrink_to_fit();
        // This is an old comment prior to using the want_to_have rendezvous
        // rank function in want2have.cpp. The old method did not scale
        // to more sgids than could fit on a single rank, because
        // each rank sent its "need" list to every rank.
        // <old comment>
        // This machine needs to send which sources to which other machines.
        // It does not need to send to itself.
        // Which targets have sources on other machines.(none if nrnmpi_numprocs=1)
        // 1) list sources needed that are on other machines.
        // 2) send that info to all machines.
        // 3) source machine can figure out which machines want its sids
        //    and therefore construct outsrc_buf, etc.
        // 4) Notify target machines which sids the source machine will send
        // 5) The target machines can figure out where the sids are coming from
        //    and therefore construct insrc_buf, etc.
        // <new comment>
        // 1) List sources needed by this rank and sources that this rank owns.
        // 2) Call the have_to_want function. Returns two sets of three
        //    vectors. The first set of three vectors is an sgid buffer,
        //    along with counts and displacements. The sgids in the ith region
        //    of the buffer are the sgids from this rank that are
        //    wanted by the ith rank. For the second set, the sgids in the ith
        //    region are the sgids on the ith rank that are wanted by this rank.
        // 3) First return triple creates the proper outsrc_buf_.
        // 4) The second triple is creates the insrc_buf_.

        // 1)
        // It will often be the case that multiple targets will need the
        // same source. We count the needed sids only once regardless of
        // how often they are used.
        // At the end of this section, needsrc is an array of needsrc_cnt
        // sids needed by this machine. The 'seen' table values are unused
        // but the keys are all the (unique) sgid needed by this process.
        // At the end seen is in fact what we want for sid2insrc_.
        int szalloc = targets_.size();
        szalloc = szalloc ? szalloc : 1;

        // At the moment sid2insrc_ is serving as 'seen'
        sid2insrc_.clear();
        sid2insrc_.reserve(szalloc);  // for single counting
        std::vector<sgid_t> needsrc{};
        for (size_t i = 0; i < sgid2targets_.size(); ++i) {
            sgid_t sid = sgid2targets_[i];
            auto search = sid2insrc_.find(sid);
            if (search == sid2insrc_.end()) {
                sid2insrc_[sid] = 0;  // at the moment, value does not matter
                needsrc.push_back(sid);
            }
        }

        // 1 continued) Create an array of sources this rank owns.
        // This already exists as a vector in the SgidList sgids_ but
        // that is private so go ahead and copy.
        std::vector<sgid_t> ownsrc = sgids_;

        // 2) Call the have_to_want function.
        auto [send_to_want, recv_from_have] = have_to_want<sgid_t>(ownsrc, needsrc, sgid_alltoallv);

        // sanity check. all the sgids we are asked to send, we actually have
        int n = send_to_want.displ[nhost];
        // sanity check. all the sgids we receive, we actually need.
        // also set the sid2insrc_ value to the proper recv_from_have index.
        n = recv_from_have.displ[nhost];
        for (int i = 0; i < n; ++i) {
            sgid_t sgid = recv_from_have.data[i];
            nrn_assert(sid2insrc_.find(sgid) != sid2insrc_.end());
            sid2insrc_[sgid] = i;
        }

        // 3) First return triple creates the proper outsrc_buf_.
        // Now that we know what machines are interested in our sids...
        // construct outsrc_buf, outsrc_buf_size, outsrccnt_, outsrcdspl_
        // and poutsrc_;
        std::swap(outsrccnt_, send_to_want.cnt);
        std::swap(outsrcdspl_, send_to_want.displ);
        outsrc_buf_size_ = outsrcdspl_[nrnmpi_numprocs];
        szalloc = std::max(1, outsrc_buf_size_);
        outsrc_buf_ = new double[szalloc];
        poutsrc_.resize(szalloc);
        poutsrc_indices_ = new int[szalloc];
        for (int i = 0; i < outsrc_buf_size_; ++i) {
            sgid_t sid = send_to_want.data[i];
            auto search = sgid2srcindex_.find(sid);
            nrn_assert(search != sgid2srcindex_.end());
            Node* nd = visources_[search->second];
            auto const it = non_vsrc_update_info_.find(sid);
            if (it != non_vsrc_update_info_.end()) {
                poutsrc_[i] = non_vsrc_update(nd, it->second.first, it->second.second);
            } else if (!nd->extnode) {
                poutsrc_[i] = nd->v_handle();
            } else {
                // the v+vext case can only be done after mk_svib()
            }
            poutsrc_indices_[i] = search->second;
            outsrc_buf_[i] = double(sid);  // see step 5
        }

        // 4) The second triple is creates the insrc_buf_.
        // From the recv_from_have and sid2insrc_ table, construct the insrc...
        std::swap(insrccnt_, recv_from_have.cnt);
        std::swap(insrcdspl_, recv_from_have.displ);
        insrc_buf_size_ = insrcdspl_[nrnmpi_numprocs];
        szalloc = insrc_buf_size_ ? insrc_buf_size_ : 1;
        insrc_buf_ = new double[szalloc];
        // from sid2insrc_, mk_ttd can construct the right pointer to the source.

        nrnmpi_v_transfer_ = mpi_transfer;
    }
#endif  // NRNMPI
    nrn_mk_transfer_thread_data_ = mk_ttd;
    if (!v_structure_change) {
        mk_ttd();
    }
}

void nrn_partrans_clear() {
    nrnthread_v_transfer_ = nullptr;
    nrnthread_vi_compute_ = nullptr;
    nrnmpi_v_transfer_ = nullptr;
    sgid2srcindex_.clear();
    sgids_.resize(0);
    visources_.resize(0);
    sgid2targets_.resize(0);
    target_pntlist_.resize(0);
    targets_.clear();
    max_targets_ = 0;
    rm_svibuf();
    rm_ttd();
    delete[] std::exchange(insrc_buf_, nullptr);
    delete[] std::exchange(outsrc_buf_, nullptr);
    outsrc_buf_size_ = 0;
    sid2insrc_.clear();
    poutsrc_.clear();
    delete[] std::exchange(poutsrc_indices_, nullptr);
    non_vsrc_update_info_.clear();
    nrn_mk_transfer_thread_data_ = nullptr;
}

// assume one thread and no extracellular

static double *vgap1, *vgap2;
static int imped_change_cnt;

void pargap_jacobi_setup(int mode) {
    if (!nrnthread_v_transfer_) {
        return;
    }

    // list of gap junction types and memb_list for each
    if (mode == 0) {
        if (imped_change_cnt != structure_change_cnt) {
            delete_imped_info();
            imped_change_cnt = structure_change_cnt;
        }
        if (imped_current_type_count_ == 0 && !targets_.empty()) {
            for (size_t i = 0; i < targets_.size(); ++i) {
                Point_process* pp = target_pntlist_[i];
                if (!pp) {
                    hoc_execerror(
                        "For impedance, pc.target_var requires that its first arg be a reference "
                        "to the POINT_PROCESS",
                        0);
                }
                int type = pp->prop->_type;
                if (imped_current_type_count_ == 0) {
                    imped_current_type_count_ = 1;
                    imped_current_type_ = new int[5];
                    imped_current_ml_ = new Memb_list*[5];
                    imped_current_type_[0] = type;
                }
                int add = 1;
                for (int k = 0; k < imped_current_type_count_; ++k) {
                    if (type == imped_current_type_[k]) {
                        add = 0;
                        break;
                    }
                }
                if (add) {
                    assert(imped_current_type_count_ < 5);
                    imped_current_type_[imped_current_type_count_] = type;
                    imped_current_type_count_ += 1;
                }
            }
            NrnThread* nt = nrn_threads;
            for (int k = 0; k < imped_current_type_count_; ++k) {
                for (NrnThreadMembList* tml = nt->tml; tml; tml = tml->next) {
                    if (imped_current_type_[k] == tml->index) {
                        imped_current_ml_[k] = tml->ml;
                    }
                }
            }
            // are all the instances in use
            size_t ninst = 0;
            for (int k = 0; k < imped_current_type_count_; ++k) {
                ninst += imped_current_ml_[k]->nodecount;
            }
            if (ninst != targets_.size()) {
                hoc_execerr_ext(
                    "number of gap junctions, %zd, not equal to number of pc.transfer_var, %zd",
                    ninst,
                    targets_.size());
            }
        }
    }
    TransferThreadData* ttd = transfer_thread_data_;
    if (mode == 0) {  // setup
        if (visources_.size()) {
            vgap1 = new double[visources_.size()];
        }
        if (ttd && ttd->cnt) {
            vgap2 = new double[ttd->cnt];
        }
        for (size_t i = 0; i < visources_.size(); ++i) {
            vgap1[i] = NODEV(visources_[i]);
        }
        if (ttd)
            for (int i = 0; i < ttd->cnt; ++i) {
                vgap2[i] = *(ttd->tv[i]);
            }
    } else {  // tear down
        for (size_t i = 0; i < visources_.size(); ++i) {
            visources_[i]->v() = vgap1[i];
        }
        if (ttd) {
            for (int i = 0; i < ttd->cnt; ++i) {
                *(ttd->tv[i]) = vgap2[i];
            }
        }
        delete[] std::exchange(vgap1, nullptr);
        delete[] std::exchange(vgap2, nullptr);
    }
}

void pargap_jacobi_rhs(std::vector<std::complex<double>>& b,
                       const std::vector<std::complex<double>>& x) {
    // First loop for real, second for imag
    for (int real_imag = 0; real_imag < 2; ++real_imag) {
        // helper for complex impedance with parallel gap junctions
        // b = b - R*x  R are the off diagonal gap elements of the jacobian.
        // we presume 1 thread. First nrn_thread[0].end equations are in node order.
        if (!nrnthread_v_transfer_) {
            return;
        }

        NrnThread* _nt = nrn_threads;

        // transfer gap node voltages to gap vpre
        for (size_t i = 0; i < visources_.size(); ++i) {
            Node* nd = visources_[i];
            if (real_imag == 0) {
                nd->v() = x[nd->v_node_index].real();
            } else {
                nd->v() = x[nd->v_node_index].imag();
            }
        }
        mpi_transfer();
        thread_transfer(_nt);

        // set gap node voltages to 0 so we can use nrn_cur to set rhs
        for (size_t i = 0; i < visources_.size(); ++i) {
            Node* nd = visources_[i];
            nd->v() = 0.0;
        }
        auto const sorted_token = nrn_ensure_model_data_are_sorted();
        auto* const vec_rhs = _nt->node_rhs_storage();
        // Initialize rhs to 0.
        for (int i = 0; i < _nt->end; ++i) {
            vec_rhs[i] = 0.0;
        }
        for (int k = 0; k < imped_current_type_count_; ++k) {
            int type = imped_current_type_[k];
            Memb_list* ml = imped_current_ml_[k];
            memb_func[type].current(sorted_token, _nt, ml, type);
        }

        // possibly many gap junctions in same node (and possible even different
        // types) but rhs is the accumulation of all those instances at each node
        // so ...  The only thing that can go wrong is if there are intances of
        // gap junctions that are not being used  (not in the target list).
        for (int i = 0; i < _nt->end; ++i) {
            if (real_imag == 0) {
                b[i] += vec_rhs[i];
            } else {
                b[i] += std::complex<double>(0, vec_rhs[i]);
            }
        }
    }
}

extern size_t nrnbbcore_gap_write(const char* path, int* group_ids);

/*
  file format for <path>/<group_id>_gap.dat
  All gap info for thread. Extracellular not allowed
  ntar  // number of targets in this thread (vpre)
  nsrc  // number of sources in this thread (v)

  Note: type, index is sufficient for CoreNEURON stdindex2ptr to determine
    double* in its NrnThread.data array.

  src_sid // nsrc of these
  src_type // nsrc mechanism type containing source variable, -1 is voltage.
  src_index // range variable index relative to beginning of first instance.

  tar_sid  // ntar of these
  tar_type // ntar mechanism type containing target variable.
  tar_index // range variable index relative to beginning of first instance.

  Assert no extracellular.

*/

/*
  The original file creation for each thread was accomplished by
  a serial function that:
  Verified the assertion constraints.
  Created an nthread array of BBCoreGapInfo.
  Wrote the <gid>_gap.dat files forall the threads.
  Cleaned up the BBCoreGapInfo (and gap_ml).

  So a simple factoring of the verify and create portions suffices
  for both files and direct memory transfer. Note that direct call
  returns pointer to SetupTransferInfo array.
  To cleanup, CoreNEURON should delete [] the return pointer.
*/

static SetupTransferInfo* nrncore_transfer_info(int);

SetupTransferInfo* nrn_get_partrans_setup_info(int ngroup, int cn_nthread, size_t cn_sidt_sz) {
    assert(cn_sidt_sz == sizeof(sgid_t));
    assert(ngroup == nrn_nthread);
    return nrncore_transfer_info(cn_nthread);
}

size_t nrnbbcore_gap_write(const char* path, int* group_ids) {
    auto gi = nrncore_transfer_info(nrn_nthread);  // gi stood for gapinfo
    if (gi == nullptr) {
        return 0;
    }

    // print the files
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        auto& g = gi[tid];

        if (g.src_sid.empty() && g.tar_sid.empty()) {  // no file
            continue;
        }

        char fname[1000];
        Sprintf(fname, "%s/%d_gap.dat", path, group_ids[tid]);
        FILE* f = fopen(fname, "wb");
        assert(f);
        fprintf(f, "%s\n", bbcore_write_version);
        fprintf(f, "%d sizeof_sid_t\n", int(sizeof(sgid_t)));

        int ntar = int(g.tar_sid.size());
        int nsrc = int(g.src_sid.size());
        fprintf(f, "%d ntar\n", ntar);
        fprintf(f, "%d nsrc\n", nsrc);

        int chkpnt = 0;
#define CHKPNT fprintf(f, "chkpnt %d\n", chkpnt++);

        if (!g.src_sid.empty()) {
            CHKPNT fwrite(g.src_sid.data(), nsrc, sizeof(sgid_t), f);
            CHKPNT fwrite(g.src_type.data(), nsrc, sizeof(int), f);
            CHKPNT fwrite(g.src_index.data(), nsrc, sizeof(int), f);
        }

        if (!g.tar_sid.empty()) {
            CHKPNT fwrite(g.tar_sid.data(), ntar, sizeof(sgid_t), f);
            CHKPNT fwrite(g.tar_type.data(), ntar, sizeof(int), f);
            CHKPNT fwrite(g.tar_index.data(), ntar, sizeof(int), f);
        }

        fclose(f);
    }

    // cleanup
    delete[] gi;
    return 0;
}

static SetupTransferInfo* nrncore_transfer_info(int cn_nthread) {
    assert(target_pntlist_.size() == targets_.size());

    // space for the info
    auto gi = new SetupTransferInfo[cn_nthread];

    // info for targets, segregate into threads
    if (targets_.size()) {
        for (size_t i = 0; i < targets_.size(); ++i) {
            sgid_t sid = sgid2targets_[i];
            Point_process* pp = target_pntlist_[i];
            NrnThread* nt = (NrnThread*) pp->_vnt;
            int tid = nt ? nt->id : 0;
            int type = pp->prop->_type;
            Memb_list& ml = *(nrn_threads[tid]._ml_list[type]);
            int ix = ml.legacy_index(targets_[i]);
            assert(ix >= 0);

            auto& g = gi[tid];
            g.tar_sid.push_back(sid);
            g.tar_type.push_back(type);
            g.tar_index.push_back(ix);
        }
    }

    // info for sources, segregate into threads.
    if (visources_.size()) {
        for (size_t i = 0; i < sgids_.size(); ++i) {
            sgid_t sid = sgids_[i];
            Node* nd = visources_[i];
            int tid = nd->_nt ? nd->_nt->id : 0;
            int type = -1;  // default voltage
            int ix = 0;     // fill below
            auto const it = non_vsrc_update_info_.find(sid);
            if (it != non_vsrc_update_info_.end()) {  // not a voltage source
                type = it->second.first;
                // this entire context needs to be reworked. If the source is a
                // point process, then if more than one in this nd, it is an error.
                auto d = non_vsrc_update(nd, type, it->second.second);
                NrnThread* nt = nd->_nt ? nd->_nt : nrn_threads;
                Memb_list& ml = *nt->_ml_list[type];
                ix = ml.legacy_index(d);
                assert(ix >= 0);
            } else {  // is a voltage source
                // Calculate the offset of the Node voltage in the section of
                // the underlying storage vector that is dedicated to NrnThread
                // number `tid`. Warning: this is only correct if no
                // modifications have been made to any Node since
                // reorder_secorder() was last called.
                auto const cache_token = nrn_ensure_model_data_are_sorted();
                ix = nd->_node_handle.current_row() -
                     cache_token.thread_cache(tid).node_data_offset;
                assert(nd->extnode == NULL);  // only if v
                assert(ix >= 0 && ix < nrn_threads[tid].end);
            }

            auto& g = gi[tid];
            g.src_sid.push_back(sid);
            g.src_type.push_back(type);
            g.src_index.push_back(ix);
        }
    }
    return gi;
}
