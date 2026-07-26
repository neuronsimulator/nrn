#pragma once

struct NrnThread;

namespace neuron::gpu {

/** True when mechanism state/nonvint can run on device (OpenACC mods, no host-only hooks). */
[[nodiscard]] bool nonvint_can_run_on_device(NrnThread const& nt) noexcept;

/**
 * True when STATE should run on device during nonvint (OpenACC nrnivmodl mods and
 * NRN_NATIVE_GPU_DEVICE_NONVINT=1).
 */
[[nodiscard]] bool nonvint_state_on_device(NrnThread const& nt) noexcept;

/** Gate C structural predicate (ignores transient nt.compute_gpu). */
[[nodiscard]] bool nonvint_qualifies_for_gpu_native(NrnThread const& nt) noexcept;

/** Push thread time before GPU nonvint/state. */
void prepare_nonvint_on_device(NrnThread& nt);

/**
 * Pre-nonvint stream drain + full host SoA mirror (residual; still required
 * for OpenACC coherency on the current path). Not part of Gate F.
 */
void sync_before_device_nonvint(NrnThread& nt) noexcept;

/**
 * Wait for GPU nonvint; Gate F pulls host SoA only when
 * lastpart_host_phases_required (AFTER_SOLVE / BEFORE_STEP / Vector.record).
 */
void finalize_nonvint_on_device(NrnThread& nt);

/**
 * Pull sorted node and mechanism SOA from device (host lastpart tail or
 * pre-nonvint residual mirror).
 */
void sync_before_host_lastpart_tail(NrnThread& nt) noexcept;

/**
 * True when host lastpart tail needs a post-nonvint full SoA pull
 * (AFTER_SOLVE / BEFORE_STEP art or continuous Vector.record).
 * Gate F is green when this is false under device nonvint (ringtest default).
 */
[[nodiscard]] bool lastpart_host_phases_required(NrnThread const& nt) noexcept;

/** Download device state and clear compute_gpu before host lastpart tail (AFTER_SOLVE, deliver). */
void begin_lastpart_host_phases(NrnThread& nt);

/** Restore compute_gpu after host lastpart tail. */
void end_lastpart_host_phases(NrnThread& nt);

}  // namespace neuron::gpu
