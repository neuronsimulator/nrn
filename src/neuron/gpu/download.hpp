#pragma once

#include <cstddef>

struct NrnThread;

namespace neuron::gpu {

/** Steps between optional per-step host downloads during psolve (0 = tstop mirror only). */
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
 * Waits all device streams before D→H (safe after Session E async kernels).
 */
void sync_node_soa_to_host_for_host_reads() noexcept;

/**
 * Pull STATEful mechanism SOA from device after device nonvint STATE integration.
 * Skips host-only CURRENT mechanisms (e.g. pas) whose host mirrors stay authoritative.
 * Waits all device streams before D→H.
 */
void sync_mechanism_soa_to_host_for_host_reads() noexcept;

/**
 * Pull sorted node and mechanism SOA vectors from device to host.
 * Required for mid-step checkpoints and HOC reads when device owns all state.
 * Waits all device streams before D→H so finalize_psolve_download / phases=0
 * prcellstate do not race async CURRENT/STATE/JACOB (Session E density).
 */
void sync_state_to_host_for_host_reads() noexcept;

/**
 * Push sorted node and mechanism SOA from host to device after host lastpart tail
 * (AFTER_SOLVE, fixed_record, deliver_events).
 */
void sync_state_to_device_after_host_lastpart() noexcept;

/**
 * Push host mechanism (+ node) SOA columns that are already present on device.
 * Used after host Vector.play updates RANGE (e.g. IClamp.amp) on the native path.
 */
void upload_present_model_soa_to_device() noexcept;

/**
 * Push one mechanism type's present double SoA columns host→device.
 * Used after host NET_RECEIVE/WATCH writes RANGE (e.g. hhwatch g,e) that device
 * CURRENT must see on the native path.
 */
void upload_present_mechanism_soa_to_device(int type) noexcept;

/** Push host voltages to the device after HOC/VecPlay writes. */
void batch_upload_to_device();

/** Final download at psolve end (always runs when native GPU is active). */
void finalize_psolve_download();

/**
 * If the model is already on device, push full host SOA/state to the device.
 * Call at psolve entry after host may have advanced (mode-2 continuerun).
 */
void refresh_device_from_host_if_on_device() noexcept;

}  // namespace neuron::gpu