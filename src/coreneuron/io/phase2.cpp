/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "coreneuron/io/phase2.hpp"
#include "coreneuron/coreneuron.hpp"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/io/nrn_checkpoint.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"
#include "coreneuron/permute/cellorder.hpp"
#include "coreneuron/permute/data_layout.hpp"
#include "coreneuron/permute/node_permute.h"
#include "coreneuron/utils/utils.hpp"
#include "coreneuron/utils/vrecitem.h"
#include "coreneuron/io/mem_layout_util.hpp"
#include "coreneuron/io/setup_fornetcon.hpp"

#if defined(_OPENMP)
#include <omp.h>
#endif

int (*nrn2core_get_dat2_1_)(int tid,
                            int& n_real_cell,
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
                               int*& pdata,
                               std::vector<uint32_t>& nmodlrandom,
                               std::vector<int>& pointer2type);

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

int (*nrn2core_get_dat2_vecplay_)(int tid, std::vector<int>& indices);

int (*nrn2core_get_dat2_vecplay_inst_)(int tid,
                                       int i,
                                       int& vptype,
                                       int& mtype,
                                       int& ix,
                                       int& sz,
                                       double*& yvec,
                                       double*& tvec,
                                       int& last_index,
                                       int& discon_index,
                                       int& ubound_index);

