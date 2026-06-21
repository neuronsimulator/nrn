#pragma once

struct NrnThread;

namespace neuron::gpu {

/** Pull node voltages to host before host-side VecPlay / stimulus updates. */
void sync_before_vecplay(NrnThread& nt);

/** Push node voltages back to device after VecPlay updates. */
void sync_after_vecplay(NrnThread& nt);

/** Push mechanism-updated matrix state to device before OpenACC axial loops. */
void sync_matrix_to_device_after_mechanisms(NrnThread& nt);

/** Pull matrix state to host before the host Hines solver runs. */
void sync_matrix_to_host_before_solve(NrnThread& nt);

/** Push post-solve voltages (and rhs) back to device for the next GPU step. */
void sync_voltage_to_device_after_solve(NrnThread& nt);

/** Ensure gap-junction source voltages are visible on host before MPI transfer. */
void sync_gap_after_voltage_update(NrnThread& nt);

}  // namespace neuron::gpu