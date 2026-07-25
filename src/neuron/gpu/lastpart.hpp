#pragma once

struct NrnThread;

namespace neuron::gpu {

/** True when mechanism state/nonvint can run on device (OpenACC mods, no host-only hooks). */
[[nodiscard]] bool nonvint_can_run_on_device(NrnThread const& nt) noexcept;

/**
 * True when STATE should run on device during nonvint (OpenACC nrnivmodl mods and
 * NRN_NATIVE_GPU_DEVICE_NONVINT=1). Requires mechanism SOA download before host AFTER_SOLVE.
 */
[[nodiscard]] bool nonvint_state_on_device(NrnThread const& nt) noexcept;

/** Gate C structural predicate (ignores transient nt.compute_gpu). */
[[nodiscard]] bool nonvint_qualifies_for_gpu_native(NrnThread const& nt) noexcept;

/** Push host-authoritative voltages and thread time before GPU nonvint/state. */
void prepare_nonvint_on_device(NrnThread& nt);

/** Mirror device SOA to host before GPU nonvint when OpenACC STATE runs on device. */
void sync_before_device_nonvint(NrnThread& nt) noexcept;

/** Wait for GPU nonvint/state kernels before host AFTER_SOLVE / recording. */
void finalize_nonvint_on_device(NrnThread& nt);

/**
 * Pull sorted node and mechanism SOA from device before host lastpart tail
 * (AFTER_SOLVE, fixed_record, deliver_events). Required when device nonvint ran.
 */
void sync_before_host_lastpart_tail(NrnThread& nt) noexcept;

/** True when AFTER_SOLVE / deliver tail of lastpart must run on host (CPU parity). */
[[nodiscard]] bool lastpart_host_phases_required(NrnThread const& nt) noexcept;

/** Download device state and clear compute_gpu before host lastpart tail (AFTER_SOLVE, deliver). */
void begin_lastpart_host_phases(NrnThread& nt);

/** Restore compute_gpu after host lastpart tail. */
void end_lastpart_host_phases(NrnThread& nt);

}  // namespace neuron::gpu