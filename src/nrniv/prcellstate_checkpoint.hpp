#pragma once

struct NrnThread;

/** Phase-tagged prcellstate dumps during psolve (debug / parity). */
enum class PrcellCheckpointPhase {
    post_setup,
    post_solve,
    pre_nonvint,
    post_nonvint,
};

/**
 * Arm a one-shot checkpoint during the next psolve.
 * @param gid  presyn gid to dump (-1 clears / disables)
 * @param step 0-based step index from psolve start; use -1 to trigger by time instead
 * @param t    simulation time trigger when step < 0
 */
void nrn_prcellstate_checkpoint_arm(int gid, long step, double t);

/** Disable any armed checkpoint. */
void nrn_prcellstate_checkpoint_clear() noexcept;

/** Reset per-psolve step counter; call at the start of fixed-step integration. */
void nrn_prcellstate_checkpoint_psolve_begin() noexcept;

/** Advance the psolve step counter; call once per completed fixed step. */
void nrn_prcellstate_checkpoint_fixed_step_end() noexcept;

/**
 * If armed and the current step/time matches, download GPU state (when needed)
 * and write <gid>_<suffix>.nrndat for this phase. Only the NrnThread that owns
 * gid performs the dump.
 */
void nrn_prcellstate_checkpoint_maybe(PrcellCheckpointPhase phase, NrnThread& nt);