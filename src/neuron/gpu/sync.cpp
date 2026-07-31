#include "neuron/gpu/sync.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/mechanism_phases.hpp"
#include "neuron/gpu/phase_timer.hpp"
#include "neuron/gpu/post_solve.hpp"

#include "coreneuron/utils/offload.hpp"
#include "membfunc.h"
#include "multicore.h"
#include "neuron/model_data.hpp"
#include "nonvintblock.h"
#include "nrn_ansi.h"
#include "nrncvode.h"  // nrn_thread_has_fixed_play, nrn_fixed_play_foreach_pd
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/offload.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <vector>

extern int use_sparse13;

namespace neuron::gpu {
namespace {

void sync_node_voltages_to_host(NrnThread& nt) {
    phase_timer::bump(phase_timer::Id::vecplay_sync);
#if defined(NRN_ENABLE_GPU)
    if (!nt.compute_gpu || nt.end <= 0) {
        return;
    }
    auto* const vec_v = nt.node_voltage_storage();
    nrn_pragma_acc(update host(vec_v [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target update from(vec_v [0:nt.end]) if (nt.compute_gpu))
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

void sync_node_voltages_to_device(NrnThread& nt) {
    phase_timer::bump(phase_timer::Id::vecplay_sync);
#if defined(NRN_ENABLE_GPU)
    if (!nt.compute_gpu || nt.end <= 0) {
        return;
    }
    auto* const vec_v = nt.node_voltage_storage();
    nrn_pragma_acc(update device(vec_v [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target update to(vec_v [0:nt.end]) if (nt.compute_gpu))
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

void sync_matrix_arrays_to_device(NrnThread& nt) {
    phase_timer::Scope const timer{phase_timer::Id::matrix_sync};
    phase_timer::bump(phase_timer::Id::matrix_sync);
#if defined(NRN_ENABLE_GPU)
    if (!nt.compute_gpu || nt.end <= 0) {
        return;
    }
    auto* const vec_rhs = nt.node_rhs_storage();
    auto* const vec_d = nt.node_d_storage();
    nrn_pragma_acc(update device(vec_rhs [0:nt.end], vec_d [0:nt.end]) if (nt.compute_gpu)
                       async(nt.stream_id))
    nrn_pragma_omp(target update to(vec_rhs [0:nt.end], vec_d [0:nt.end]) if (nt.compute_gpu))
    if (auto* const vec_sav_rhs = nt.node_sav_rhs_storage()) {
        nrn_pragma_acc(update device(vec_sav_rhs [0:nt.end]) if (nt.compute_gpu)
                           async(nt.stream_id))
        nrn_pragma_omp(target update to(vec_sav_rhs [0:nt.end]) if (nt.compute_gpu))
    }
    if (auto* const vec_sav_d = nt.node_sav_d_storage()) {
        nrn_pragma_acc(update device(vec_sav_d [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
        nrn_pragma_omp(target update to(vec_sav_d [0:nt.end]) if (nt.compute_gpu))
    }
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

void sync_matrix_arrays_to_host(NrnThread& nt) {
    phase_timer::Scope const timer{phase_timer::Id::matrix_sync};
    phase_timer::bump(phase_timer::Id::matrix_sync);
#if defined(NRN_ENABLE_GPU)
    if (!nt.compute_gpu || nt.end <= 0) {
        return;
    }
    auto* const vec_rhs = nt.node_rhs_storage();
    auto* const vec_d = nt.node_d_storage();
    nrn_pragma_acc(update host(vec_rhs [0:nt.end], vec_d [0:nt.end]) if (nt.compute_gpu)
                       async(nt.stream_id))
    nrn_pragma_omp(target update from(vec_rhs [0:nt.end], vec_d [0:nt.end]) if (nt.compute_gpu))
    if (auto* const vec_sav_rhs = nt.node_sav_rhs_storage()) {
        nrn_pragma_acc(update host(vec_sav_rhs [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
        nrn_pragma_omp(target update from(vec_sav_rhs [0:nt.end]) if (nt.compute_gpu))
    }
    if (auto* const vec_sav_d = nt.node_sav_d_storage()) {
        nrn_pragma_acc(update host(vec_sav_d [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
        nrn_pragma_omp(target update from(vec_sav_d [0:nt.end]) if (nt.compute_gpu))
    }
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

}  // namespace

bool host_voltage_is_authoritative(NrnThread const& nt) noexcept {
    // fixed_play alone does NOT make host V authoritative: play commonly targets
    // mechanism RANGE (IClamp.amp). Under native, device owns V; treating any
    // fixed_play as host-authoritative H→D'd stale host V every step and froze
    // cells at -65 (testvecplay residual). Play-into-V is handled by selective
    // push in sync_after_vecplay.
    return post_solve_needs_host_fallback(nt);
}

bool matrix_rhs_d_stays_on_device_for_solve(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!matrix_rhs_d_qualifies_for_gpu_native(nt) || !nt.compute_gpu) {
        return false;
    }
    return true;
#else
    (void) nt;
    return false;
#endif
}

bool matrix_rhs_d_qualifies_for_gpu_native(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || nt.end <= 0 || nrn_nonvint_block || ::use_sparse13) {
        return false;
    }
    // Extracellular rhs/lhs hooks (nrn_rhs_ext, nrn_setup_ext) run only when this is set.
    if (nt._ecell_memb_list) {
        return false;
    }
    return true;
#else
    (void) nt;
    return false;
#endif
}

bool matrix_currents_qualify_for_gpu_native(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!matrix_rhs_d_qualifies_for_gpu_native(nt)) {
        return false;
    }
    for (auto* tml = nt.tml; tml; tml = tml->next) {
        if (nrn_is_ion(tml->index)) {
            continue;
        }
        // BEFORE/AFTER instrumentation stays host-side (see mechanism_has_before_after).
        if (mechanism_has_before_after(tml->index)) {
            continue;
        }
        if (memb_func[tml->index].current && !mechanism_current_on_device(tml->index)) {
            return false;
        }
        if (memb_func[tml->index].jacob && !mechanism_jacobian_on_device(tml->index)) {
            return false;
        }
    }
    if (nt.tml && nt.tml->index == CAP && !mechanism_jacobian_on_device(CAP)) {
        return false;
    }
    return true;
#else
    (void) nt;
    return false;
#endif
}

bool matrix_has_host_jacobians(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (nt.end <= 0) {
        return false;
    }
    for (auto* tml = nt.tml; tml; tml = tml->next) {
        if (nrn_is_ion(tml->index)) {
            continue;
        }
        if (mechanism_has_before_after(tml->index)) {
            continue;
        }
        if (memb_func[tml->index].jacob && !mechanism_jacobian_on_device(tml->index)) {
            return true;
        }
    }
    if (nt.tml && nt.tml->index == CAP && !mechanism_jacobian_on_device(CAP)) {
        return true;
    }
    return false;
#else
    (void) nt;
    return false;
#endif
}

bool matrix_has_host_currents(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (nt.end <= 0) {
        return false;
    }
    for (auto* tml = nt.tml; tml; tml = tml->next) {
        if (nrn_is_ion(tml->index)) {
            continue;
        }
        if (mechanism_has_before_after(tml->index)) {
            continue;
        }
        if (memb_func[tml->index].current && !mechanism_current_on_device(tml->index)) {
            return true;
        }
    }
    return false;
#else
    (void) nt;
    return false;
#endif
}

bool mechanism_matrix_jacobian_on_device(NrnThread const& nt, int type) noexcept {
#if defined(NRN_ENABLE_GPU)
    return nt.compute_gpu && matrix_rhs_d_stays_on_device_for_solve(nt) &&
           mechanism_jacobian_on_device(type);
#else
    (void) nt;
    (void) type;
    return false;
#endif
}

bool matrix_currents_on_device(NrnThread const& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!matrix_rhs_d_stays_on_device_for_solve(nt) || nt.end <= 0) {
        return false;
    }
    if (matrix_has_host_currents(nt)) {
        return false;
    }
    for (auto* tml = nt.tml; tml; tml = tml->next) {
        if (nrn_is_ion(tml->index)) {
            continue;
        }
        if (memb_func[tml->index].jacob && !mechanism_jacobian_on_device(tml->index)) {
            return false;
        }
    }
    if (nt.tml && nt.tml->index == CAP && !mechanism_jacobian_on_device(CAP)) {
        return false;
    }
    return true;
#else
    (void) nt;
    return false;
#endif
}

void zero_matrix_rhs_on_device(NrnThread& nt, int begin, int end) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!matrix_rhs_d_stays_on_device_for_solve(nt) || !nt.compute_gpu || begin >= end) {
        return;
    }
    phase_timer::Scope const timer{phase_timer::Id::matrix_sync};
    auto* const vec_rhs = nt.node_rhs_storage();
    nrn_pragma_acc(parallel loop present(vec_rhs [begin:end]) if (nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target teams distribute parallel for if(nt.compute_gpu))
    for (int i = begin; i < end; ++i) {
        vec_rhs[i] = 0.;
    }
    if (auto* const vec_sav_rhs = nt.node_sav_rhs_storage()) {
        nrn_pragma_acc(parallel loop present(vec_sav_rhs [begin:end]) if (nt.compute_gpu)
                           async(nt.stream_id))
        nrn_pragma_omp(target teams distribute parallel for if(nt.compute_gpu))
        for (int i = begin; i < end; ++i) {
            vec_sav_rhs[i] = 0.;
        }
    }
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
    (void) begin;
    (void) end;
#endif
}

void zero_matrix_diagonal_on_device(NrnThread& nt, int begin, int end) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!matrix_rhs_d_stays_on_device_for_solve(nt) || !nt.compute_gpu || begin >= end) {
        return;
    }
    phase_timer::Scope const timer{phase_timer::Id::matrix_sync};
    auto* const vec_d = nt.node_d_storage();
    nrn_pragma_acc(parallel loop present(vec_d [begin:end]) if (nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target teams distribute parallel for if(nt.compute_gpu))
    for (int i = begin; i < end; ++i) {
        vec_d[i] = 0.;
    }
    if (auto* const vec_sav_d = nt.node_sav_d_storage()) {
        nrn_pragma_acc(parallel loop present(vec_sav_d [begin:end]) if (nt.compute_gpu)
                           async(nt.stream_id))
        nrn_pragma_omp(target teams distribute parallel for if(nt.compute_gpu))
        for (int i = begin; i < end; ++i) {
            vec_sav_d[i] = 0.;
        }
    }
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
    (void) begin;
    (void) end;
#endif
}

void transform_sav_rhs_membrane_only_on_device(NrnThread& nt, int begin, int end) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!matrix_rhs_d_stays_on_device_for_solve(nt) || !nt.compute_gpu || begin >= end) {
        return;
    }
    auto* const vec_rhs = nt.node_rhs_storage();
    auto* const vec_sav_rhs = nt.node_sav_rhs_storage();
    if (!vec_sav_rhs) {
        return;
    }
    nrn_pragma_acc(parallel loop present(vec_rhs [begin:end], vec_sav_rhs [begin:end]) if (
                       nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target teams distribute parallel for if(nt.compute_gpu))
    for (int i = begin; i < end; ++i) {
        vec_sav_rhs[i] -= vec_rhs[i];
    }
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
    (void) begin;
    (void) end;
#endif
}

void transform_sav_d_membrane_only_on_device(NrnThread& nt, int begin, int end) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!matrix_rhs_d_stays_on_device_for_solve(nt) || !nt.compute_gpu || begin >= end) {
        return;
    }
    auto* const vec_d = nt.node_d_storage();
    auto* const vec_sav_d = nt.node_sav_d_storage();
    if (!vec_sav_d) {
        return;
    }
    nrn_pragma_acc(parallel loop present(vec_d [begin:end], vec_sav_d [begin:end]) if (
                       nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target teams distribute parallel for if(nt.compute_gpu))
    for (int i = begin; i < end; ++i) {
        vec_sav_d[i] = vec_d[i] - vec_sav_d[i];
    }
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
    (void) begin;
    (void) end;
#endif
}

void sync_diagonal_to_device_before_axial_lhs(NrnThread& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (matrix_currents_on_device(nt) || !nt.compute_gpu || nt.end <= 0) {
        return;
    }
    phase_timer::Scope const timer{phase_timer::Id::matrix_sync};
    phase_timer::bump(phase_timer::Id::matrix_sync);
    auto* const vec_d = nt.node_d_storage();
    nrn_pragma_acc(update device(vec_d [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target update to(vec_d [0:nt.end]) if (nt.compute_gpu))
    if (auto* const vec_sav_d = nt.node_sav_d_storage()) {
        nrn_pragma_acc(update device(vec_sav_d [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
        nrn_pragma_omp(target update to(vec_sav_d [0:nt.end]) if (nt.compute_gpu))
    }
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

void sync_before_vecplay(NrnThread& nt) {
    if (!nrn_thread_has_fixed_play(&nt)) {
        return;
    }
    sync_node_voltages_to_host(nt);
}

void sync_after_vecplay(NrnThread& nt) {
    if (!nrn_thread_has_fixed_play(&nt)) {
        return;
    }
#if defined(NRN_ENABLE_GPU)
    if (!nt.compute_gpu || !model_is_on_device()) {
        // Host-only path: nothing to push (play already wrote host storage).
        return;
    }
    // Collect played host addresses for this thread.
    std::vector<double*> play_ptrs;
    play_ptrs.reserve(16);
    nrn_fixed_play_foreach_pd(
        &nt,
        [](double* p, void* ctx) {
            if (p) {
                static_cast<std::vector<double*>*>(ctx)->push_back(p);
            }
        },
        &play_ptrs);
    if (play_ptrs.empty()) {
        return;
    }

    // Push node voltages ONLY if a play target lies in this thread's V storage.
    // Unconditional H→D of host V resets device V to stale -65 every step
    // (host V is not authoritative under native unless a host post_solve path
    // ran). That was the testvecplay residual: amp reached device, but V was
    // clobbered each step.
    if (nt.end > 0) {
        double* const vec_v = nt.node_voltage_storage();
        bool play_v = false;
        for (double* p: play_ptrs) {
            if (p >= vec_v && p < vec_v + nt.end) {
                play_v = true;
                break;
            }
        }
        if (play_v) {
            sync_node_voltages_to_device(nt);
        }
    }

    // Push only mechanism SoA double columns that contain a played address
    // (IClamp.amp, …). Column-scoped update — not full model SoA (would
    // clobber device STATE) and not interior-pointer update alone (unreliable
    // on this stack; same lesson as gap scatter).
    neuron::model().apply_to_mechanisms([&](auto& mech_data) {
        mech_data.for_each_vector_for_gpu_upload(
            [&](auto const& /*tag*/, auto const& vec, int /*field_index*/, int /*array_dim*/) {
                if (vec.empty()) {
                    return;
                }
                using Value = typename std::decay_t<decltype(vec)>::value_type;
                if constexpr (std::is_same_v<Value, double>) {
                    double* const base = const_cast<double*>(vec.data());
                    std::size_t const nvec = vec.size();
                    bool hit = false;
                    for (double* p: play_ptrs) {
                        if (p >= base && p < base + nvec) {
                            hit = true;
                            break;
                        }
                    }
                    if (!hit || !nrn_target_is_present(base)) {
                        return;
                    }
                    nrn_target_update_on_device(base, nvec);
                }
            });
    });
#else
    (void) nt;
#endif
}

void sync_voltages_to_device_after_lastpart(NrnThread& nt) {
    sync_node_voltages_to_device(nt);
}

void sync_voltages_to_device_before_axial(NrnThread& nt) {
    if (!host_voltage_is_authoritative(nt)) {
        return;
    }
    sync_node_voltages_to_device(nt);
}

void sync_matrix_to_device_after_mechanisms(NrnThread& nt) {
    // OpenACC CURRENT updates rhs on device; skip host push.
    if (matrix_currents_on_device(nt)) {
        return;
    }
    // Vectorized mechanisms still evaluate CURRENT on the host even when rhs/d
    // stay on device for the solver. Push host rhs after zero+CURRENT so the
    // subsequent device axial loop does not accumulate on stale post_solve rhs.
    sync_matrix_arrays_to_device(nt);
}

void sync_diagonal_to_device_after_mechanisms(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (matrix_currents_on_device(nt)) {
        return;
    }
    if (!nt.compute_gpu || nt.end <= 0) {
        return;
    }
    auto* const vec_d = nt.node_d_storage();
    nrn_pragma_acc(update device(vec_d [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target update to(vec_d [0:nt.end]) if (nt.compute_gpu))
    if (auto* const vec_sav_d = nt.node_sav_d_storage()) {
        nrn_pragma_acc(update device(vec_sav_d [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
        nrn_pragma_omp(target update to(vec_sav_d [0:nt.end]) if (nt.compute_gpu))
    }
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

void sync_matrix_to_host_before_solve(NrnThread& nt) {
    sync_matrix_arrays_to_host(nt);
}

void matrix_probe_maybe(NrnThread& nt, char const* tag) noexcept {
#if defined(NRN_ENABLE_GPU)
    static int enabled_cache = -1;
    static double tmax = 0.1;
    if (enabled_cache < 0) {
        char const* e = std::getenv("NRN_GPU_MATRIX_PROBE");
        enabled_cache = (e && e[0] && e[0] != '0') ? 1 : 0;
        if (char const* tm = std::getenv("NRN_GPU_MATRIX_PROBE_TMAX")) {
            tmax = std::atof(tm);
        }
    }
    if (!enabled_cache || !enabled() || !backend_native()) {
        return;
    }
    if (nt._t > tmax + 1e-15) {
        return;
    }
    if (nt.end <= 0) {
        std::fprintf(stderr,
                     "MATRIX_PROBE %s tid=%d t=%.9g compute_gpu=%d end=0 stays_dev=%d\n",
                     tag ? tag : "?",
                     nt.id,
                     nt._t,
                     nt.compute_gpu,
                     matrix_rhs_d_stays_on_device_for_solve(nt) ? 1 : 0);
        return;
    }
    // Authoritative snapshot: wait this thread's stream then pull d/rhs/v.
    if (nt.compute_gpu) {
        nrn_pragma_acc(wait(nt.stream_id))
        auto* const vec_rhs = nt.node_rhs_storage();
        auto* const vec_d = nt.node_d_storage();
        auto* const vec_v = nt.node_voltage_storage();
        nrn_pragma_acc(update host(vec_rhs [0:nt.end], vec_d [0:nt.end], vec_v [0:nt.end]))
        nrn_pragma_omp(target update from(vec_rhs [0:nt.end], vec_d [0:nt.end], vec_v [0:nt.end]))
    }
    auto* const vec_rhs = nt.node_rhs_storage();
    auto* const vec_d = nt.node_d_storage();
    auto* const vec_v = nt.node_voltage_storage();
    int const nshow = nt.end < 3 ? nt.end : 3;
    std::fprintf(stderr,
                 "MATRIX_PROBE %s tid=%d t=%.9g compute_gpu=%d stays_dev=%d ncell=%d end=%d",
                 tag ? tag : "?",
                 nt.id,
                 nt._t,
                 nt.compute_gpu,
                 matrix_rhs_d_stays_on_device_for_solve(nt) ? 1 : 0,
                 nt.ncell,
                 nt.end);
    for (int i = 0; i < nshow; ++i) {
        std::fprintf(stderr,
                     " |i=%d d=%.17g rhs=%.17g v=%.17g",
                     i,
                     vec_d[i],
                     vec_rhs[i],
                     vec_v[i]);
    }
    std::fprintf(stderr, "\n");
#else
    (void) nt;
    (void) tag;
#endif
}

void sync_matrix_to_device_before_solve(NrnThread& nt) {
    sync_matrix_arrays_to_device(nt);
}

void sync_rhs_to_host_after_solve(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (!nt.compute_gpu || nt.end <= 0) {
        return;
    }
    auto* const vec_rhs = nt.node_rhs_storage();
    nrn_pragma_acc(update host(vec_rhs [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
    nrn_pragma_omp(target update from(vec_rhs [0:nt.end]) if (nt.compute_gpu))
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

void sync_voltages_to_host_after_post_solve(NrnThread& nt) {
    sync_node_voltages_to_host(nt);
}

void sync_voltages_to_host_before_check_thresh(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || !nt.compute_gpu || nt.end <= 0) {
        return;
    }
    nrn_pragma_acc(wait(nt.stream_id))
#endif
    sync_node_voltages_to_host(nt);
}

void sync_voltages_to_host_before_nonvint(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || nt.end <= 0) {
        return;
    }
    phase_timer::bump(phase_timer::Id::vecplay_sync);
    auto* const vec_v = nt.node_voltage_storage();
    nrn_pragma_acc(update host(vec_v [0:nt.end]) async(nt.stream_id))
    nrn_pragma_omp(target update from(vec_v [0:nt.end]))
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

void sync_rhs_to_host_before_nonvint(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native() || nt.end <= 0) {
        return;
    }
    phase_timer::Scope const timer{phase_timer::Id::matrix_sync};
    auto* const vec_rhs = nt.node_rhs_storage();
    nrn_pragma_acc(update host(vec_rhs [0:nt.end]) async(nt.stream_id))
    nrn_pragma_omp(target update from(vec_rhs [0:nt.end]))
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

void sync_fast_imem_to_host_after_post_solve(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (!nt.compute_gpu || nt.end <= 0) {
        return;
    }
    if (!::nrn_use_fast_imem) {
        return;
    }
    if (auto* const vec_sav_rhs = nt.node_sav_rhs_storage()) {
        nrn_pragma_acc(update host(vec_sav_rhs [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
        nrn_pragma_omp(target update from(vec_sav_rhs [0:nt.end]) if (nt.compute_gpu))
        nrn_pragma_acc(wait(nt.stream_id))
    }
#else
    (void) nt;
#endif
}

void sync_gap_after_voltage_update(NrnThread& nt) {
    sync_node_voltages_to_host(nt);
}

void sync_gap_after_host_voltage_update(NrnThread& nt) {
    sync_node_voltages_to_device(nt);
}

void sync_all_device_streams() {
#if defined(NRN_ENABLE_GPU)
    for (int ith = 0; ith < nrn_nthread; ++ith) {
        nrn_pragma_acc(wait(nrn_threads[ith].stream_id))
        nrn_pragma_omp(taskwait)
    }
#else
#endif
}

}  // namespace neuron::gpu
