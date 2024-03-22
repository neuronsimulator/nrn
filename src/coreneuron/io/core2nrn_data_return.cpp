/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <sstream>
#include <mutex>

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/io/nrn2core_direct.h"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/io/core2nrn_data_return.hpp"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/permute/node_permute.h"
#include "coreneuron/utils/nrnoc_aux.hpp"
#include "coreneuron/utils/vrecitem.h"
#include "coreneuron/io/mem_layout_util.hpp"

/** @brief, Information from NEURON to help with copying data to NEURON.
 *  Info for copying voltage, i_membrane_, and mechanism data.
 *  See implementaton in
 *  nrn/src/nrniv/nrnbbcore_write.cpp:nrnthreads_type_return.
 *  Return is size of either the returned data pointer or the number
 *  of pointers in mdata. tid is the thread index.
 */
size_t (*nrn2core_type_return_)(int type, int tid, double*& data, std::vector<double*>& mdata);

/** @brief, Call NEURON mechanism bbcore_read.
 *  Inverse of bbcore_write for transfer from NEURON to CoreNEURON.
 *  Mostly for transferring back the nrnran123_State sequence so psolve can
 *  continue on NEURON side (or continue psolve on CoreNEURON).
 */
extern "C" {
int (*core2nrn_corepointer_mech_)(int tid,
                                  int type,
                                  int icnt,
                                  int dcnt,
                                  int* iArray,
                                  double* dArray);

int (*core2nrn_nmodlrandom_)(int tid,
                             int type,
                             const std::vector<int>& indices,
                             const std::vector<double>& nmodlrandom);
}

