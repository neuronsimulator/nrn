#include "neuron/gpu/upload.hpp"

#include "coreneuron/permute/data_layout.hpp"
#include "membfunc.h"
#include "multicore.h"
#include "neuron/gpu/net_receive_buffer.hpp"
#include "neuron/gpu/net_send_buffer.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/model_data.hpp"
#include "nrnoc_ml.h"

#include <stdexcept>
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

/**
 * Locate the mechanism float SoA column that owns @p ptr (USEION shadows).
 * Used to convert ion pdata double* tables into (host base, int index) form.
 */
bool find_float_soa_home(double* ptr, double*& base_out, std::size_t& count_out) {
    if (!ptr) {
        return false;
    }
    auto& model = neuron::model();
    auto const ntypes = model.mechanism_storage_size();
    for (std::size_t type = 0; type < ntypes; ++type) {
        if (!model.is_valid_mechanism(static_cast<int>(type))) {
            continue;
        }
        auto const n_inst = model.mechanism_data(static_cast<int>(type)).size();
        if (n_inst == 0) {
            continue;
        }
        int const n_fields = mechanism::get_field_count<double>(static_cast<int>(type));
        auto* const bases = mechanism::get_data_ptrs<double>(static_cast<int>(type));
        auto* const dims = mechanism::get_array_dims<double>(static_cast<int>(type));
        if (!bases || n_fields <= 0) {
            continue;
        }
        for (int f = 0; f < n_fields; ++f) {
            if (!bases[f]) {
                continue;
            }
            int const dim = dims ? dims[f] : 1;
            if (dim <= 0) {
                continue;
            }
            auto const count = n_inst * static_cast<std::size_t>(dim);
            if (ptr >= bases[f] && ptr < bases[f] + count) {
                base_out = bases[f];
                count_out = count;
                return true;
            }
        }
    }
    return false;
}

void record_upload(UploadState& state,
                   void const* host,
                   std::size_t count,
                   std::size_t sizeof_elem) {
    state.record(host, count, sizeof_elem);
}

void upload_net_receive_buffer(Memb_list* ml, Memb_list* d_ml, UploadState& state) {
    auto* const nrb = ml->_net_receive_buffer;
    if (!nrb) {
        return;
    }
    upload_net_receive_buffer_to_device(ml);
    if (!nrb->device_uploaded) {
        return;
    }
    auto* const d_nrb = static_cast<NetReceiveBuffer_t*>(nrn_target_deviceptr(nrb));
    record_upload(state, nrb, 1, sizeof(NetReceiveBuffer_t));
    nrn_target_memcpy_to_device(&(d_ml->_net_receive_buffer), &d_nrb, 1);
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
    auto* const host_row_ptr_table = new Datum*[static_cast<std::size_t>(padded_n)]();
    for (int i = 0; i < padded_n; ++i) {
        Datum const* host_row = (i < n) ? ml->pdata[i] : padding_row.data();
        Datum* const d_row = nrn_target_copyin(host_row, static_cast<std::size_t>(szdp));
        if (i < n) {
            record_upload(state, host_row, static_cast<std::size_t>(szdp), sizeof(Datum));
        }
        host_row_ptr_table[static_cast<std::size_t>(i)] = d_row;
    }

    Datum** const d_pdata_rows =
        nrn_target_copyin(host_row_ptr_table, static_cast<std::size_t>(padded_n));
    state.record_cpu_owned(host_row_ptr_table, static_cast<std::size_t>(padded_n), sizeof(Datum*));
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
        Datum* const d_thread = nrn_target_copyin(ml->_thread,
                                                  static_cast<std::size_t>(thread_size));
        record_upload(state, ml->_thread, static_cast<std::size_t>(thread_size), sizeof(Datum));
        nrn_target_memcpy_to_device(&(d_ml->_thread), &d_thread, 1);
    }

    upload_mechanism_pdata(ml, type, d_ml, state);
    upload_net_receive_buffer(ml, d_ml, state);
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
            device_ptrs[static_cast<std::size_t>(type)] = nrn_target_deviceptr(nt._ml_list[type]);
        }
    }
    nrn_target_memcpy_to_device(d_ml_list, device_ptrs.data(), static_cast<std::size_t>(n_type));

    auto* const d_nt = nrn_target_deviceptr(&nt);
    nrn_target_memcpy_to_device(&(d_nt->_ml_list), &d_ml_list, 1);
}

}  // namespace

