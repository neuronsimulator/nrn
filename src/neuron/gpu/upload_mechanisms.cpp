#include "neuron/gpu/upload.hpp"

#include "coreneuron/permute/data_layout.hpp"
#include "membfunc.h"
#include "multicore.h"
#include "neuron/gpu/net_send_buffer.hpp"
#include "neuron/gpu/offload.hpp"
#include "nrnoc_ml.h"

#include <vector>

extern int nrn_nthread;
extern int* nrn_prop_dparam_size_;

namespace neuron::gpu {
namespace {

constexpr int k_soa_pad = 8;

int mechanism_padded_count(int count) {
    if (count <= 0) {
        return 0;
    }
    return ((count + k_soa_pad - 1) / k_soa_pad) * k_soa_pad;
}

void record_upload(UploadState& state, void const* host, std::size_t count, std::size_t sizeof_elem) {
    state.record(host, count, sizeof_elem);
}

void upload_net_send_buffer(Memb_list* ml, Memb_list* d_ml, UploadState& state) {
    auto* const nsb = ml->_net_send_buffer;
    if (!nsb) {
        return;
    }
    auto* const d_nsb = nrn_target_copyin(nsb, 1);
    record_upload(state, nsb, 1, sizeof(NetSendBuffer_t));
    nrn_target_memcpy_to_device(&(d_ml->_net_send_buffer), &d_nsb, 1);

    int* d_iptr = nrn_target_copyin(nsb->_sendtype, static_cast<std::size_t>(nsb->_size));
    record_upload(state, nsb->_sendtype, static_cast<std::size_t>(nsb->_size), sizeof(int));
    nrn_target_memcpy_to_device(&(d_nsb->_sendtype), &d_iptr, 1);

    d_iptr = nrn_target_copyin(nsb->_vdata_index, static_cast<std::size_t>(nsb->_size));
    record_upload(state, nsb->_vdata_index, static_cast<std::size_t>(nsb->_size), sizeof(int));
    nrn_target_memcpy_to_device(&(d_nsb->_vdata_index), &d_iptr, 1);

    d_iptr = nrn_target_copyin(nsb->_pnt_index, static_cast<std::size_t>(nsb->_size));
    record_upload(state, nsb->_pnt_index, static_cast<std::size_t>(nsb->_size), sizeof(int));
    nrn_target_memcpy_to_device(&(d_nsb->_pnt_index), &d_iptr, 1);

    d_iptr = nrn_target_copyin(nsb->_weight_index, static_cast<std::size_t>(nsb->_size));
    record_upload(state, nsb->_weight_index, static_cast<std::size_t>(nsb->_size), sizeof(int));
    nrn_target_memcpy_to_device(&(d_nsb->_weight_index), &d_iptr, 1);

    double* d_dptr = nrn_target_copyin(nsb->_nsb_t, static_cast<std::size_t>(nsb->_size));
    record_upload(state, nsb->_nsb_t, static_cast<std::size_t>(nsb->_size), sizeof(double));
    nrn_target_memcpy_to_device(&(d_nsb->_nsb_t), &d_dptr, 1);

    d_dptr = nrn_target_copyin(nsb->_nsb_flag, static_cast<std::size_t>(nsb->_size));
    record_upload(state, nsb->_nsb_flag, static_cast<std::size_t>(nsb->_size), sizeof(double));
    nrn_target_memcpy_to_device(&(d_nsb->_nsb_flag), &d_dptr, 1);
}

void upload_mechanism_pdata(Memb_list* ml, int type, Memb_list* d_ml, UploadState& state) {
    int const szdp = nrn_prop_dparam_size_[type];
    if (!szdp || !ml->pdata || ml->nodecount <= 0) {
        return;
    }
    int const n = ml->nodecount;
    int const padded_n = mechanism_padded_count(n);
    ml->_nodecount_padded = padded_n;

    std::vector<Datum> padding_row(static_cast<std::size_t>(szdp));
    std::vector<Datum*> device_row_ptrs(static_cast<std::size_t>(padded_n), nullptr);
    for (int i = 0; i < padded_n; ++i) {
        Datum const* host_row = (i < n) ? ml->pdata[i] : padding_row.data();
        Datum* const d_row = nrn_target_copyin(host_row, static_cast<std::size_t>(szdp));
        if (i < n) {
            record_upload(state, host_row, static_cast<std::size_t>(szdp), sizeof(Datum));
        }
        device_row_ptrs[static_cast<std::size_t>(i)] = d_row;
    }

    Datum** const d_pdata_rows =
        nrn_target_copyin(device_row_ptrs.data(), device_row_ptrs.size());
    record_upload(state, device_row_ptrs.data(), device_row_ptrs.size(), sizeof(Datum*));
    nrn_target_memcpy_to_device(&(d_ml->pdata), &d_pdata_rows, 1);
}

void upload_mechanism_shell(Memb_list* ml, int type, UploadState& state) {
    if (!ml || ml->nodecount <= 0) {
        return;
    }

    ml->_nodecount_padded = mechanism_padded_count(ml->nodecount);
    auto* const d_ml = nrn_target_copyin(ml, 1);
    record_upload(state, ml, 1, sizeof(Memb_list));

    if (ml->nodeindices) {
        int* const d_ni = nrn_target_deviceptr(ml->nodeindices);
        nrn_target_memcpy_to_device(&(d_ml->nodeindices), &d_ni, 1);
    }

    int const thread_size = memb_func[type].thread_size_;
    if (thread_size > 0 && ml->_thread) {
        Datum* const d_thread = nrn_target_copyin(ml->_thread, static_cast<std::size_t>(thread_size));
        record_upload(state, ml->_thread, static_cast<std::size_t>(thread_size), sizeof(Datum));
        nrn_target_memcpy_to_device(&(d_ml->_thread), &d_thread, 1);
    }

    upload_mechanism_pdata(ml, type, d_ml, state);
    upload_net_send_buffer(ml, d_ml, state);
}

void upload_thread_ml_list(NrnThread& nt, UploadState& state) {
    if (!nt._ml_list) {
        return;
    }
    int const n_type = n_memb_func;
    Memb_list** const d_ml_list = nrn_target_copyin(nt._ml_list, static_cast<std::size_t>(n_type));
    record_upload(state, nt._ml_list, static_cast<std::size_t>(n_type), sizeof(Memb_list*));

    std::vector<Memb_list*> device_ptrs(static_cast<std::size_t>(n_type), nullptr);
    for (int type = 0; type < n_type; ++type) {
        if (nt._ml_list[type]) {
            device_ptrs[static_cast<std::size_t>(type)] =
                nrn_target_deviceptr(nt._ml_list[type]);
        }
    }
    nrn_target_memcpy_to_device(d_ml_list, device_ptrs.data(), static_cast<std::size_t>(n_type));

    auto* const d_nt = nrn_target_deviceptr(&nt);
    nrn_target_memcpy_to_device(&(d_nt->_ml_list), &d_ml_list, 1);
}

}  // namespace

void upload_mechanism_lists(UploadState& state) {
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        auto& nt = nrn_threads[ith];
        for (auto* tml = nt.tml; tml; tml = tml->next) {
            upload_mechanism_shell(tml->ml, tml->index, state);
        }
        upload_thread_ml_list(nt, state);
    }
}

}  // namespace neuron::gpu