namespace coreneuron {

/** @brief permuted array copied to unpermuted array
 *  If permute is NULL then just a copy
 */
static void inverse_permute_copy(size_t n, double* permuted_src, double* dest, int* permute) {
    if (permute) {
        for (size_t i = 0; i < n; ++i) {
            dest[i] = permuted_src[permute[i]];
        }
    } else {
        std::copy(permuted_src, permuted_src + n, dest);
    }
}

/** @brief SoA permuted mechanism data copied to unpermuted AoS data.
 *  dest is an array of n pointers to the beginning of each sz length array.
 *  src is a contiguous array of sz segments of size stride. The stride
 *  may be slightly greater than n for purposes of alignment.
 *  Each of the sz segments of src are permuted.
 */
static void soa2aos_inverse_permute_copy(size_t n,
                                         int sz,
                                         int stride,
                                         double* src,
                                         std::vector<double*>& dest,
                                         int* permute) {
    // src is soa and permuted. dest is n pointers to sz doubles (aos).
    for (size_t instance = 0; instance < n; ++instance) {
        double* s = src + permute[instance];
        for (int i = 0; i < sz; ++i) {
            dest[i][instance] = s[i * stride];
        }
    }
}

/** @brief SoA unpermuted mechanism data copied to unpermuted AoS data.
 *  dest is an array of n pointers to the beginning of each sz length array.
 *  src is a contiguous array of sz segments of size stride. The stride
 *  may be slightly greater than n for purposes of alignment.
 *  Each of the sz segments of src have the same order as the n pointers
 *  of dest.
 */
static void soa2aos_unpermuted_copy(size_t n,
                                    int sz,
                                    int stride,
                                    double* src,
                                    std::vector<double*>& dest) {
    // src is soa and permuted. dest is n pointers to sz doubles (aos).
    for (size_t instance = 0; instance < n; ++instance) {
        double* s = src + instance;
        for (int i = 0; i < sz; ++i) {
            dest[i][instance] = s[i * stride];
        }
    }
}

/** @brief AoS mechanism data copied to AoS data.
 *  dest is an array of n pointers to the beginning of each sz length array.
 *  src is a contiguous array of n segments of size sz.
 */
static void aos2aos_copy(size_t n, int sz, double* src, std::vector<double*>& dest) {
    for (size_t instance = 0; instance < n; ++instance) {
        double* s = src + (instance * sz);
        for (auto i = 0; i < sz; ++i) {
            dest[i][instance] = s[i];
        }
    }
}

/** @brief Copy back COREPOINTER info to NEURON
 */
static void core2nrn_corepointer(int tid, NrnThreadMembList* tml) {
    // Based on get_bbcore_write fragment in nrn_checkpoint.cpp
    int type = tml->index;
    if (!corenrn.get_bbcore_write()[type]) {
        return;
    }
    NrnThread& nt = nrn_threads[tid];
    Memb_list* ml = tml->ml;
    double* d = nullptr;
    Datum* pd = nullptr;
    int layout = corenrn.get_mech_data_layout()[type];
    int dsz = corenrn.get_prop_param_size()[type];
    int pdsz = corenrn.get_prop_dparam_size()[type];
    int aln_cntml = nrn_soa_padded_size(ml->nodecount, layout);

    int icnt = 0;
    int dcnt = 0;
    // data size and allocate
    for (int j = 0; j < ml->nodecount; ++j) {
        int jp = j;
        if (ml->_permute) {
            jp = ml->_permute[j];
        }
        d = ml->data + nrn_i_layout(jp, ml->nodecount, 0, dsz, layout);
        pd = ml->pdata + nrn_i_layout(jp, ml->nodecount, 0, pdsz, layout);
        (*corenrn.get_bbcore_write()[type])(
            nullptr, nullptr, &dcnt, &icnt, 0, aln_cntml, d, pd, ml->_thread, &nt, ml, 0.0);
    }

    std::unique_ptr<int[]> iArray;
    std::unique_ptr<double[]> dArray;
    if (icnt) {
        iArray.reset(new int[icnt]);
    }
    if (dcnt) {
        dArray.reset(new double[dcnt]);
    }
    icnt = dcnt = 0;
    for (int j = 0; j < ml->nodecount; j++) {
        int jp = j;

        if (ml->_permute) {
            jp = ml->_permute[j];
        }

        d = ml->data + nrn_i_layout(jp, ml->nodecount, 0, dsz, layout);
        pd = ml->pdata + nrn_i_layout(jp, ml->nodecount, 0, pdsz, layout);

        (*corenrn.get_bbcore_write()[type])(dArray.get(),
                                            iArray.get(),
                                            &dcnt,
                                            &icnt,
                                            0,
                                            aln_cntml,
                                            d,
                                            pd,
                                            ml->_thread,
                                            &nt,
                                            ml,
                                            0.0);
    }

    (*core2nrn_corepointer_mech_)(tid, type, icnt, dcnt, iArray.get(), dArray.get());
}

// based on code from nrncore_callbacks.cpp
std::vector<int>& nrn_mech_random_indices(int type) {
    static std::unordered_map<int, std::vector<int>> mech_random_indices{};
    static std::mutex mx;
    std::unique_lock<std::mutex> lock(mx);
    if (mech_random_indices.count(type) == 0) {
        // if no element, create empty one and search dparam_semantics to fill
        auto& mri = mech_random_indices[type];
        int* semantics = corenrn.get_memb_func(type).dparam_semantics;
        int dparam_size = corenrn.get_prop_dparam_size()[type];
        for (int i = 0; i < dparam_size; ++i) {
            if (semantics[i] == -11) {
                mri.push_back(i);
            }
        }
    }
    lock.unlock();
    return mech_random_indices[type];
}

/** @brief Copy back NMODL RANDOM sequence to NEURON
 */
static void c2n_nmodlrandom(int tid, NrnThreadMembList* tml) {
    // Started out as a copy of corenrn_corepointer above.
    // overall algorithm for nmodlrandom is similar to nrnthread_dat2_mech.
    int type = tml->index;
    auto& indices = nrn_mech_random_indices(type);
    if (indices.size() == 0) {
        return;
    }
    NrnThread& nt = nrn_threads[tid];
    Memb_list* ml = tml->ml;
    int layout = corenrn.get_mech_data_layout()[type];
    int pdsz = corenrn.get_prop_dparam_size()[type];
    int aln_cntml = nrn_soa_padded_size(ml->nodecount, layout);
    int n = ml->nodecount;

    // will send back vector of 34 bit uints (aka double)
    std::vector<double> nmodlrandom{};
    nmodlrandom.reserve(n * indices.size());
    for (int ix: indices) {
        for (int j = 0; j < n; ++j) {
            int jp = j;
            if (ml->_permute) {
                jp = ml->_permute[j];
            }
            int pv = ml->pdata[nrn_i_layout(jp, n, ix, pdsz, layout)];
            nrnran123_State* state = (nrnran123_State*) nt._vdata[pv];
            uint32_t seq;
            char which;
            nrnran123_getseq(state, &seq, &which);
            nmodlrandom.push_back(double(seq) * 4 + which);
        }
    }

    (*core2nrn_nmodlrandom_)(tid, type, indices, nmodlrandom);
}

/** @brief Copy event queue and related state back to NEURON.
 */
static void core2nrn_tqueue(NrnThread&);

/** @brief Callback to clear NEURON thread queues.
    In particular need to initialize bin queues to the current time before
    transferring events.
 */
extern "C" {
void (*core2nrn_clear_queues_)(double t);
}

/** @brief All activated WATCH statements need activation on NEURON side.
 */
// vector in unpermuted Memb_list index order of vector of
// activated watch_index (the bool is whether it is above threshold).
using Core2NrnWatchInfoItem = std::vector<std::pair<int, bool>>;
using Core2NrnWatchInfo = std::vector<Core2NrnWatchInfoItem>;

extern "C" {
void (*core2nrn_watch_clear_)();
void (*core2nrn_watch_activate_)(int tid, int type, int watch_begin, Core2NrnWatchInfo&);
}

static void core2nrn_watch();

/** @brief VecPlay indices back to NEURON */
extern "C" {
void (*core2nrn_vecplay_)(int tid, int i_nrn, int last, int discon, int ubound);
void (*core2nrn_vecplay_events_)();
}

static void core2nrn_vecplay();

/** @brief copy data back to NEURON.
 *  Copies t, voltage, i_membrane_ if it used, and mechanism param data.
 *  Copies event queue and related state, e.g. WATCH, VecPlayContinuous.
 */
void core2nrn_data_return() {
    if (!nrn2core_type_return_) {
        return;
    }

    (*core2nrn_clear_queues_)(nrn_threads[0]._t);  // all threads at same time

    for (int tid = 0; tid < nrn_nthread; ++tid) {
        size_t n = 0;
        double* data = nullptr;
        NrnThread& nt = nrn_threads[tid];
        std::vector<double*> mdata{};
        n = (*nrn2core_type_return_)(0, tid, data, mdata);  // 0 means time
        if (n) {                                            // not the empty thread
            data[0] = nt._t;
        }

        if (nt.end) {  // transfer voltage and possibly i_membrane_
            n = (*nrn2core_type_return_)(voltage, tid, data, mdata);
            assert(n == size_t(nt.end) && data);
            inverse_permute_copy(n, nt._actual_v, data, nt._permute);

            if (nt.nrn_fast_imem) {
                n = (*nrn2core_type_return_)(i_membrane_, tid, data, mdata);
                assert(n == size_t(nt.end) && data);
                inverse_permute_copy(n, nt.nrn_fast_imem->nrn_sav_rhs, data, nt._permute);
            }
        }

        for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            int mtype = tml->index;
            Memb_list* ml = tml->ml;
            n = (*nrn2core_type_return_)(mtype, tid, data, mdata);
            assert(n == size_t(ml->nodecount) && !mdata.empty());
            if (n == 0) {
                continue;
            }
            // NEURON is AoS, CoreNEURON may be SoA and may be permuted.
            // On the NEURON side, the data is actually contiguous because of
            // cache_efficient, but that may not be the case for ARTIFICIAL_CELL.
            // For initial implementation simplicity, use the mdata info which gives
            // a double* for each param_size mech instance.
            int* permute = ml->_permute;
            double* cndat = ml->data;
            int layout = corenrn.get_mech_data_layout()[mtype];
            int sz = corenrn.get_prop_param_size()[mtype];
            if (layout == Layout::SoA) {
                int stride = ml->_nodecount_padded;
                if (permute) {
                    soa2aos_inverse_permute_copy(n, sz, stride, cndat, mdata, permute);
                } else {
                    soa2aos_unpermuted_copy(n, sz, stride, cndat, mdata);
                }
            } else { /* AoS */
                aos2aos_copy(n, sz, cndat, mdata);
            }

            core2nrn_corepointer(tid, tml);
            c2n_nmodlrandom(tid, tml);
        }

        // Copy the event queue and related state.
        core2nrn_tqueue(nt);
    }
    core2nrn_vecplay();
    core2nrn_watch();
}

