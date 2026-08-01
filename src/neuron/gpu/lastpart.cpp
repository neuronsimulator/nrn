#include "neuron/gpu/lastpart.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/download.hpp"
#include "neuron/gpu/mechanism_phases.hpp"
#include "neuron/gpu/phase_timer.hpp"
#include "neuron/gpu/post_solve.hpp"
#include "neuron/gpu/sync.hpp"
#include "neuron/gpu/trajectory.hpp"

#include "coreneuron/utils/offload.hpp"
#include "membfunc.h"
#include "multicore.h"
#include "nonvintblock.h"
#include "nrn_ansi.h"
#include "nrncvode.h"

#include <cstdio>

extern int use_sparse13;
extern void hoc_execerror(const char*, const char*);

namespace neuron::gpu {
namespace {

thread_local int g_saved_compute_gpu_for_nonvint{};
thread_local int g_saved_compute_gpu_for_lastpart_host{};
thread_local bool g_lastpart_host_phases_active{};
thread_local bool g_nonvint_state_on_device{};

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

/** True when lastpart host tail (AFTER_SOLVE / BEFORE_STEP / Vector.record) reads SoA. */
[[nodiscard]] bool lastpart_host_soa_consumers_present(NrnThread const& nt) noexcept {
    if (nt.tbl[AFTER_SOLVE] || nt.tbl[BEFORE_STEP]) {
        return true;
    }
    // Gate F: Vector.record alone does not force full SoA when native trajectory covers it.
    if (nrn_has_fixed_record_continuous()) {
        return !trajectory_covers_fixed_record();
    }
    return false;
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
    return nonvint_can_run_on_device(nt);
#else
    (void) nt;
    return false;
#endif
}

bool nonvint_qualifies_for_gpu_native(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    return nonvint_device_preconditions(nt);
#else
    (void) nt;
    return false;
#endif
}

void prepare_nonvint_on_device(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    g_nonvint_state_on_device = false;
    if (!enabled() || !backend_native()) {
        return;
    }
    if (nt.end <= 0) {
        return;
    }
    // Device nonvint is mandatory on the native path — no host STATE fallback.
    // Full Gate C (per-mech Solve registration) is enforced at psolve start via
    // require_gpu_native_qualification_or_stop(); structural blockers land here
    // as a hard error if anything slipped through (e.g. ALLOW_UNQUALIFIED).
    if (!nonvint_device_preconditions(nt)) {
        auto const report = native_gpu_qualification_report();
        std::fprintf(stderr, "%s", report.c_str());
        hoc_execerror(
            "Native GPU requires device nonvint (STATE on GPU); host fallback is not allowed. "
            "Model fails Gate C structural preconditions (sparse13, extracellular, Python "
            "nonvint block, or native backend inactive). See qualification report above. "
            "Use pc.gpu_qualification() / prcellstate phases to debug; do not expect silent "
            "CPU STATE under native.",
            nullptr);
    }
    g_nonvint_state_on_device = true;
    g_saved_compute_gpu_for_nonvint = nt.compute_gpu;
    nt.compute_gpu = 1;

    // Keep device nt._t in step with host. ACC STATE/CURRENT use host-captured
    // `_nrn_thread_t`, but other device paths may still read nt._t. Fence so the
    // update is visible before STATE.
    nrn_pragma_acc(update device(nt._t) if (nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target update to(nt._t) if (nt.compute_gpu))
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

void sync_before_host_lastpart_tail(NrnThread& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !model_is_on_device() || nt.id != 0) {
        return;
    }
    sync_all_device_streams();
    sync_state_to_host_for_host_reads();
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
    // Gate F: after device nonvint, pull full SoA only if host lastpart consumers
    // (AFTER_SOLVE / BEFORE_STEP / Vector.record) need it. Always drain streams.
    if (lastpart_host_phases_required(nt)) {
        sync_before_host_lastpart_tail(nt);
    } else if (nt.id == 0) {
        sync_all_device_streams();
    }
    nt.compute_gpu = g_saved_compute_gpu_for_nonvint;
    g_nonvint_state_on_device = false;
#else
    (void) nt;
#endif
}

void sync_before_device_nonvint(NrnThread& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    // Wait for post_solve only. Device owns vec_v for the whole psolve — no
    // host↔device V transfer here (or after post_solve on the ringtest path).
    if (!enabled() || !backend_native() || !nonvint_device_preconditions(nt) || nt.end <= 0) {
        return;
    }
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

bool lastpart_host_phases_required(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    // Device nonvint leaves SoA authoritative on the GPU. Host lastpart needs a
    // full SoA pull only when AFTER_SOLVE / BEFORE_STEP / fixed_record read it.
    // Gate F is green when this returns false (ringtest default).
    if (!enabled() || !backend_native() || !nonvint_device_preconditions(nt) ||
        post_solve_needs_host_fallback(nt)) {
        return false;
    }
    return lastpart_host_soa_consumers_present(nt);
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
    sync_before_host_lastpart_tail(nt);
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
