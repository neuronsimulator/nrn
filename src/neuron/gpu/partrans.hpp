#pragma once

#include <vector>

struct NrnThread;

namespace neuron::gpu {

/** Gather voltage sources from device vec_v into outsrc_buf on the host.
 *  Legacy MPI outsrc packing helper (one index per outsrc slot). */
void gather_gap_voltage_sources_to_outsrc(int const* v_node_index_per_outsrc,
                                         int n_outsrc,
                                         double* outsrc_buf);

/** Per-thread gather for pc.nthread(n>1); outsrc indices are scattered in outsrc_buf. */
void gather_gap_voltage_sources_multithread(
    std::vector<std::vector<int>> const& outsrc_index_by_thread,
    std::vector<std::vector<int>> const& v_node_index_by_thread,
    int n_outsrc,
    double* outsrc_buf);

/**
 * CoreNEURON-style local source mailbox gather (S0/S1).
 * For each thread, slot_by_tid[tid][i] is the index into host/device mailbox;
 * vnode_by_tid[tid][i] is the v_node_index on that thread. Device reads vec_v,
 * writes mailbox[slot], then updates mailbox to host.
 */
/** @return true if at least one thread gathered from device vec_v. */
bool gather_gap_voltage_mailbox(std::vector<std::vector<int>> const& slot_by_tid,
                                std::vector<std::vector<int>> const& vnode_by_tid,
                                double* mailbox,
                                int n_mailbox);

/** Push insrc_buf to device after MPI exchange. */
void sync_insrc_buf_to_device(double* insrc_buf, int n_insrc);

/**
 * Sparse push of transferred target scalars to device (not full mech SoA).
 * host_ptrs[i] must already hold the transferred value on the host.
 */
void scatter_gap_targets_to_device(double* const* host_ptrs, int n);

/**
 * Push only the named mechanism types' floating SoA to device (after host target writes).
 * Use for gap target mechs like HalfGap that have no STATE — never call with density mechs
 * that own device-authoritative STATE (hh, etc.).
 */
void sync_gap_target_mechs_to_device(int const* mech_types, int n_types);

/** @deprecated Full SoA push clobbers device STATE — do not use on native product path. */
void sync_mechanism_storage_to_device_after_partrans();

}  // namespace neuron::gpu