namespace coreneuron {
template <typename T>
void mech_data_layout_transform(T* data, int cnt, const std::vector<int>& array_dims, int layout) {
    if (layout == Layout::AoS) {
        throw std::runtime_error("AoS memory layout not implemented.");
    }

    int n_vars = array_dims.size();
    int row_width = std::accumulate(array_dims.begin(), array_dims.end(), 0);
    int padded_cnt = nrn_soa_padded_size(cnt, layout);

    std::vector<T> tmp(padded_cnt * row_width);
    std::copy(data, data + cnt * row_width, tmp.begin());

    size_t offset_var = 0;
    for (size_t i_var = 0; i_var < n_vars; ++i_var) {
        size_t K = array_dims[i_var];

        for (size_t i = 0; i < cnt; ++i) {
            for (size_t k = 0; k < K; ++k) {
                size_t i_dst = padded_cnt * offset_var + i * K + k;
                size_t i_src = i * row_width + offset_var + k;
                data[i_dst] = tmp[i_src];
            }
        }

        offset_var += K;
    }
}

template <typename T>
inline void mech_data_layout_transform(T* data, int cnt, int sz, int layout) {
    std::vector<int> array_dims(sz, 1);
    mech_data_layout_transform(data, cnt, array_dims, layout);
}


void Phase2::read_file(FileHandler& F, const NrnThread& nt) {
    n_real_cell = F.read_int();
    n_output = F.read_int();
    n_real_output = F.read_int();
    n_node = F.read_int();
    n_diam = F.read_int();
    n_mech = F.read_int();
    mech_types = std::vector<int>(n_mech, 0);
    nodecounts = std::vector<int>(n_mech, 0);
    for (int i = 0; i < n_mech; ++i) {
        mech_types[i] = F.read_int();
        nodecounts[i] = F.read_int();
    }

    // check mechanism compatibility before reading data
    check_mechanism();

    n_idata = F.read_int();
    n_vdata = F.read_int();
    int n_weight = F.read_int();
    v_parent_index = (int*) ecalloc_align(n_node, sizeof(int));
    F.read_array<int>(v_parent_index, n_node);

    int n_data_padded = nrn_soa_padded_size(n_node, SOA_LAYOUT);
    {
        {  // Compute size of _data and allocate
            int n_data = 6 * n_data_padded;
            if (n_diam > 0) {
                n_data += n_data_padded;
            }
            for (int i = 0; i < n_mech; ++i) {
                int layout = corenrn.get_mech_data_layout()[mech_types[i]];
                int n = nodecounts[i];
                int sz = corenrn.get_prop_param_size()[mech_types[i]];
                n_data = nrn_soa_byte_align(n_data);
                n_data += nrn_soa_padded_size(n, layout) * sz;
            }
            _data = (double*) ecalloc_align(n_data, sizeof(double));
        }
        F.read_array<double>(_data + 2 * n_data_padded, n_node);
        F.read_array<double>(_data + 3 * n_data_padded, n_node);
        F.read_array<double>(_data + 5 * n_data_padded, n_node);
        F.read_array<double>(_data + 4 * n_data_padded, n_node);
        if (n_diam > 0) {
            F.read_array<double>(_data + 6 * n_data_padded, n_node);
        }
    }

    size_t offset = 6 * n_data_padded;
    if (n_diam > 0) {
        offset += n_data_padded;
    }
    for (int i = 0; i < n_mech; ++i) {
        int layout = corenrn.get_mech_data_layout()[mech_types[i]];
        int n = nodecounts[i];
        int sz = corenrn.get_prop_param_size()[mech_types[i]];
        int dsz = corenrn.get_prop_dparam_size()[mech_types[i]];
        offset = nrn_soa_byte_align(offset);
        std::vector<int> nodeindices;
        if (!corenrn.get_is_artificial()[mech_types[i]]) {
            nodeindices = F.read_vector<int>(n);
        }
        F.read_array<double>(_data + offset, sz * n);
        offset += nrn_soa_padded_size(n, layout) * sz;
        std::vector<int> pdata;
        if (dsz > 0) {
            pdata = F.read_vector<int>(dsz * n);
        }
        tmls.emplace_back(TML{nodeindices, pdata, mech_types[i], {}, {}});
        if (dsz > 0) {
            int sz = F.read_int();
            if (sz) {
                auto& p2t = tmls.back().pointer2type;
                p2t = F.read_vector<int>(sz);
            }

            // nmodlrandom
            sz = F.read_int();
            if (sz) {
                auto& nmodlrandom = tmls.back().nmodlrandom;
                nmodlrandom = F.read_vector<uint32_t>(sz);
            }
        }
    }
    output_vindex = F.read_vector<int>(nt.n_presyn);
    output_threshold = F.read_vector<double>(n_real_output);
    pnttype = F.read_vector<int>(nt.n_netcon);
    pntindex = F.read_vector<int>(nt.n_netcon);
    weights = F.read_vector<double>(n_weight);
    delay = F.read_vector<double>(nt.n_netcon);
    num_point_process = F.read_int();

    for (int i = 0; i < n_mech; ++i) {
        if (!corenrn.get_bbcore_read()[mech_types[i]]) {
            continue;
        }
        tmls[i].type = F.read_int();
        int icnt = F.read_int();
        int dcnt = F.read_int();
        if (icnt > 0) {
            tmls[i].iArray = F.read_vector<int>(icnt);
        }
        if (dcnt > 0) {
            tmls[i].dArray = F.read_vector<double>(dcnt);
        }
    }

    int n_vec_play_continuous = F.read_int();
    vec_play_continuous.reserve(n_vec_play_continuous);
    for (int i = 0; i < n_vec_play_continuous; ++i) {
        VecPlayContinuous_ item;
        item.vtype = F.read_int();
        item.mtype = F.read_int();
        item.ix = F.read_int();
        int sz = F.read_int();
        item.yvec = IvocVect(sz);
        item.tvec = IvocVect(sz);
        F.read_array<double>(item.yvec.data(), sz);
        F.read_array<double>(item.tvec.data(), sz);
        vec_play_continuous.push_back(std::move(item));
    }

    // store current checkpoint state to continue reading mapping
    // The checkpoint numbering in phase 3 is a continuing of phase 2, and so will be restored
    F.record_checkpoint();

    if (F.eof())
        return;

    nrn_assert(F.read_int() == n_vec_play_continuous);

    for (int i = 0; i < n_vec_play_continuous; ++i) {
        auto& vecPlay = vec_play_continuous[i];
        vecPlay.last_index = F.read_int();
        vecPlay.discon_index = F.read_int();
        vecPlay.ubound_index = F.read_int();
    }

    patstim_index = F.read_int();

    nrn_assert(F.read_int() == -1);

    for (int i = 0; i < nt.n_presyn; ++i) {
        preSynConditionEventFlags.push_back(F.read_int());
    }

    nrn_assert(F.read_int() == -1);
    restore_events(F);

    nrn_assert(F.read_int() == -1);
    restore_events(F);
}

void Phase2::read_direct(int thread_id, const NrnThread& nt) {
    int* types_ = nullptr;
    int* nodecounts_ = nullptr;
    int n_weight;
    (*nrn2core_get_dat2_1_)(thread_id,
                            n_real_cell,
                            n_output,
                            n_real_output,
                            n_node,
                            n_diam,
                            n_mech,
                            types_,
                            nodecounts_,
                            n_idata,
                            n_vdata,
                            n_weight);
    mech_types = std::vector<int>(types_, types_ + n_mech);
    delete[] types_;

    nodecounts = std::vector<int>(nodecounts_, nodecounts_ + n_mech);
    delete[] nodecounts_;

    check_mechanism();

    // TODO: fix it in the future
    int n_data_padded = nrn_soa_padded_size(n_node, SOA_LAYOUT);
    int n_data = 6 * n_data_padded;
    if (n_diam > 0) {
        n_data += n_data_padded;
    }
    for (int i = 0; i < n_mech; ++i) {
        int layout = corenrn.get_mech_data_layout()[mech_types[i]];
        int n = nodecounts[i];
        int sz = corenrn.get_prop_param_size()[mech_types[i]];
        n_data = nrn_soa_byte_align(n_data);
        n_data += nrn_soa_padded_size(n, layout) * sz;
    }
    _data = (double*) ecalloc_align(n_data, sizeof(double));

    v_parent_index = (int*) ecalloc_align(n_node, sizeof(int));
    double* actual_a = _data + 2 * n_data_padded;
    double* actual_b = _data + 3 * n_data_padded;
    double* actual_v = _data + 4 * n_data_padded;
    double* actual_area = _data + 5 * n_data_padded;
    double* actual_diam = n_diam > 0 ? _data + 6 * n_data_padded : nullptr;
    (*nrn2core_get_dat2_2_)(
        thread_id, v_parent_index, actual_a, actual_b, actual_area, actual_v, actual_diam);

    tmls.resize(n_mech);

    auto& param_sizes = corenrn.get_prop_param_size();
    auto& dparam_sizes = corenrn.get_prop_dparam_size();
    int dsz_inst = 0;
    size_t offset = 6 * n_data_padded;
    if (n_diam > 0) {
        offset += n_data_padded;
    }
    for (int i = 0; i < n_mech; ++i) {
        auto& tml = tmls[i];
        int type = mech_types[i];
        int layout = corenrn.get_mech_data_layout()[type];
        offset = nrn_soa_byte_align(offset);

        tml.type = type;
        // artificial cell don't use nodeindices
        if (!corenrn.get_is_artificial()[type]) {
            tml.nodeindices.resize(nodecounts[i]);
        }
        tml.pdata.resize(nodecounts[i] * dparam_sizes[type]);

        int* nodeindices_ = nullptr;
        double* data_ = _data + offset;
        int* pdata_ = const_cast<int*>(tml.pdata.data());

        // nmodlrandom, receives:
        // number of random variables
        // dparam index (neuron side) of each random variable
        // 5 uint32 for each var of each instance
        // id1, id2, id3, seq, uint32_t(which)
        // all instances of ranvar1 first, then all instances of ranvar2, etc.
        auto& nmodlrandom = tml.nmodlrandom;

        (*nrn2core_get_dat2_mech_)(thread_id,
                                   i,
                                   dparam_sizes[type] > 0 ? dsz_inst : 0,
                                   nodeindices_,
                                   data_,
                                   pdata_,
                                   nmodlrandom,
                                   tml.pointer2type);

        if (dparam_sizes[type] > 0) {
            dsz_inst++;
        }
        offset += nrn_soa_padded_size(nodecounts[i], layout) * param_sizes[type];
        if (nodeindices_) {
            std::copy(nodeindices_, nodeindices_ + nodecounts[i], tml.nodeindices.data());
            free(nodeindices_);  // not free_memory because this is allocated by NEURON?
        }
        if (corenrn.get_is_artificial()[type]) {
            assert(nodeindices_ == nullptr);
        }
    }

    int* output_vindex_ = nullptr;
    double* output_threshold_ = nullptr;
    int* pnttype_ = nullptr;
    int* pntindex_ = nullptr;
    double* weight_ = nullptr;
    double* delay_ = nullptr;
    (*nrn2core_get_dat2_3_)(thread_id,
                            n_weight,
                            output_vindex_,
                            output_threshold_,
                            pnttype_,
                            pntindex_,
                            weight_,
                            delay_);

    output_vindex = std::vector<int>(output_vindex_, output_vindex_ + nt.n_presyn);
    delete[] output_vindex_;

    output_threshold = std::vector<double>(output_threshold_, output_threshold_ + n_real_output);
    delete[] output_threshold_;

    int n_netcon = nt.n_netcon;
    pnttype = std::vector<int>(pnttype_, pnttype_ + n_netcon);
    delete[] pnttype_;

    pntindex = std::vector<int>(pntindex_, pntindex_ + n_netcon);
    delete[] pntindex_;

    weights = std::vector<double>(weight_, weight_ + n_weight);
    delete[] weight_;

    delay = std::vector<double>(delay_, delay_ + n_netcon);
    delete[] delay_;

    (*nrn2core_get_dat2_corepointer_)(nt.id, num_point_process);

    for (int i = 0; i < n_mech; ++i) {
        // not all mod files have BBCOREPOINTER data to read
        if (!corenrn.get_bbcore_read()[mech_types[i]]) {
            continue;
        }
        int icnt;
        int* iArray_ = nullptr;
        int dcnt;
        double* dArray_ = nullptr;
        (*nrn2core_get_dat2_corepointer_mech_)(nt.id, tmls[i].type, icnt, dcnt, iArray_, dArray_);
        tmls[i].iArray.resize(icnt);
        std::copy(iArray_, iArray_ + icnt, tmls[i].iArray.begin());
        delete[] iArray_;

        tmls[i].dArray.resize(dcnt);
        std::copy(dArray_, dArray_ + dcnt, tmls[i].dArray.begin());
        delete[] dArray_;
    }

    // Get from NEURON, the VecPlayContinuous indices in
    // NetCvode::fixed_play_ for this thread.
    std::vector<int> indices_vec_play_continuous;
    (*nrn2core_get_dat2_vecplay_)(thread_id, indices_vec_play_continuous);

    // i is an index into NEURON's NetCvode::fixed_play_ for this thread.
    for (auto i: indices_vec_play_continuous) {
        VecPlayContinuous_ item;
        // yvec_ and tvec_ are not deleted as that space is within
        // NEURON Vector
        double *yvec_, *tvec_;
        int sz;
        (*nrn2core_get_dat2_vecplay_inst_)(thread_id,
                                           i,
                                           item.vtype,
                                           item.mtype,
                                           item.ix,
                                           sz,
                                           yvec_,
                                           tvec_,
                                           item.last_index,
                                           item.discon_index,
                                           item.ubound_index);
        item.yvec = IvocVect(sz);
        item.tvec = IvocVect(sz);
        std::copy(yvec_, yvec_ + sz, item.yvec.data());
        std::copy(tvec_, tvec_ + sz, item.tvec.data());
        vec_play_continuous.push_back(std::move(item));
    }
}

/// Check if MOD file used between NEURON and CoreNEURON is same
void Phase2::check_mechanism() {
    int diff_mech_count = 0;
    for (int i = 0; i < n_mech; ++i) {
        if (std::any_of(corenrn.get_different_mechanism_type().begin(),
                        corenrn.get_different_mechanism_type().end(),
                        [&](int e) { return e == mech_types[i]; })) {
            if (nrnmpi_myid == 0) {
                printf("Error: %s is a different MOD file than used by NEURON!\n",
                       nrn_get_mechname(mech_types[i]));
            }
            diff_mech_count++;
        }
    }

    if (diff_mech_count > 0) {
        if (nrnmpi_myid == 0) {
            printf(
                "Error : NEURON and CoreNEURON must use same mod files for compatibility, %d "
                "different mod file(s) found. Re-compile special and special-core!\n",
                diff_mech_count);
            nrn_abort(1);
        }
    }
}

/// Perform in memory transformation between AoS<>SoA for integer data
void Phase2::transform_int_data(int elem0,
                                int nodecount,
                                int* pdata,
                                int i,
                                int dparam_size,
                                int layout,
                                int n_node_) {
    for (int iml = 0; iml < nodecount; ++iml) {
        int* pd = pdata + nrn_i_layout(iml, nodecount, i, dparam_size, layout);
        int ix = *pd;  // relative to beginning of _actual_*
        nrn_assert((ix >= 0) && (ix < n_node_));
        *pd = elem0 + ix;  // relative to nt._data
    }
}

void Phase2::set_net_send_buffer(Memb_list** ml_list, const std::vector<int>& pnt_offset) {
    // NetReceiveBuffering
    for (auto& net_buf_receive: corenrn.get_net_buf_receive()) {
        int type = net_buf_receive.second;
        // Does this thread have this type.
        Memb_list* ml = ml_list[type];
        if (ml) {  // needs a NetReceiveBuffer
            NetReceiveBuffer_t* nrb =
                (NetReceiveBuffer_t*) ecalloc_align(1, sizeof(NetReceiveBuffer_t));
            assert(!ml->_net_receive_buffer);
            ml->_net_receive_buffer = nrb;
            nrb->_pnt_offset = pnt_offset[type];

            // begin with a size equal to the number of instances, or at least 8
            nrb->_size = std::max(8, ml->nodecount);
            nrb->_pnt_index = (int*) ecalloc_align(nrb->_size, sizeof(int));
            nrb->_displ = (int*) ecalloc_align(nrb->_size + 1, sizeof(int));
            nrb->_nrb_index = (int*) ecalloc_align(nrb->_size, sizeof(int));
            nrb->_weight_index = (int*) ecalloc_align(nrb->_size, sizeof(int));
            nrb->_nrb_t = (double*) ecalloc_align(nrb->_size, sizeof(double));
            nrb->_nrb_flag = (double*) ecalloc_align(nrb->_size, sizeof(double));
        }
    }

    // NetSendBuffering
    for (int type: corenrn.get_net_buf_send_type()) {
        // Does this thread have this type.
        Memb_list* ml = ml_list[type];
        if (ml) {  // needs a NetSendBuffer
            assert(!ml->_net_send_buffer);
            // begin with a size equal to twice number of instances
            NetSendBuffer_t* nsb = new NetSendBuffer_t(ml->nodecount * 2);
            ml->_net_send_buffer = nsb;
        }
    }
}

void Phase2::restore_events(FileHandler& F) {
    int type;
    while ((type = F.read_int()) != 0) {
        double time;
        F.read_array(&time, 1);
        switch (type) {
        case NetConType: {
            auto event = std::make_shared<NetConType_>();
            event->time = time;
            event->netcon_index = F.read_int();
            events.emplace_back(type, event);
            break;
        }
        case SelfEventType: {
            auto event = std::make_shared<SelfEventType_>();
            event->time = time;
            event->target_type = F.read_int();
            event->point_proc_instance = F.read_int();
            event->target_instance = F.read_int();
            F.read_array(&event->flag, 1);
            event->movable = F.read_int();
            event->weight_index = F.read_int();
            events.emplace_back(type, event);
            break;
        }
        case PreSynType: {
            auto event = std::make_shared<PreSynType_>();
            event->time = time;
            event->presyn_index = F.read_int();
            events.emplace_back(type, event);
            break;
        }
        case NetParEventType: {
            auto event = std::make_shared<NetParEvent_>();
            event->time = time;
            events.emplace_back(type, event);
            break;
        }
        case PlayRecordEventType: {
            auto event = std::make_shared<PlayRecordEventType_>();
            event->time = time;
            event->play_record_type = F.read_int();
            if (event->play_record_type == VecPlayContinuousType) {
                event->vecplay_index = F.read_int();
                events.emplace_back(type, event);
            } else {
                nrn_assert(0);
            }
            break;
        }
        default: {
            nrn_assert(0);
            break;
        }
        }
    }
}

void Phase2::fill_before_after_lists(NrnThread& nt, const std::vector<Memb_func>& memb_func) {
    /// Fill the BA lists
    std::vector<BAMech*> before_after_map(memb_func.size());
    for (int i = 0; i < BEFORE_AFTER_SIZE; ++i) {
        for (size_t ii = 0; ii < memb_func.size(); ++ii) {
            before_after_map[ii] = nullptr;
        }
        // Save first before-after block only. In case of multiple before-after blocks with the
        // same mech type, we will get subsequent ones using linked list below.
        for (auto bam = corenrn.get_bamech()[i]; bam; bam = bam->next) {
            if (!before_after_map[bam->type]) {
                before_after_map[bam->type] = bam;
            }
        }
        // necessary to keep in order wrt multiple BAMech with same mech type
        NrnThreadBAList** ptbl = nt.tbl + i;
        for (auto tml = nt.tml; tml; tml = tml->next) {
            if (before_after_map[tml->index]) {
                int mtype = tml->index;
                for (auto bam = before_after_map[mtype]; bam && bam->type == mtype;
                     bam = bam->next) {
                    auto tbl = (NrnThreadBAList*) emalloc(sizeof(NrnThreadBAList));
                    *ptbl = tbl;
                    tbl->next = nullptr;
                    tbl->bam = bam;
                    tbl->ml = tml->ml;
                    ptbl = &(tbl->next);
                }
            }
        }
    }
}

void Phase2::pdata_relocation(const NrnThread& nt, const std::vector<Memb_func>& memb_func) {
    // Some pdata may index into data which has been reordered from AoS to
    // SoA. The four possibilities are if semantics is -1 (area), -5 (pointer),
    // -9 (diam), // or 0-999 (ion variables).
    // Note that pdata has a layout and the // type block in nt.data into which
    // it indexes, has a layout.

    // For faster search of tmls[i].type == type, use a map.
    // (perhaps would be better to replace tmls so that we can use tmls[type].
    std::map<int, size_t> type2itml;
    for (size_t i = 0; i < tmls.size(); ++i) {
        if (tmls[i].pointer2type.size()) {
            type2itml[tmls[i].type] = i;
        }
    }

    for (auto tml = nt.tml; tml; tml = tml->next) {
        int type = tml->index;
        int layout = corenrn.get_mech_data_layout()[type];
        int* pdata = tml->ml->pdata;
        int cnt = tml->ml->nodecount;
        int szdp = corenrn.get_prop_dparam_size()[type];
        int* semantics = memb_func[type].dparam_semantics;

        // compute only for ARTIFICIAL_CELL (has useful area pointer with semantics=-1)
        if (!corenrn.get_is_artificial()[type]) {
            if (szdp) {
                if (!semantics)
                    continue;  // temporary for HDFReport, Binreport which will be skipped in
                // bbcore_write of HBPNeuron
                nrn_assert(semantics);
            }

            for (int i = 0; i < szdp; ++i) {
                int s = semantics[i];
                switch (s) {
                case -1:  // area
                    transform_int_data(
                        nt._actual_area - nt._data, cnt, pdata, i, szdp, layout, nt.end);
                    break;
                case -9:  // diam
                    transform_int_data(
                        nt._actual_diam - nt._data, cnt, pdata, i, szdp, layout, nt.end);
                    break;
                case -5:  // pointer assumes a pointer to membrane voltage
                    // or mechanism data in this thread. The value of the
                    // pointer on the NEURON side was analyzed by
                    // nrn_dblpntr2nrncore which returned the
                    // mechanism index and type. At this moment the index
                    // is in pdata and the type is in tmls[type].pointer2type.
                    // However the latter order is according to the nested
                    // iteration for nodecount { for szdp {}}
                    // Also the nodecount POINTER instances of mechanism
                    // might possibly point to differnt range variables.
                    // Therefore it is not possible to use transform_int_data
                    // and the transform must be done one at a time.
                    // So we do nothing here and separately iterate
                    // after this loop instead of the former voltage only
                    /**
                    transform_int_data(
                        nt._actual_v - nt._data, cnt, pdata, i, szdp, layout, nt.end);
                     **/
                    break;
                default:
                    if (s >= 0 && s < 1000) {  // ion
                        int etype = s;
                        /* if ion is SoA, must recalculate pdata values */
                        /* if ion is AoS, have to deal with offset */
                        Memb_list* eml = nt._ml_list[etype];
                        int edata0 = eml->data - nt._data;
                        int ecnt = eml->nodecount;
                        int ecnt_padded = nrn_soa_padded_size(ecnt, Layout::SoA);
                        int esz = corenrn.get_prop_param_size()[etype];
                        const std::vector<int>& array_dims = corenrn.get_array_dims()[etype];
                        for (int iml = 0; iml < cnt; ++iml) {
                            int* pd = pdata + nrn_i_layout(iml, cnt, i, szdp, layout);
                            int legacy_index = *pd;  // relative to the ion data
                            nrn_assert((legacy_index >= 0) && (legacy_index < ecnt * esz));

                            auto soaos_index = legacy2soaos_index(legacy_index, array_dims);
                            *pd = edata0 +
                                  soaos2cnrn_index(soaos_index, array_dims, ecnt_padded, nullptr);
                        }
                    }
                }
            }
            // Handle case -5 POINTER transformation (see comment above)
            auto search = type2itml.find(type);
            if (search != type2itml.end()) {
                auto& ptypes = tmls[type2itml[type]].pointer2type;
                assert(ptypes.size());
                size_t iptype = 0;
                for (int iml = 0; iml < cnt; ++iml) {
                    for (int i = 0; i < szdp; ++i) {
                        if (semantics[i] == -5) {  // POINTER
                            int* pd = pdata + nrn_i_layout(iml, cnt, i, szdp, layout);
                            int ix = *pd;  // relative to elem0
                            int ptype = ptypes[iptype++];
                            if (ptype == voltage) {
                                nrn_assert((ix >= 0) && (ix < nt.end));
                                int elem0 = nt._actual_v - nt._data;
                                *pd = elem0 + ix;
                            } else {
                                Memb_list* pml = nt._ml_list[ptype];
                                int pcnt = pml->nodecount;
                                int pcnt_padded = nrn_soa_padded_size(pcnt, Layout::SoA);
                                int psz = corenrn.get_prop_param_size()[ptype];
                                const std::vector<int>& array_dims =
                                    corenrn.get_array_dims()[ptype];
                                nrn_assert((ix >= 0) && (ix < pcnt * psz));

                                int elem0 = pml->data - nt._data;
                                auto soaos_index = legacy2soaos_index(ix, array_dims);
                                *pd =
                                    elem0 +
                                    soaos2cnrn_index(soaos_index, array_dims, pcnt_padded, nullptr);
                            }
                        }
                    }
                }
                ptypes.clear();
            }
        }
    }
}

void Phase2::set_dependencies(const NrnThread& nt, const std::vector<Memb_func>& memb_func) {
    /* here we setup the mechanism dependencies. if there is a mechanism dependency
     * then we allocate an array for tml->dependencies otherwise set it to nullptr.
     * In order to find out the "real" dependencies i.e. dependent mechanism
     * exist at the same compartment, we compare the nodeindices of mechanisms
     * returned by nrn_mech_depend.
     */

    /* temporary array for dependencies */
    int* mech_deps = (int*) ecalloc(memb_func.size(), sizeof(int));

    for (auto tml = nt.tml; tml; tml = tml->next) {
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
                    std::set_intersection(nodeindices,
                                          nodeindices + ml->nodecount,
                                          dnodeindices,
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
                tml->dependencies = (int*) ecalloc(actual_mech_deps.size(), sizeof(int));
                std::copy(actual_mech_deps.begin(), actual_mech_deps.end(), tml->dependencies);
            }
        }
    }

