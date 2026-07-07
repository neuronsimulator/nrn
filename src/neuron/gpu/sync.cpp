#include "neuron/gpu/sync.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/mechanism_phases.hpp"
#include "neuron/gpu/phase_timer.hpp"
#include "neuron/gpu/post_solve.hpp"

#include "coreneuron/utils/offload.hpp"
#include "membfunc.h"
#include "multicore.h"
#include "nonvintblock.h"
#include "nrn_ansi.h"
#include "nrncvode.h"  // nrn_thread_has_fixed_play

#include <cstring>

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
    return post_solve_needs_host_fallback(nt) ||
           nrn_thread_has_fixed_play(const_cast<NrnThread*>(&nt));
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

void sync_diagonal_to_device_before_axial_lhs(NrnThread& nt) noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!matrix_currents_on_device(nt) || !nt.compute_gpu || nt.end <= 0) {
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
    sync_node_voltages_to_device(nt);
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
