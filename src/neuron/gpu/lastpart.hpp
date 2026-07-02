#pragma once

struct NrnThread;

namespace neuron::gpu {

/** True when mechanism state/nonvint can run on device (OpenACC mods, no host-only hooks). */
[[nodiscard]] bool nonvint_can_run_on_device(NrnThread const& nt) noexcept;

/** Push host-authoritative voltages and thread time before GPU nonvint/state. */
void prepare_nonvint_on_device(NrnThread& nt);

/** Wait for GPU nonvint/state kernels before host AFTER_SOLVE / recording. */
void finalize_nonvint_on_device(NrnThread& nt);

/** True when AFTER_SOLVE / deliver tail of lastpart must run on host (CPU parity). */
[[nodiscard]] bool lastpart_host_phases_required(NrnThread const& nt) noexcept;

/** Download device state and clear compute_gpu before host lastpart tail (AFTER_SOLVE, deliver). */
void begin_lastpart_host_phases(NrnThread& nt);

/** Restore compute_gpu after host lastpart tail. */
void end_lastpart_host_phases(NrnThread& nt);

}  // namespace neuron::gpu