    /* free temp dependency array */
    free(mech_deps);
}

void Phase2::handle_weights(NrnThread& nt, int n_netcon, NrnThreadChkpnt& ntc) {
    nt.n_weight = weights.size();
    // weights in netcons order in groups defined by Point_process target type.
    nt.weights = (double*) ecalloc_align(nt.n_weight, sizeof(double));
    std::copy(weights.begin(), weights.end(), nt.weights);

    int iw = 0;
    for (int i = 0; i < n_netcon; ++i) {
        NetCon& nc = nt.netcons[i];
        nc.u.weight_index_ = iw;
        if (pnttype[i] != 0) {
            iw += corenrn.get_pnt_receive_size()[pnttype[i]];
        } else {
            iw += 1;
        }
    }
    assert(iw == nt.n_weight);

    // Nontrivial if FOR_NETCON in use by some mechanisms
    setup_fornetcon_info(nt);


#if CHKPNTDEBUG
    ntc.delay = new double[n_netcon];
    memcpy(ntc.delay, delay.data(), n_netcon * sizeof(double));
#endif
    for (int i = 0; i < n_netcon; ++i) {
        NetCon& nc = nt.netcons[i];
        nc.delay_ = delay[i];
    }
}

void Phase2::get_info_from_bbcore(NrnThread& nt,
                                  const std::vector<Memb_func>& memb_func,
                                  NrnThreadChkpnt& ntc) {
    // BBCOREPOINTER information
#if CHKPNTDEBUG
    ntc.nbcp = num_point_process;
    ntc.bcpicnt = new int[n_mech];
    ntc.bcpdcnt = new int[n_mech];
    ntc.bcptype = new int[n_mech];
    size_t point_proc_id = 0;
#endif
    for (int i = 0; i < n_mech; ++i) {
        int type = mech_types[i];
        if (!corenrn.get_bbcore_read()[type]) {
            continue;
        }
        type = tmls[i].type;  // This is not an error, but it has to be fixed I think
#if CHKPNTDEBUG
        ntc.bcptype[point_proc_id] = type;
        ntc.bcpicnt[point_proc_id] = tmls[i].iArray.size();
        ntc.bcpdcnt[point_proc_id] = tmls[i].dArray.size();
        point_proc_id++;
#endif
        int ik = 0;
        int dk = 0;
        Memb_list* ml = nt._ml_list[type];
        int dsz = corenrn.get_prop_param_size()[type];
        int pdsz = corenrn.get_prop_dparam_size()[type];
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
            (*corenrn.get_bbcore_read()[type])(tmls[i].dArray.data(),
                                               tmls[i].iArray.data(),
                                               &dk,
                                               &ik,
                                               0,
                                               aln_cntml,
                                               d,
                                               pd,
                                               ml->_thread,
                                               &nt,
                                               ml,
                                               0.0);
        }
        assert(dk == static_cast<int>(tmls[i].dArray.size()));
        assert(ik == static_cast<int>(tmls[i].iArray.size()));
    }
}

