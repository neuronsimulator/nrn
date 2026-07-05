#include "neuron/gpu/lastpart.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/download.hpp"
#include "neuron/gpu/phase_timer.hpp"
#include "neuron/gpu/post_solve.hpp"
#include "neuron/gpu/sync.hpp"

#include "coreneuron/utils/offload.hpp"
#include "multicore.h"
#include "nonvintblock.h"
#include "nrn_ansi.h"

#include <cstdlib>

extern int use_sparse13;

namespace neuron::gpu {
namespace {

thread_local int g_saved_compute_gpu_for_nonvint{};
thread_local int g_saved_compute_gpu_for_lastpart_host{};
thread_local bool g_lastpart_host_phases_active{};
thread_local bool g_nonvint_state_on_device{};

[[nodiscard]] bool device_nonvint_enabled() noexcept {
    static int const enabled = [] {
        char const* const env = std::getenv("NRN_NATIVE_GPU_DEVICE_NONVINT");
        return env && env[0] == '1' && env[1] == '\0';
    }();
    return enabled != 0;
}

[[nodiscard]] bool nonvint_device_preconditions(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || nt.end <= 0) {
        return false;
    }
    if (nrn_nonvint_block || ::use_sparse13) {
        return false;
    }
    if (nt._ecell_memb_list) {
        return false;
    }
    return true;
#else
    (void) nt;
    return false;
#endif
}

}  // namespace

bool nonvint_can_run_on_device(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    return nonvint_device_preconditions(nt) && nt.compute_gpu;
#else
    (void) nt;
    return false;
#endif
}

bool nonvint_state_on_device(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    return device_nonvint_enabled() && nonvint_can_run_on_device(nt);
#else
    (void) nt;
    return false;
#endif
}

bool nonvint_qualifies_for_gpu_native(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    return device_nonvint_enabled() && nonvint_device_preconditions(nt);
#else
    (void) nt;
    return false;
#endif
}

void prepare_nonvint_on_device(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    g_nonvint_state_on_device = false;
    if (!nonvint_device_preconditions(nt) || !nonvint_state_on_device(nt)) {
        // OpenACC mods still integrate STATE on the host unless device nonvint is enabled.
        return;
    }
    g_nonvint_state_on_device = true;
    g_saved_compute_gpu_for_nonvint = nt.compute_gpu;
    nt.compute_gpu = 1;

    nrn_pragma_acc(update device(nt._t) if (nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target update to(nt._t) if (nt.compute_gpu))
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

void finalize_nonvint_on_device(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (!g_nonvint_state_on_device) {
        return;
    }
    nrn_pragma_acc(wait(nt.stream_id))
    nt.compute_gpu = g_saved_compute_gpu_for_nonvint;
    g_nonvint_state_on_device = false;
#else
    (void) nt;
#endif
}

bool lastpart_host_phases_required(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    return enabled() && backend_native() && nonvint_device_preconditions(nt) &&
           !post_solve_needs_host_fallback(nt) && nt.compute_gpu;
#else
    (void) nt;
    return false;
#endif
}

void begin_lastpart_host_phases(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (!lastpart_host_phases_required(nt)) {
        return;
    }
    // AFTER_SOLVE, fixed_record (BEFORE_STEP), and deliver_events still iterate
    // host-side; device nonvint left SOA current on the GPU. Mirror to host and
    // run the tail with compute_gpu=0 to match the CPU fixed-step path.
    g_saved_compute_gpu_for_lastpart_host = nt.compute_gpu;
    g_lastpart_host_phases_active = true;
    nt.compute_gpu = 0;
    if (nt.id == 0) {
        // Pull node/matrix SOA from device. Mechanism STATE is on the host after
        // host nonvint, or was downloaded in finalize_nonvint_on_device after
        // device OpenACC state kernels.
        sync_all_device_streams();
        sync_node_soa_to_host_for_host_reads();
    }
#else
    (void) nt;
#endif
}

void end_lastpart_host_phases(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (!g_lastpart_host_phases_active) {
        return;
    }
    if (nt.id == 0) {
        sync_state_to_device_after_host_lastpart();
    }
    nt.compute_gpu = g_saved_compute_gpu_for_lastpart_host;
    g_lastpart_host_phases_active = false;
#else
    (void) nt;
#endif
}

}  // namespace neuron::gpu