/** @brief Callbacks into NEURON for WatchCondition.
 */
static void core2nrn_watch() {
    (*core2nrn_watch_clear_)();

    // much of the following nested iterations follows the
    // watch_activate_clear() function in sim/finitialize.cpp, though here
    // we iterate over nt._watch_types instead of nt.tml and then picking out
    // the WATCH relevant types with corenrn.get_watch_check().
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        NrnThread& nt = nrn_threads[tid];
        if (nt._watch_types) {
            for (int i = 0; nt._watch_types[i] != 0; ++i) {
                int type = nt._watch_types[i];
                Memb_list& ml = *(nt._ml_list[type]);
                int nodecount = ml.nodecount;
                Core2NrnWatchInfo watch_info(ml.nodecount);
                int* permute = ml._permute;
                int* pdata = (int*) ml.pdata;
                int dparam_size = corenrn.get_prop_dparam_size()[type];
                int layout = corenrn.get_mech_data_layout()[type];
                int first, last;
                watch_datum_indices(type, first, last);
                int watch_begin = first;
                for (int iml = 0; iml < nodecount; ++iml) {
                    int iml_permute = permute ? permute[iml] : iml;
                    Core2NrnWatchInfoItem& wiv = watch_info[iml];
                    for (int ix = first; ix <= last; ++ix) {
                        int datum =
                            pdata[nrn_i_layout(iml_permute, nodecount, ix, dparam_size, layout)];
                        if (datum & 2) {  // activated
                            bool above_thresh = bool(datum & 1);
                            wiv.push_back(std::pair<int, bool>(ix, above_thresh));
                        }
                    }
                }
                (*core2nrn_watch_activate_)(tid, type, watch_begin, watch_info);
            }
        }
    }
}