void Phase2::set_vec_play(NrnThread& nt, NrnThreadChkpnt& ntc) {
    // VecPlayContinuous instances
    // No attempt at memory efficiency
    nt.n_vecplay = vec_play_continuous.size();
    if (nt.n_vecplay) {
        nt._vecplay = new void*[nt.n_vecplay];
    } else {
        nt._vecplay = nullptr;
    }
#if CHKPNTDEBUG
    ntc.vecplay_ix = new int[nt.n_vecplay];
    ntc.vtype = new int[nt.n_vecplay];
    ntc.mtype = new int[nt.n_vecplay];
#endif
    for (int i = 0; i < nt.n_vecplay; ++i) {
        auto& vecPlay = vec_play_continuous[i];
        nrn_assert(vecPlay.vtype == VecPlayContinuousType);
#if CHKPNTDEBUG
        ntc.vtype[i] = vecPlay.vtype;
        ntc.mtype[i] = vecPlay.mtype;
        ntc.vecplay_ix[i] = vecPlay.ix;
#endif
        Memb_list* ml = nt._ml_list[vecPlay.mtype];

        const std::vector<int>& array_dims = corenrn.get_array_dims()[vecPlay.mtype];

        auto padded_nodecount = nrn_soa_padded_size(ml->nodecount, Layout::SoA);
        auto soaos_index = legacy2soaos_index(vecPlay.ix, array_dims);
        vecPlay.ix = soaos2cnrn_index(soaos_index, array_dims, padded_nodecount, ml->_permute);

        nt._vecplay[i] = new VecPlayContinuous(ml->data + vecPlay.ix,
                                               std::move(vecPlay.yvec),
                                               std::move(vecPlay.tvec),
                                               nullptr,
                                               nt.id);
    }
}

