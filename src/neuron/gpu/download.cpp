#include "neuron/gpu/download.hpp"

#include "multicore.h"
#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/gpu/phase_timer.hpp"
#include "neuron/gpu/sync.hpp"
#include "neuron/model_data.hpp"
#include "nrn_ansi.h"

#include <type_traits>

namespace neuron::gpu {
namespace {

std::size_t g_flush_interval{1};
std::size_t g_step_counter{0};

template <typename Storage>
void download_soa_storage(Storage const& storage) {
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    storage.for_each_vector_for_gpu_upload(
        [](auto const& /*tag*/, auto const& vec, int /*field_index*/, int /*array_dim*/) {
            if (vec.empty()) {
                return;
            }
            using Value = typename std::decay_t<decltype(vec)>::value_type;
            if constexpr (std::is_same_v<Value, double> || std::is_same_v<Value, int>) {
                Value const* const host = vec.data();
                if (!nrn_target_is_present(host)) {
                    return;
                }
                nrn_pragma_acc(update host(host [0:vec.size()]))
                nrn_pragma_omp(target update from(host [0:vec.size()]))
            }
        });
#else
    (void) storage;
#endif
}

void download_sorted_model_soa() {
#if defined(NRN_ENABLE_GPU)
    download_soa_storage(neuron::model().node_data());
    neuron::model().apply_to_mechanisms(
        [&](auto& mech_data) { download_soa_storage(mech_data); });
#endif
}

template <typename Storage>
void upload_soa_storage_to_device(Storage const& storage) {
#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
    storage.for_each_vector_for_gpu_upload(
        [](auto const& /*tag*/, auto const& vec, int /*field_index*/, int /*array_dim*/) {
            if (vec.empty()) {
                return;
            }
            using Value = typename std::decay_t<decltype(vec)>::value_type;
            if constexpr (std::is_same_v<Value, double> || std::is_same_v<Value, int>) {
                Value const* const host = vec.data();
                if (!nrn_target_is_present(host)) {
                    return;
                }
                nrn_pragma_acc(update device(host [0:vec.size()]))
                nrn_pragma_omp(target update to(host [0:vec.size()]))
            }
        });
#else
    (void) storage;
#endif
}

void upload_sorted_model_soa_to_device() {
#if defined(NRN_ENABLE_GPU)
    upload_soa_storage_to_device(neuron::model().node_data());
    neuron::model().apply_to_mechanisms(
        [&](auto& mech_data) { upload_soa_storage_to_device(mech_data); });
#endif
}

}  // namespace

std::size_t download_flush_interval() noexcept {
    return g_flush_interval;
}

void set_download_flush_interval(std::size_t interval) noexcept {
    g_flush_interval = interval;
}

void reset_download_step_counter() noexcept {
    g_step_counter = 0;
}

void advance_download_step_counter() noexcept {
    ++g_step_counter;
}

bool should_flush_download() noexcept {
    if (g_flush_interval == 0) {
        return false;
    }
    return (g_step_counter % g_flush_interval) == 0;
}

void batch_download_post_solve(NrnThread& nt) {
    sync_voltages_to_host_after_post_solve(nt);
    sync_fast_imem_to_host_after_post_solve(nt);
}

void batch_download_to_host() {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native()) {
        return;
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        batch_download_post_solve(nrn_threads[ith]);
    }
#endif
}

void sync_state_to_host_for_host_reads() noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !model_is_on_device()) {
        return;
    }
    phase_timer::Scope const timer{phase_timer::Id::download_flush};
    download_sorted_model_soa();
    batch_download_to_host();
    sync_all_device_streams();
#endif
}

void sync_state_to_device_after_host_lastpart() noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !model_is_on_device()) {
        return;
    }
    phase_timer::Scope const timer{phase_timer::Id::matrix_sync};
    upload_sorted_model_soa_to_device();
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        auto& nt = nrn_threads[ith];
        if (nt.end <= 0) {
            continue;
        }
        auto* const vec_v = nt.node_voltage_storage();
        nrn_pragma_acc(update device(vec_v [0:nt.end]) async(nt.stream_id))
        nrn_pragma_omp(target update to(vec_v [0:nt.end]))
        nrn_pragma_acc(update device(nt._t) async(nt.stream_id))
        nrn_pragma_omp(target update to(nt._t))
    }
    sync_all_device_streams();
#endif
}

void batch_upload_to_device() {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native()) {
        return;
    }
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        sync_after_vecplay(nrn_threads[ith]);
    }
#endif
}

void finalize_psolve_download() {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native()) {
        return;
    }
    sync_state_to_host_for_host_reads();
    phase_timer::print_summary();
    reset_download_step_counter();
#endif
}

}  // namespace neuron::gpu