/** @brief Transfer VecPlay indices to NEURON.
 */
void core2nrn_vecplay() {
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        NrnThread& nt = nrn_threads[tid];
        std::vector<int> i_nrn;
        int ok = (*nrn2core_get_dat2_vecplay_)(tid, i_nrn);
        if (nt.n_vecplay) {
            assert(ok);
        }
        for (int i = 0; i < nt.n_vecplay; ++i) {
            VecPlayContinuous& vp = *((VecPlayContinuous*) nt._vecplay[i]);
            (*core2nrn_vecplay_)(tid,
                                 i_nrn[i],
                                 (int) vp.last_index_,
                                 (int) vp.discon_index_,
                                 (int) vp.ubound_index_);
        }
    }
    (*core2nrn_vecplay_events_)();
}

/** @brief Callbacks into NEURON for queue event types.
 */
extern "C" {
void (*core2nrn_NetCon_event_)(int tid, double td, size_t nc_index);

// must calculate netcon index from the weight index on this side
void (*core2nrn_SelfEvent_event_)(int tid,
                                  double td,
                                  int tar_type,
                                  int tar_index,
                                  double flag,
                                  size_t nc_index,
                                  int is_movable);
// the no weight case
void (*core2nrn_SelfEvent_event_noweight_)(int tid,
                                           double td,
                                           int tar_type,
                                           int tar_index,
                                           double flag,
                                           int is_movable);

// PreSyn.flag_ will be 1 if it has fired and the value it is watching
// is still greater than threshold. (Note, is 0 no matter what after
// finitialize so using a set to send back the flag explicitly for any
// that are 1. Although that is not really relevant in the core2nrn
// direction. To match up PreSyn on NEURON and CoreNEURON side, we use
// the (unpermuted) voltage index.
void (*core2nrn_PreSyn_flag_)(int tid, std::set<int> presyns_flag_true);
// Receive the PreSyn.flag_ == true voltage indices from the neuron side.
void (*nrn2core_transfer_PreSyn_flag_)(int tid, std::set<int>& presyns_flag_true);
}

