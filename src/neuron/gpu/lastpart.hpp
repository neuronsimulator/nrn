#pragma once

struct NrnThread;

namespace neuron::gpu {

/** True when mechanism state/nonvint can run on device (OpenACC mods, no host-only hooks). */
[[nodiscard]] bool nonvint_can_run_on_device(NrnThread const& nt) noexcept;

/** Push host-authoritative voltages and thread time before GPU nonvint/state. */
void prepare_nonvint_on_device(NrnThread& nt);

/** Wait for GPU nonvint/state kernels before host AFTER_SOLVE / recording. */
void finalize_nonvint_on_device(NrnThread& nt);

}  // namespace neuron::gpu