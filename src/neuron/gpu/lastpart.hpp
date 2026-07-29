#pragma once

struct NrnThread;

namespace neuron::gpu {

/** True when mechanism state/nonvint can run on device (OpenACC mods, no host-only hooks). */
[[nodiscard]] bool nonvint_can_run_on_device(NrnThread const& nt) noexcept;

/**
 * True when STATE runs on device during nonvint (native path + structural
 * preconditions + compute_gpu). Device nonvint is mandatory under native —
 * there is no host STATE fallback and no env switch.
 */
[[nodiscard]] bool nonvint_state_on_device(NrnThread const& nt) noexcept;

/** Gate C structural predicate (ignores transient nt.compute_gpu). */
[[nodiscard]] bool nonvint_qualifies_for_gpu_native(NrnThread const& nt) noexcept;

/** Push thread time before GPU nonvint/state. */
void prepare_nonvint_on_device(NrnThread& nt);

/**
 * Wait for post_solve before device nonvint. Host voltages are mirrored once
 * per step immediately after post_solve_on_device (not here).
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
 * (AFTER_SOLVE / BEFORE_STEP art, or Vector.record not covered by native trajectory).
 * Gate F is green when this is false under device nonvint (ringtest default;
 * also green with pure Vector.record when trajectory_covers_fixed_record()).
 */
[[nodiscard]] bool lastpart_host_phases_required(NrnThread const& nt) noexcept;

/** Download device state and clear compute_gpu before host lastpart tail (AFTER_SOLVE, deliver). */
void begin_lastpart_host_phases(NrnThread& nt);

/** Restore compute_gpu after host lastpart tail. */
void end_lastpart_host_phases(NrnThread& nt);

}  // namespace neuron::gpu
