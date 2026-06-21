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

/** Pull all thread state needed for HOC reads and Vector.record. */
void batch_download_to_host();

/** Push host voltages to the device after HOC/VecPlay writes. */
void batch_upload_to_device();

/** Final download at psolve end (always runs when native GPU is active). */
void finalize_psolve_download();

}  // namespace neuron::gpu