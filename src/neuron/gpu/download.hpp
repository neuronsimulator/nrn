#pragma once

#include <cstddef>

struct NrnThread;

namespace neuron::gpu {

/** Steps between host downloads during psolve (0 = only at psolve end). */
[[nodiscard]] std::size_t download_flush_interval() noexcept;

void set_download_flush_interval(std::size_t interval) noexcept;

/** Reset the per-psolve step counter (call at psolve start). */
void reset_download_step_counter() noexcept;

/** Advance the per-psolve step counter after a fixed step. */
void advance_download_step_counter() noexcept;

/** True when the current step should pull recorded state to the host. */
[[nodiscard]] bool should_flush_download() noexcept;

/** Pull post-solve node voltages and fast_imem to the host for one thread. */
void batch_download_post_solve(NrnThread& nt);

/** Pull node voltages (and fast_imem when active) for host reads, ignoring compute_gpu. */
void download_thread_state_for_host_read(NrnThread& nt);

/** Pull voltages and fast_imem for all threads (recording / per-step flush). */
void batch_download_to_host();

/**
 * Pull sorted node SOA (matrix, voltage, ...) from device without touching
 * mechanism SOA. Use when host nonvint/AFTER_SOLVE already own mechanism state.
 */
void sync_node_soa_to_host_for_host_reads() noexcept;

/**
 * Pull sorted node and mechanism SOA vectors from device to host.
 * Required for mid-step checkpoints and HOC reads when device owns all state.
 */
void sync_state_to_host_for_host_reads() noexcept;

/**
 * Push sorted node and mechanism SOA from host to device after host lastpart tail
 * (AFTER_SOLVE, fixed_record, deliver_events).
 */
void sync_state_to_device_after_host_lastpart() noexcept;

/** Push host voltages to the device after HOC/VecPlay writes. */
void batch_upload_to_device();

/** Final download at psolve end (always runs when native GPU is active). */
void finalize_psolve_download();

}  // namespace neuron::gpu