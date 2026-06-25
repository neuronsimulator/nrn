#include "neuron/gpu/partrans.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/model_data.hpp"

#include "coreneuron/utils/offload.hpp"
#include "multicore.h"
#include "nrn_ansi.h"

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

void sync_insrc_buf_to_device(double* insrc_buf, int n_insrc) {
#if defined(NRN_ENABLE_GPU)
    if (!native_gap_gpu_active() || n_insrc <= 0 || !insrc_buf) {
        return;
    }
    NrnThread& nt0 = nrn_threads[0];
    int const saved_compute_gpu = nt0.compute_gpu;
    nt0.compute_gpu = 1;
    nrn_pragma_acc(update device(insrc_buf [0:n_insrc]) async(nt0.stream_id))
    nrn_pragma_omp(target update to(insrc_buf [0:n_insrc]) if(nt0.compute_gpu))
    nrn_pragma_acc(wait(nt0.stream_id))
    nt0.compute_gpu = saved_compute_gpu;
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
