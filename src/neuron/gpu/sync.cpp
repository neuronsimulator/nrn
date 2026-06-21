#include "neuron/gpu/sync.hpp"

#include "coreneuron/utils/offload.hpp"
#include "multicore.h"

namespace neuron::gpu {
namespace {

void sync_node_voltages_to_host(NrnThread& nt) {
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
        nrn_pragma_acc(update device(vec_sav_rhs [0:nt.end]) if (nt.compute_gpu) async(nt.stream_id))
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

void sync_voltage_and_rhs_to_device(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
    if (!nt.compute_gpu || nt.end <= 0) {
        return;
    }
    auto* const vec_v = nt.node_voltage_storage();
    auto* const vec_rhs = nt.node_rhs_storage();
    nrn_pragma_acc(update device(vec_v [0:nt.end], vec_rhs [0:nt.end]) if (nt.compute_gpu)
                       async(nt.stream_id))
    nrn_pragma_omp(target update to(vec_v [0:nt.end], vec_rhs [0:nt.end]) if (nt.compute_gpu))
    nrn_pragma_acc(wait(nt.stream_id))
#else
    (void) nt;
#endif
}

}  // namespace

void sync_before_vecplay(NrnThread& nt) {
    sync_node_voltages_to_host(nt);
}

void sync_after_vecplay(NrnThread& nt) {
    sync_node_voltages_to_device(nt);
}

void sync_voltages_to_device_before_axial(NrnThread& nt) {
    sync_node_voltages_to_device(nt);
}

void sync_matrix_to_device_after_mechanisms(NrnThread& nt) {
    sync_matrix_arrays_to_device(nt);
}

void sync_diagonal_to_device_after_mechanisms(NrnThread& nt) {
#if defined(NRN_ENABLE_GPU)
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

void sync_voltage_to_device_after_solve(NrnThread& nt) {
    sync_voltage_and_rhs_to_device(nt);
}

void sync_gap_after_voltage_update(NrnThread& nt) {
    sync_node_voltages_to_host(nt);
}

}  // namespace neuron::gpu