void upload_mechanism_pointer_tables(model_sorted_token& sorted, UploadState& state) {
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    auto& cache = sorted.cache();
    for (std::size_t type = 0; type < cache.mechanism.size(); ++type) {
        if (type == static_cast<std::size_t>(MORPHOLOGY)) {
            continue;
        }
        if (!neuron::model().is_valid_mechanism(static_cast<int>(type))) {
            continue;
        }
        auto& mech = cache.mechanism[type];

        double* const* const host_data_ptrs =
            mechanism::get_data_ptrs<double>(static_cast<int>(type));
        int const n_fp_fields = mechanism::get_field_count<double>(static_cast<int>(type));
        if (host_data_ptrs && n_fp_fields > 0) {
            auto const n_fields = static_cast<std::size_t>(n_fp_fields);
            auto* const staging = new double*[n_fields];
            for (std::size_t i = 0; i < n_fields; ++i) {
                staging[i] = host_data_ptrs[i] ? nrn_target_deviceptr(host_data_ptrs[i]) : nullptr;
            }
            state.record_cpu_owned(staging, n_fields, sizeof(double*));
            // Host-readable table of device SoA bases; make_instance runs on host before kernels.
            mech.gpu_data_ptrs = staging;
        }

        if (mech.pdata_ptr_cache.empty()) {
            continue;
        }

        auto const n_pdata_fields = mech.pdata_ptr_cache.size();
        auto* const device_bases_staging = new double* const*[n_pdata_fields]();
        auto* const soa_base_staging = new double*[n_pdata_fields]();
        auto* const soa_index_staging = new int*[n_pdata_fields]();
        auto* const soa_count_staging = new std::size_t[n_pdata_fields]();
        auto* const soa_index_n_staging = new std::size_t[n_pdata_fields]();
        bool any_pdata_field = false;
        bool any_soa_index = false;

        for (std::size_t field = 0; field < n_pdata_fields; ++field) {
            if (!mech.pdata_ptr_cache[field]) {
                continue;
            }
            auto const& host_row = mech.pdata[field];
            if (host_row.empty()) {
                continue;
            }
            auto* const row_staging = new double*[host_row.size()];
            for (std::size_t i = 0; i < host_row.size(); ++i) {
                row_staging[i] = host_row[i] ? nrn_target_deviceptr(host_row[i]) : nullptr;
            }
            double** const d_row =
                nrn_target_copyin(row_staging, static_cast<std::size_t>(host_row.size()));
            device_bases_staging[field] = d_row;
            state.record_cpu_owned(row_staging, host_row.size(), sizeof(double*));
            any_pdata_field = true;

            // CoreNEURON-style ion path: host SoA base + int index for present().
            // (deviceptr of device SoA base was illegal-address on nvc++; RANGE-style
            // present of host pointers matches ion_cur / _present_fp_N.)
            double* soa_base = nullptr;
            std::size_t soa_count = 0;
            bool indexable = true;
            auto* const indices = new int[host_row.size()]();
            for (std::size_t i = 0; i < host_row.size(); ++i) {
                double* const p = host_row[i];
                if (!p) {
                    indexable = false;
                    break;
                }
                if (!soa_base) {
                    if (!find_float_soa_home(p, soa_base, soa_count)) {
                        indexable = false;
                        break;
                    }
                } else if (p < soa_base || p >= soa_base + soa_count) {
                    indexable = false;
                    break;
                }
                indices[i] = static_cast<int>(p - soa_base);
            }
            if (indexable && soa_base) {
                // Keep host index row alive; copyin so present() finds device mapping.
                (void) nrn_target_copyin(indices, host_row.size());
                state.record_cpu_owned(indices, host_row.size(), sizeof(int));
                soa_base_staging[field] = soa_base;  // host pointer (already copyin via SoA)
                soa_index_staging[field] = indices;  // host pointer (just copyin)
                soa_count_staging[field] = soa_count;
                soa_index_n_staging[field] = host_row.size();
                any_soa_index = true;
            } else {
                delete[] indices;
            }
        }

        if (!any_pdata_field) {
            delete[] device_bases_staging;
            delete[] soa_base_staging;
            delete[] soa_index_staging;
            delete[] soa_count_staging;
            delete[] soa_index_n_staging;
            continue;
        }

        state.record_cpu_owned(device_bases_staging, n_pdata_fields, sizeof(double* const*));
        // Host-readable table of device-resident pdata rows (device pointer values).
        mech.gpu_pdata_ptr_cache = device_bases_staging;

        if (any_soa_index) {
            state.record_cpu_owned(soa_base_staging, n_pdata_fields, sizeof(double*));
            state.record_cpu_owned(soa_index_staging, n_pdata_fields, sizeof(int*));
            state.record_cpu_owned(soa_count_staging, n_pdata_fields, sizeof(std::size_t));
            state.record_cpu_owned(soa_index_n_staging, n_pdata_fields, sizeof(std::size_t));
            mech.gpu_pdata_soa_base = soa_base_staging;
            mech.gpu_pdata_soa_index = soa_index_staging;
            mech.gpu_pdata_soa_count = soa_count_staging;
            mech.gpu_pdata_soa_index_n = soa_index_n_staging;
        } else {
            delete[] soa_base_staging;
            delete[] soa_index_staging;
            delete[] soa_count_staging;
            delete[] soa_index_n_staging;
            mech.gpu_pdata_soa_base = nullptr;
            mech.gpu_pdata_soa_index = nullptr;
            mech.gpu_pdata_soa_count = nullptr;
            mech.gpu_pdata_soa_index_n = nullptr;
        }
    }
#else
    (void) sorted;
    (void) state;
    throw std::runtime_error("neuron::gpu::upload_mechanism_pointer_tables requires OpenACC");
#endif
}

void upload_mechanism_lists(UploadState& state) {
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        auto& nt = nrn_threads[ith];
        ensure_thread_net_receive_buffers(&nt);
        for (auto* tml = nt.tml; tml; tml = tml->next) {
            upload_mechanism_shell(tml->ml, tml->index, state);
        }
        upload_thread_ml_list(nt, state);
    }
}

}  // namespace neuron::gpu