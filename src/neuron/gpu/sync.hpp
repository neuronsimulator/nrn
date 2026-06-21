#pragma once

struct NrnThread;

namespace neuron::gpu {

/** Pull node voltages to host before host-side VecPlay / stimulus updates. */
void sync_before_vecplay(NrnThread& nt);

/** Push node voltages back to device after VecPlay updates. */
void sync_after_vecplay(NrnThread& nt);

/** Ensure gap-junction source voltages are visible on host before MPI transfer. */
void sync_gap_after_voltage_update(NrnThread& nt);

}  // namespace neuron::gpu