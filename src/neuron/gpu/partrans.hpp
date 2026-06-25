#pragma once

struct NrnThread;

namespace neuron::gpu {

/** Gather voltage sources from device vec_v into outsrc_buf on the host. */
void gather_gap_voltage_sources_to_outsrc(int const* v_node_index_per_outsrc,
                                         int n_outsrc,
                                         double* outsrc_buf);

/** Push insrc_buf to device after MPI exchange. */
void sync_insrc_buf_to_device(double* insrc_buf, int n_insrc);

/** Push mechanism SOA storage to device after host partrans scatter. */
void sync_mechanism_storage_to_device_after_partrans();

}  // namespace neuron::gpu