static void core2nrn_PreSyn_flag(NrnThread& nt) {
    std::set<int> presyns_flag_true;
    std::unique_ptr<int[]> pinv_nt;
    if (nt._permute) {
        pinv_nt.reset(inverse_permute(nt._permute, nt.end));
    }
    for (int i = 0; i < nt.n_presyn; ++i) {
        PreSyn& ps = nt.presyns[i];
        PreSynHelper& psh = nt.presyns_helper[i];
        if (psh.flag_ && ps.thvar_index_ >= 0) {
            int index_v = pinv_nt ? pinv_nt[ps.thvar_index_] : ps.thvar_index_;
            presyns_flag_true.insert(index_v);
        }
    }
    // have to send even if empty so NEURON side can turn off all flag_
    (*core2nrn_PreSyn_flag_)(nt.id, presyns_flag_true);
}

void nrn2core_PreSyn_flag_receive(int tid) {
    NrnThread& nt = nrn_threads[tid];
    // turn off all the PreSyn.flag_ as they might have been turned off
    // on the NEURON side if NEURON integrated a bit.
    for (int i = 0; i < nt.n_presyn; ++i) {
        nt.presyns_helper[i].flag_ = 0;  // in case 1 from previous psolve
    }
    std::set<int> presyns_flag_true;
    (*nrn2core_transfer_PreSyn_flag_)(tid, presyns_flag_true);
    if (presyns_flag_true.empty()) {
        return;
    }
    std::unique_ptr<int[]> pinv_nt;
    if (nt._permute) {
        pinv_nt.reset(inverse_permute(nt._permute, nt.end));
    }
    for (int i = 0; i < nt.n_presyn; ++i) {
        PreSyn& ps = nt.presyns[i];
        PreSynHelper& psh = nt.presyns_helper[i];
        if (ps.thvar_index_ >= 0) {
            int index_v = pinv_nt ? pinv_nt[ps.thvar_index_] : ps.thvar_index_;
            if (presyns_flag_true.erase(index_v)) {
                psh.flag_ = 1;
                if (presyns_flag_true.empty()) {
                    break;
                }
            }
        }
    }
}

std::map<int, int*> type2invperm;

static void clear_inv_perm_for_selfevent_targets() {
    for (auto it: type2invperm) {
        delete[] it.second;
    }
    type2invperm.clear();
}


using SelfEventWeightMap = std::map<int, std::vector<TQItem*>>;

// return false unless q is pushed to sewm
static bool core2nrn_tqueue_item(TQItem* q, SelfEventWeightMap& sewm, NrnThread& nt) {
    DiscreteEvent* d = (DiscreteEvent*) q->data_;
    double td = q->t_;
    bool in_sewm = false;

    switch (d->type()) {
    case NetConType: {
        NetCon* nc = (NetCon*) d;
        assert(nc >= nt.netcons && (nc < (nt.netcons + nt.n_netcon)));
        size_t nc_index = nc - nt.netcons;
        (*core2nrn_NetCon_event_)(nt.id, td, nc_index);
        break;
    }
    case SelfEventType: {
        SelfEvent* se = (SelfEvent*) d;
        Point_process* pnt = se->target_;
        assert(pnt->_tid == nt.id);
        int tar_type = (int) pnt->_type;
        Memb_list* ml = nt._ml_list[tar_type];
        if (ml->_permute) {  // if permutation, then make inverse available
            // Doing this here because we don't know, in general, which
            // mechanisms use SelfEvent
            if (type2invperm.count(tar_type) == 0) {
                type2invperm[tar_type] = inverse_permute(ml->_permute, ml->nodecount);
            }
        }
        double flag = se->flag_;
        TQItem** movable = (TQItem**) (se->movable_);
        int is_movable = (movable && *movable == q) ? 1 : 0;
        int weight_index = se->weight_index_;
        // the weight_index is useless on the NEURON side so we need
        // to convert that to NetCon index  and let the NEURON side
        // figure out the weight_index. To figure out the netcon_index
        // construct a {weight_index : [TQItem]} here for any
        // weight_index >= 0, otherwise send it NEURON now.
        if (weight_index >= 0) {
            // Potentially several SelfEvent TQItem* associated with
            // same weight index. More importantly, collect them all
            // so that we only need to iterate over the nt.netcons once
            sewm[weight_index].push_back(q);
            in_sewm = true;

        } else {
            int tar_index = pnt->_i_instance;  // correct for no permutation
            if (ml->_permute) {
                tar_index = type2invperm[tar_type][tar_index];
            }
            (*core2nrn_SelfEvent_event_noweight_)(nt.id, td, tar_type, tar_index, flag, is_movable);
            delete se;
        }
        break;
    }
    case PreSynType: {
        // nothing to transfer
        // `d` can be cast to PreSyn*
        break;
    }
    case NetParEventType: {
        // nothing to transfer
        break;
    }
    case PlayRecordEventType: {
        // nothing to transfer
        break;
    }
    default: {
        // In particular, InputPreSyn does not appear in tqueue as it
        // immediately fans out to NetCon.
        std::stringstream qetype;
        qetype << d->type();
        hoc_execerror("core2nrn_tqueue_item -> unimplemented queue event type:",
                      qetype.str().c_str());
        break;
    }
    }
    return in_sewm;
}