void Phase2::populate(NrnThread& nt, const UserParams& userParams) {
    NrnThreadChkpnt& ntc = nrnthread_chkpnt[nt.id];
    ntc.file_id = userParams.gidgroups[nt.id];

    nt.ncell = n_real_cell;
    nt.end = n_node;
    nt.n_real_output = n_real_output;

#if CHKPNTDEBUG
    ntc.n_outputgids = n_output;
    ntc.nmech = n_mech;
#endif

    /// Checkpoint in coreneuron is defined for both phase 1 and phase 2 since they are written
    /// together
    nt._ml_list = (Memb_list**) ecalloc_align(corenrn.get_memb_funcs().size(), sizeof(Memb_list*));

    auto& memb_func = corenrn.get_memb_funcs();
#if CHKPNTDEBUG
    ntc.mlmap = new Memb_list_chkpnt*[memb_func.size()];
    for (int i = 0; i < memb_func.size(); ++i) {
        ntc.mlmap[i] = nullptr;
    }
#endif

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

    int shadow_rhs_cnt = 0;
    nt.shadow_rhs_cnt = 0;

    NrnThreadMembList* tml_last = nullptr;
    for (int i = 0; i < n_mech; ++i) {
        auto tml =
            create_tml(nt, i, memb_func[mech_types[i]], shadow_rhs_cnt, mech_types, nodecounts);

        nt._ml_list[tml->index] = tml->ml;

#if CHKPNTDEBUG
        Memb_list_chkpnt* mlc = new Memb_list_chkpnt;
        ntc.mlmap[tml->index] = mlc;
#endif

        if (nt.tml) {
            tml_last->next = tml;
        } else {
            nt.tml = tml;
        }
        tml_last = tml;
    }

    if (shadow_rhs_cnt) {
        nt._shadow_rhs = (double*) ecalloc_align(nrn_soa_padded_size(shadow_rhs_cnt, 0),
                                                 sizeof(double));
        nt._shadow_d = (double*) ecalloc_align(nrn_soa_padded_size(shadow_rhs_cnt, 0),
                                               sizeof(double));
        nt.shadow_rhs_cnt = shadow_rhs_cnt;
    }

    nt.mapping = nullptr;  // section segment mapping

    nt._nidata = n_idata;
    if (nt._nidata)
        nt._idata = (int*) ecalloc(nt._nidata, sizeof(int));
    else
        nt._idata = nullptr;
    // see patternstim.cpp
    int extra_nv = (&nt == nrn_threads) ? nrn_extra_thread0_vdata : 0;
    nt._nvdata = n_vdata;
    if (nt._nvdata + extra_nv)
        nt._vdata = (void**) ecalloc_align(nt._nvdata + extra_nv, sizeof(void*));
    else
        nt._vdata = nullptr;

    // The data format begins with the matrix data
    int n_data_padded = nrn_soa_padded_size(nt.end, SOA_LAYOUT);
    nt._data = _data;
    nt._actual_rhs = nt._data + 0 * n_data_padded;
    nt._actual_d = nt._data + 1 * n_data_padded;
    nt._actual_a = nt._data + 2 * n_data_padded;
    nt._actual_b = nt._data + 3 * n_data_padded;
    nt._actual_v = nt._data + 4 * n_data_padded;
    nt._actual_area = nt._data + 5 * n_data_padded;
    nt._actual_diam = n_diam ? nt._data + 6 * n_data_padded : nullptr;

    size_t offset = 6 * n_data_padded;
    if (n_diam) {
        // in the rare case that a mechanism has dparam with diam semantics
        // then actual_diam array added after matrix in nt._data
        // Generally wasteful since only a few diam are pointed to.
        // Probably better to move the diam semantics to the p array of the mechanism
        offset += n_data_padded;
    }

    // Memb_list.data points into the nt._data array.
    // Also count the number of Point_process
    int num_point_process = 0;
    for (auto tml = nt.tml; tml; tml = tml->next) {
        Memb_list* ml = tml->ml;
        int type = tml->index;
        int layout = corenrn.get_mech_data_layout()[type];
        int n = ml->nodecount;
        int sz = nrn_prop_param_size_[type];
        offset = nrn_soa_byte_align(offset);
        ml->data = nt._data + offset;
        offset += nrn_soa_padded_size(n, layout) * sz;
        if (corenrn.get_pnt_map()[type] > 0) {
            num_point_process += n;
        }
    }
    nt.pntprocs = new Point_process[num_point_process]{};  // includes acell with and without gid
    nt.n_pntproc = num_point_process;
    nt._ndata = offset;


    // matrix info
    nt._v_parent_index = v_parent_index;

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
    for (auto tml = nt.tml; tml; tml = tml->next, ++itml) {
        int type = tml->index;
        Memb_list* ml = tml->ml;
        int n = ml->nodecount;
        int szp = nrn_prop_param_size_[type];
        int szdp = nrn_prop_dparam_size_[type];

        const std::vector<int>& array_dims = corenrn.get_array_dims()[type];
        int layout = corenrn.get_mech_data_layout()[type];

        ml->nodeindices = (int*) ecalloc_align(ml->nodecount, sizeof(int));
        std::copy(tmls[itml].nodeindices.begin(), tmls[itml].nodeindices.end(), ml->nodeindices);

        mech_data_layout_transform<double>(ml->data, n, array_dims, layout);

        if (szdp) {
            ml->pdata = (int*) ecalloc_align(nrn_soa_padded_size(n, layout) * szdp, sizeof(int));
            std::copy(tmls[itml].pdata.begin(), tmls[itml].pdata.end(), ml->pdata);
            mech_data_layout_transform<int>(ml->pdata, n, szdp, layout);

#if CHKPNTDEBUG  // Not substantive. Only for debugging.
            Memb_list_chkpnt* mlc = ntc.mlmap[type];
            mlc->pdata_not_permuted = (int*) coreneuron::ecalloc_align(n * szdp, sizeof(int));
            if (layout == Layout::AoS) {  // only copy
                for (int i = 0; i < n; ++i) {
                    for (int j = 0; j < szdp; ++j) {
                        mlc->pdata_not_permuted[i * szdp + j] = ml->pdata[i * szdp + j];
                    }
                }
            } else if (layout == Layout::SoA) {  // transpose and unpad
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

        auto& r = tmls[itml].nmodlrandom;
        if (r.size()) {
            size_t ix{};
            uint32_t n_randomvar = r[ix++];
            assert(r.size() == 1 + n_randomvar + 5 * n_randomvar * n);
            std::vector<uint32_t> indices(n_randomvar);
            for (uint32_t i = 0; i < n_randomvar; ++i) {
                indices[i] = r[ix++];
            }
            int cnt = ml->nodecount;
            for (auto index: indices) {
                // should we also verify that index on corenrn side same as on nrn side?
                // sonarcloud thinks ml_pdata can be nullptr, so ...
                assert(index >= 0 && index < szdp);
                for (int i = 0; i < n; ++i) {
                    nrnran123_State* state = nrnran123_newstream3(r[ix], r[ix + 1], r[ix + 2]);
                    nrnran123_setseq(state, r[ix + 3], char(r[ix + 4]));
                    ix += 5;
                    int ipd = ml->pdata[nrn_i_layout(i, cnt, index, szdp, layout)];
                    assert(ipd >= 0 && ipd < n_vdata + extra_nv);
                    nt._vdata[ipd] = state;
                }
            }
        }
    }

    // pnt_offset needed for SelfEvent transfer from NEURON. Not needed on GPU.
    // Ugh. Related but not same as NetReceiveBuffer._pnt_offset
    nt._pnt_offset = pnt_offset;

    pdata_relocation(nt, memb_func);

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
    if (interleave_permute_type) {
        nt._permute = interleave_order(nt.id, nt.ncell, nt.end, nt._v_parent_index);
    }
    if (nt._permute) {
        int* p = nt._permute;
        permute_data(nt._actual_a, nt.end, p);
        permute_data(nt._actual_b, nt.end, p);
        permute_data(nt._actual_area, nt.end, p);
        permute_data(nt._actual_v,
                     nt.end,
                     p);  // need if restore or finitialize does not initialize voltage
        if (nt._actual_diam) {
            permute_data(nt._actual_diam, nt.end, p);
        }
        // index values change as well as ordering
        permute_ptr(nt._v_parent_index, nt.end, p);
        node_permute(nt._v_parent_index, nt.end, p);

#if CORENRN_DEBUG
        for (int i = 0; i < nt.end; ++i) {
            printf("parent[%d] = %d\n", i, nt._v_parent_index[i]);
        }
#endif

        // specify the ml->_permute and sort the nodeindices
        // Have to calculate all the permute before updating pdata in case
        // POINTER to data of other mechanisms exist.
        for (auto tml = nt.tml; tml; tml = tml->next) {
            if (tml->ml->nodeindices) {  // not artificial
                permute_nodeindices(tml->ml, p);
            }
        }
        for (auto tml = nt.tml; tml; tml = tml->next) {
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

    set_dependencies(nt, memb_func);

    fill_before_after_lists(nt, memb_func);

    // for fast watch statement checking
    // setup a list of types that have WATCH statement
    {
        int sz = 0;  // count the types with WATCH
        for (auto tml = nt.tml; tml; tml = tml->next) {
            if (corenrn.get_watch_check()[tml->index]) {
                ++sz;
            }
        }
        if (sz) {
            nt._watch_types = (int*) ecalloc(sz + 1, sizeof(int));  // nullptr terminated
            sz = 0;
            for (auto tml = nt.tml; tml; tml = tml->next) {
                if (corenrn.get_watch_check()[tml->index]) {
                    nt._watch_types[sz++] = tml->index;
                }
            }
        }
    }
    auto& pnttype2presyn = corenrn.get_pnttype2presyn();
    auto& nrn_has_net_event_ = corenrn.get_has_net_event();
    // create the nt.pnt2presyn_ix array of arrays.
    nt.pnt2presyn_ix = (int**) ecalloc(nrn_has_net_event_.size(), sizeof(int*));
    for (size_t i = 0; i < nrn_has_net_event_.size(); ++i) {
        Memb_list* ml = nt._ml_list[nrn_has_net_event_[i]];
        if (ml && ml->nodecount > 0) {
            nt.pnt2presyn_ix[i] = (int*) ecalloc(ml->nodecount, sizeof(int));
        }
    }

    // Real cells are at the beginning of the nt.presyns followed by
    // acells (with and without gids mixed together)
    // Here we associate the real cells with voltage pointers and
    // acell PreSyn with the Point_process.
    // nt.presyns order same as output_vindex order
#if CHKPNTDEBUG
    ntc.output_vindex = new int[nt.n_presyn];
    memcpy(ntc.output_vindex, output_vindex.data(), nt.n_presyn * sizeof(int));
#endif
    if (nt._permute) {
        // only indices >= 0 (i.e. _actual_v indices) will be changed.
        node_permute(output_vindex.data(), nt.n_presyn, nt._permute);
    }
#if CHKPNTDEBUG
    ntc.output_threshold = new double[n_real_output];
    memcpy(ntc.output_threshold, output_threshold.data(), n_real_output * sizeof(double));
#endif

    for (int i = 0; i < nt.n_presyn; ++i) {  // real cells
        PreSyn* ps = nt.presyns + i;

        int ix = output_vindex[i];
        if (ix == -1 && i < n_real_output) {  // real cell without a presyn
            continue;
        }
        if (ix < 0) {
            ix = -ix;
            int index = ix / 1000;
            int type = ix % 1000;
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

    // initial net_send_buffer size about 1% of number of presyns
    // nt._net_send_buffer_size = nt.ncell/100 + 1;
    // but, to avoid reallocation complexity on GPU ...
    nt._net_send_buffer_size = n_real_output;
    nt._net_send_buffer = (int*) ecalloc_align(nt._net_send_buffer_size, sizeof(int));

    int nnetcon = nt.n_netcon;

    // it may happen that Point_process structures will be made unnecessary
    // by factoring into NetCon.

#if CHKPNTDEBUG
    ntc.pnttype = new int[nnetcon];
    ntc.pntindex = new int[nnetcon];
    memcpy(ntc.pnttype, pnttype.data(), nnetcon * sizeof(int));
    memcpy(ntc.pntindex, pntindex.data(), nnetcon * sizeof(int));
#endif
    for (int i = 0; i < nnetcon; ++i) {
        int type = pnttype[i];
        if (type > 0) {
            int index = pnt_offset[type] + pntindex[i];  /// Potentially uninitialized pnt_offset[],
                                                         /// check for previous assignments
            NetCon& nc = nt.netcons[i];
            nc.target_ = nt.pntprocs + index;
            nc.active_ = true;
        }
    }

    handle_weights(nt, nnetcon, ntc);

    get_info_from_bbcore(nt, memb_func, ntc);

    set_vec_play(nt, ntc);

    if (!events.empty()) {
        userParams.checkPoints.restore_tqueue(nt, *this);
    }

    set_net_send_buffer(nt._ml_list, pnt_offset);
}
}  // namespace coreneuron
