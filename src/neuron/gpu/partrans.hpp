#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

struct NrnThread;

namespace neuron::gpu {

/**
 * S5 / P4 traffic audit (product path should stay sparse; see doc/gpu/native-partrans.md).
 * Enable: NRN_GAP_TRAFFIC_STATS=1 (also auto-on when NRN_NATIVE_GPU_PHASE_TIMER=1).
 * Print: end of psolve (finalize) and optional atexit.
 */
struct GapTrafficStats {
    std::uint64_t steps{0};
    std::uint64_t vsrc_gathered{0};       // unique voltage sources read (mailbox slots)
    std::uint64_t msrc_gathered{0};       // MechRange sources staged
    std::uint64_t msrc_from_device{0};    // of those, device D→H
    std::uint64_t tar_scattered{0};       // sparse H→D target scalars (memcpy)
    std::uint64_t tar_field_fallback{0};  // times vgap-field column push used
    std::uint64_t bytes_d2h{0};           // gather D→H (bulk + scalar)
    std::uint64_t bytes_h2d{0};           // scatter H→D (+ field columns + insrc bulk)
    std::uint64_t d2h_bulk_calls{0};      // OpenACC update host of whole buffer
    std::uint64_t h2d_bulk_calls{0};      // OpenACC update device of whole buffer
    std::uint64_t d2h_scalar_calls{0};    // acc_memcpy_from_device per double
    std::uint64_t h2d_scalar_calls{0};    // memcpy_to_device per double
    std::uint64_t full_v_pulls{0};        // anti-pattern: full vec_v host pull
    std::uint64_t bulk_mech_pushes{0};    // anti-pattern: full mech SoA H→D
    std::uint64_t same_thread_device{0};   // S5 opt-in same-thread device edges
    std::uint64_t buffer_path_edges{0};   // host mailbox → host tar writes
    std::uint64_t gather_ok{0};
    std::uint64_t gather_fallback{0};
    std::uint64_t scatter_miss{0};
};

[[nodiscard]] GapTrafficStats const& gap_traffic_stats() noexcept;
[[nodiscard]] bool gap_traffic_stats_enabled() noexcept;
/** Opt-in same-thread on-device copy (NRN_GAP_SAME_THREAD_DEVICE=1). Default off. */
[[nodiscard]] bool gap_same_thread_device_enabled() noexcept;
void print_gap_traffic_stats(char const* where) noexcept;
/** Zero counters (start of psolve). */
void gap_traffic_reset() noexcept;

void gap_traffic_note_step() noexcept;
void gap_traffic_note_v_gather(int n_src) noexcept;
void gap_traffic_note_mech_gather(int n_src, int n_from_device) noexcept;
void gap_traffic_note_scatter(int n_ok, int field_fallback, std::size_t field_bytes) noexcept;
void gap_traffic_note_d2h_bulk(std::size_t bytes) noexcept;
void gap_traffic_note_h2d_bulk(std::size_t bytes) noexcept;
void gap_traffic_note_d2h_scalar(int n) noexcept;
void gap_traffic_note_h2d_scalar(int n) noexcept;
void gap_traffic_note_full_v_pull() noexcept;
void gap_traffic_note_bulk_mech() noexcept;
void gap_traffic_note_same_thread(int n_edges) noexcept;
void gap_traffic_note_buffer_edges(int n_edges) noexcept;

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

/**
 * S4 MechRange: sparse D→H gather of non-voltage RANGE sources into a host mailbox.
 * host_ptrs[i] is a host scalar (typically mid-SoA ion/RANGE); mech_types lists
 * candidate mechanism types whose device field bases may contain those scalars.
 * mailbox[i] receives the device value when mapped, else the host value.
 * @return true if at least one scalar was read from device.
 */
bool gather_gap_mech_range_mailbox(double* const* host_ptrs,
                                   int n,
                                   int const* mech_types,
                                   int n_types,
                                   double* mailbox);

/**
 * S5 opt-in: same-thread voltage edges — device tar = vec_v[vnode] (no host hop).
 * Parallel arrays of length n; returns edges successfully written on device.
 */
int same_thread_voltage_device_copy(std::vector<std::vector<int>> const& vnode_by_tid,
                                   std::vector<std::vector<double*>> const& host_tar_by_tid,
                                   int const* mech_types,
                                   int n_types);

/**
 * S5 opt-in: same-thread MechRange edges — device tar = device src (sparse).
 * @return edges successfully copied on device.
 */
int same_thread_mech_device_copy(double* const* host_src,
                                double* const* host_tar,
                                int n,
                                int const* mech_types,
                                int n_types);

/** Push insrc_buf to device after MPI exchange. */
void sync_insrc_buf_to_device(double* insrc_buf, int n_insrc);

/**
 * Sparse push of transferred target scalars to device (not full mech SoA).
 * host_ptrs[i] must already hold the transferred value on the host.
 */
void scatter_gap_targets_to_device(double* const* host_ptrs, int n);

/**
 * Prefer this: mid-SoA resolve via target mech field bases + vgap-column fallback
 * when per-scalar present misses. Never full-mech SoA.
 */
void scatter_gap_targets_to_device(double* const* host_ptrs,
                                   int n,
                                   int const* mech_types,
                                   int n_types);

/**
 * Legacy full-mech SoA H→D. **No-op** unless NRN_GAP_BULK_MECH_PUSH=1 (debug only).
 * Product path must not clobber device CURRENT/STATE fields.
 */
void sync_gap_target_mechs_to_device(int const* mech_types, int n_types);

/** @deprecated Full SoA push clobbers device STATE — do not use on native product path. */
void sync_mechanism_storage_to_device_after_partrans();

}  // namespace neuron::gpu