void core2nrn_tqueue(NrnThread& nt) {
    // VecPlayContinuous

    // PatternStim

    // nrn_checkpoint.cpp has:
    // Avoid extra spikes due to some presyn voltages above threshold

    // PreSyn.flag_ that are on
    core2nrn_PreSyn_flag(nt);

    // The items on the queue
    NetCvodeThreadData& ntd = net_cvode_instance->p[nt.id];
    // make sure all buffered interthread events are on the queue
    ntd.enqueue(net_cvode_instance, &nt);

    TQueue<QTYPE>* tqe = ntd.tqe_;
    TQItem* q;
    SelfEventWeightMap sewm;
    // TQItems from atomic_dq
    while ((q = tqe->atomic_dq(1e20)) != nullptr) {
        if (core2nrn_tqueue_item(q, sewm, nt) == false) {
            delete q;
        }
    }
    // TQitems from binq_
    for (q = tqe->binq_->first(); q; q = tqe->binq_->next(q)) {
        bool const result = core2nrn_tqueue_item(q, sewm, nt);
        assert(result == false);
    }

    // For self events with weight, find the NetCon index and send that
    // to NEURON.
    // If the SelfEventWeightMap approach (and the corresponding pattern
    // on the nrn2core side in NEURON) ends up being too expensive in space
    // or time, it would be possible to modify SelfEvent to use the NetCon
    // index instead of the weight index, and then directly determine the
    // NetCon within the core2nrn_tqueue_item function above and call
    // (*core2nrn_SelfEvent_event_) from there.
    if (!sewm.empty()) {
        for (int nc_index = 0; nc_index < nt.n_netcon; ++nc_index) {
            NetCon& nc = nt.netcons[nc_index];
            int weight_index = nc.u.weight_index_;
            auto search = sewm.find(weight_index);
            if (search != sewm.end()) {
                const auto& tqitems = search->second;
                for (auto q: tqitems) {
                    DiscreteEvent* d = (DiscreteEvent*) (q->data_);
                    double td = q->t_;
                    assert(d->type() == SelfEventType);
                    SelfEvent* se = (SelfEvent*) d;
                    int tar_type = se->target_->_type;
                    // Note that instead of getting tar_index from the permuted
                    // pnt->_i_instance here and for the noweight case above
                    // which then needs the possibly large inverse permutation
                    // vectors, it would save some space to use the unpermuted
                    // nt.pntprocs array along with a much shorter vector
                    // of type offsets.
                    int tar_index = se->target_->_i_instance;
                    if (nt._ml_list[tar_type]->_permute) {
                        tar_index = type2invperm[tar_type][tar_index];
                    }
                    double flag = se->flag_;
                    TQItem** movable = (TQItem**) (se->movable_);
                    int is_movable = (movable && *movable == q) ? 1 : 0;
                    (*core2nrn_SelfEvent_event_)(
                        nt.id, td, tar_type, tar_index, flag, nc_index, is_movable);
                    delete q;
                    delete se;
                }
            }
        }
    }

    clear_inv_perm_for_selfevent_targets();
}

}  // namespace coreneuron
