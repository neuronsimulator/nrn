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
 * Legacy full-type upload. Host NET_RECEIVE uses the dirty/coalesce path
 * below (live RANGE only) so this is not the psolve hot path.
 */
void upload_present_mechanism_soa_to_device(int type) noexcept;

/**
 * Register float SoA columns that host NET_RECEIVE writes and device
 * CURRENT/STATE must see. n_fields == 0 means elide (nothing to push).
 * Unregistered types keep a full-column upload on flush.
 */
void register_host_net_receive_soa_fields(int type,
                                          int const* field_indices,
                                          int n_fields) noexcept;

/** True if this type uses host NET_RECEIVE (WATCH / BBCOREPOINTER) on native GPU. */
[[nodiscard]] bool host_net_receive_soa_registered(int type) noexcept;

/** After host NET_RECEIVE wrote RANGE: dirty this type (coalesced until flush). */
void mark_host_net_receive_soa_dirty(int type) noexcept;

/**
 * Deliver-wave coalesce: mark_dirty batches; end flushes dirty types once.
 * Nested. Depth 0 (finitialize / HOC) flushes immediately on mark.
 */
void begin_host_net_receive_soa_coalesce() noexcept;
void end_host_net_receive_soa_coalesce() noexcept;

/** Push dirty host-NR RANGE columns (registered live set, else full SoA). */
void flush_host_net_receive_soa_to_device() noexcept;

/** Push host voltages to the device after HOC/VecPlay writes. */
void batch_upload_to_device();

/** Final download at psolve end (always runs when native GPU is active). */
void finalize_psolve_download();

/**
 * If the model is already on device, push full host SOA/state to the device.
 * Call at psolve entry after host may have advanced (mode-2 continuerun).
 */
void refresh_device_from_host_if_on_device() noexcept;

namespace detail {
void reset_host_net_receive_soa_for_testing() noexcept;
}  // namespace detail

}  // namespace neuron::gpu