#include "neuron/gpu/partrans.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/model_data.hpp"

#include "coreneuron/utils/offload.hpp"
#include "multicore.h"
#include "nrn_ansi.h"

extern int nrn_nthread;

#include <type_traits>

namespace neuron::gpu {
namespace {

bool native_gap_gpu_active() {
    return enabled() && backend_native();
}

template <typename Storage>
void sync_soa_storage_to_device(Storage const& storage) {
    storage.for_each_vector_for_gpu_upload(
        [](auto const& /*tag*/, auto const& vec, int /*field_index*/, int /*array_dim*/) {
            if (vec.empty()) {
                return;
            }
            using Value = typename std::decay_t<decltype(vec)>::value_type;
            if constexpr (std::is_same_v<Value, double>) {
                double const* const data = vec.data();
                if (nrn_target_is_present(data)) {
                    nrn_target_update_on_device(data, vec.size());
                }
            }
        });
}

}  // namespace

void gather_gap_voltage_sources_to_outsrc(int const* v_node_index_per_outsrc,
                                          int n_outsrc,
                                          double* outsrc_buf) {
#if defined(NRN_ENABLE_GPU)
    if (!native_gap_gpu_active() || n_outsrc <= 0 || !v_node_index_per_outsrc || !outsrc_buf) {
        return;
    }
    NrnThread& nt0 = nrn_threads[0];
    if (nt0.end <= 0) {
        return;
    }
    auto* const vec_v = nt0.node_voltage_storage();
    int const ncell = nt0.end;
    int const saved_compute_gpu = nt0.compute_gpu;
    nt0.compute_gpu = 1;
    nrn_pragma_acc(parallel loop present(vec_v [0:ncell], outsrc_buf [0:n_outsrc],
                                         v_node_index_per_outsrc [0:n_outsrc])
                       async(nt0.stream_id))
    nrn_pragma_omp(target teams distribute parallel for simd if(nt0.compute_gpu))
    for (int i = 0; i < n_outsrc; ++i) {
        int const ix = v_node_index_per_outsrc[i];
        if (ix >= 0) {
            outsrc_buf[i] = vec_v[ix];
        }
    }
    nrn_pragma_acc(update host(outsrc_buf [0:n_outsrc]) async(nt0.stream_id))
    nrn_pragma_omp(target update from(outsrc_buf [0:n_outsrc]) if(nt0.compute_gpu))
    nrn_pragma_acc(wait(nt0.stream_id))
    nt0.compute_gpu = saved_compute_gpu;
#else
    (void) v_node_index_per_outsrc;
    (void) n_outsrc;
    (void) outsrc_buf;
#endif
}

void gather_gap_voltage_sources_multithread(
    std::vector<std::vector<int>> const& outsrc_index_by_thread,
    std::vector<std::vector<int>> const& v_node_index_by_thread,
    int n_outsrc,
    double* outsrc_buf) {
#if defined(NRN_ENABLE_GPU)
    if (!native_gap_gpu_active() || n_outsrc <= 0 || !outsrc_buf) {
        return;
    }
    int const nthread = static_cast<int>(outsrc_index_by_thread.size());
    bool any_gpu{false};
    for (int tid = 0; tid < nthread; ++tid) {
        auto const& outsrc_indices = outsrc_index_by_thread[tid];
        auto const& v_indices = v_node_index_by_thread[tid];
        int const n = static_cast<int>(outsrc_indices.size());
        if (n <= 0 || outsrc_indices.size() != v_indices.size()) {
            continue;
        }
        NrnThread& nt = nrn_threads[tid];
        if (nt.end <= 0) {
            continue;
        }
        auto* const vec_v = nt.node_voltage_storage();
        int const ncell = nt.end;
        int const saved_compute_gpu = nt.compute_gpu;
        nt.compute_gpu = 1;
        any_gpu = true;
        int const* const outsrc_idx = outsrc_indices.data();
        int const* const v_node_idx = v_indices.data();
        nrn_pragma_acc(parallel loop present(vec_v [0:ncell], outsrc_buf [0:n_outsrc], outsrc_idx [0:n],
                                             v_node_idx [0:n]) async(nt.stream_id))
        nrn_pragma_omp(target teams distribute parallel for simd if(nt.compute_gpu))
        for (int i = 0; i < n; ++i) {
            int const ix = v_node_idx[i];
            if (ix >= 0) {
                outsrc_buf[outsrc_idx[i]] = vec_v[ix];
            }
        }
        nt.compute_gpu = saved_compute_gpu;
    }
    if (any_gpu) {
        for (int tid = 0; tid < nthread; ++tid) {
            nrn_pragma_acc(wait(nrn_threads[tid].stream_id))
        }
        nrn_pragma_acc(update host(outsrc_buf [0:n_outsrc]))
        nrn_pragma_omp(target update from(outsrc_buf [0:n_outsrc]))
    }
#else
    (void) outsrc_index_by_thread;
    (void) v_node_index_by_thread;
    (void) n_outsrc;
    (void) outsrc_buf;
#endif
}

void sync_insrc_buf_to_device(double* insrc_buf, int n_insrc) {
#if defined(NRN_ENABLE_GPU)
    if (!native_gap_gpu_active() || n_insrc <= 0 || !insrc_buf) {
        return;
    }
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        NrnThread& nt = nrn_threads[tid];
        if (!nt.compute_gpu) {
            continue;
        }
        int const saved_compute_gpu = nt.compute_gpu;
        nt.compute_gpu = 1;
        nrn_pragma_acc(update device(insrc_buf [0:n_insrc]) async(nt.stream_id))
        nrn_pragma_omp(target update to(insrc_buf [0:n_insrc]) if(nt.compute_gpu))
        nrn_pragma_acc(wait(nt.stream_id))
        nt.compute_gpu = saved_compute_gpu;
        return;
    }
#else
    (void) insrc_buf;
    (void) n_insrc;
#endif
}

void sync_mechanism_storage_to_device_after_partrans() {
#if defined(NRN_ENABLE_GPU)
    if (!native_gap_gpu_active()) {
        return;
    }
    neuron::model().apply_to_mechanisms([&](auto const& mech_data) {
        sync_soa_storage_to_device(mech_data);
    });
#endif
}

}  // namespace neuron::gpu
