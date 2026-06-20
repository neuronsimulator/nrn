#pragma once

namespace neuron::gpu {

/** True when native GPU execution is active at runtime (wired in PR 10+). */
bool enabled() noexcept;

/**
 * True when solve_interleaved2 should use the CUDA launcher (cellorder.cu) instead of OpenACC.
 * Requires device data in CoreNEURON monolithic layout (PR 9 upload); false until then.
 */
bool use_cuda_launcher() noexcept;

}  // namespace neuron::gpu