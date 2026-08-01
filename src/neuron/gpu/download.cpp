#include "neuron/gpu/download.hpp"

#include "multicore.h"
#include "neuron/gpu/check_thresh.hpp"
#include "neuron/gpu/config.hpp"
#include "neuron/gpu/mechanism_phases.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/offload.hpp"
#include "neuron/gpu/partrans.hpp"
#include "neuron/gpu/phase_timer.hpp"
#include "neuron/gpu/sync.hpp"
#include "neuron/gpu/trajectory.hpp"
#include "membfunc.h"
#include "neuron/model_data.hpp"
#include "nrn_ansi.h"

#include <cstring>
#include <type_traits>

namespace neuron::gpu {
namespace {

std::size_t g_flush_interval{0};
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

void download_sorted_node_soa() {
#if defined(NRN_ENABLE_GPU)
    download_soa_storage(neuron::model().node_data());
#endif
}

/** True when any registered CURRENT runs on the device. */
[[nodiscard]] bool any_mechanism_current_on_device() noexcept {
    for (int t = 0; t < n_memb_func; ++t) {
        if (mechanism_current_on_device(t)) {
            return true;
        }
    }
    return false;
}

/** True when device kernels own this mechanism's SoA for some phase. */
[[nodiscard]] bool mechanism_soa_device_authoritative(int type) noexcept {
    // Ions have no CURRENT/STATE device registration, but device CURRENT mechs
    // write ion RANGE (ina, ik, …) via dptr. After GPU psolve those fields are
    // device-authoritative when some CURRENT is on device (else host CURRENT
    // owns host ion SoA — do not clobber).
    if (nrn_is_ion(type)) {
        return any_mechanism_current_on_device();
    }
    return mechanism_solve_on_device(type) || mechanism_current_on_device(type) ||
           mechanism_jacobian_on_device(type);
}

void download_sorted_mechanism_soa() {
#if defined(NRN_ENABLE_GPU)
    // Never clobber host-authoritative mechanism SoA (host-only CURRENT, WATCH /
    // NET_RECEIVE mechs like Bounce with no device phases). Device→host of a
    // stale init copy was wiping host WATCH/SelfEvent updates (watchrange).
    neuron::model().apply_to_mechanisms([&](auto& mech_data) {
        if (!mechanism_soa_device_authoritative(mech_data.type())) {
            return;
        }
        download_soa_storage(mech_data);
    });
#endif
}

[[nodiscard]] bool mechanism_ran_state_on_device(int type) noexcept {
    return mechanism_solve_on_device(type);
}

void download_device_state_mechanism_soa() {
#if defined(NRN_ENABLE_GPU)
    // Only pull mechanisms that integrated STATE on the device. Host-only CURRENT
    // mechanisms (e.g. pas) keep host-authoritative SOA updated during setup.
    neuron::model().apply_to_mechanisms([&](auto& mech_data) {
        if (!mechanism_ran_state_on_device(mech_data.type())) {
            return;
        }
        download_soa_storage(mech_data);
    });
#endif
}

void download_sorted_model_soa() {
#if defined(NRN_ENABLE_GPU)
    download_sorted_node_soa();
    download_sorted_mechanism_soa();
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

void upload_present_model_soa_to_device() noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !model_is_on_device()) {
        return;
    }
    upload_sorted_model_soa_to_device();
#endif
}

void upload_present_mechanism_soa_to_device(int type) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !model_is_on_device()) {
        return;
    }
    if (!neuron::model().is_valid_mechanism(type)) {
        return;
    }
    upload_soa_storage_to_device(neuron::model().mechanism_data(type));
#else
    (void) type;
#endif
}

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

void download_thread_state_for_host_read(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (nt.end <= 0) {
        return;
    }
    auto* const vec_v = nt.node_voltage_storage();
    nrn_pragma_acc(update host(vec_v [0:nt.end]) async(nt.stream_id))
    nrn_pragma_omp(target update from(vec_v [0:nt.end]))
    if (::nrn_use_fast_imem) {
        if (auto* const vec_sav_rhs = nt.node_sav_rhs_storage()) {
            nrn_pragma_acc(update host(vec_sav_rhs [0:nt.end]) async(nt.stream_id))
            nrn_pragma_omp(target update from(vec_sav_rhs [0:nt.end]))
        }
    }
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
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

void sync_node_soa_to_host_for_host_reads() noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !model_is_on_device()) {
        return;
    }
    phase_timer::Scope const timer{phase_timer::Id::download_flush};
    download_sorted_node_soa();
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        download_thread_state_for_host_read(nrn_threads[ith]);
    }
    sync_all_device_streams();
#endif
}

void sync_mechanism_soa_to_host_for_host_reads() noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !model_is_on_device()) {
        return;
    }
    phase_timer::Scope const timer{phase_timer::Id::download_flush};
    download_device_state_mechanism_soa();
    sync_all_device_streams();
#endif
}

void sync_state_to_host_for_host_reads() noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !model_is_on_device()) {
        return;
    }
    phase_timer::Scope const timer{phase_timer::Id::download_flush};
    download_sorted_model_soa();
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        download_thread_state_for_host_read(nrn_threads[ith]);
    }
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
    trajectory_finalize_psolve();
    sync_state_to_host_for_host_reads();
    phase_timer::print_summary();
    // P4 A+B: gap traffic report when NRN_GAP_TRAFFIC_STATS=1 or phase timer on.
    print_gap_traffic_stats("psolve-end");
    reset_download_step_counter();
#endif
}

void refresh_device_from_host_if_on_device() noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native()) {
        return;
    }
    if (model_is_on_device()) {
        // Mode-2 host continuerun advanced host SoA while mirrors stayed live.
        sync_state_to_device_after_host_lastpart();
    }
    // Always re-seed hysteresis from host V at psolve entry (mode-2 host half
    // may have crossed threshold while device mirrors were absent after teardown).
    reseed_threshold_flags_from_host_voltage();
#endif
}

}  // namespace neuron::gpu