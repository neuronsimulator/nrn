#pragma once

#include <string_view>

namespace neuron::gpu {

enum class Backend {
    Coreneuron,
    Native,
};

/** True when gpu.enable is set and the build has NRN_ENABLE_GPU. */
bool enabled() noexcept;

/** True when gpu.backend is native. */
bool backend_native() noexcept;

/** Current runtime backend selection. */
Backend backend() noexcept;

/** @brief Set gpu.enable (no-op when NRN_ENABLE_GPU is off). */
void set_enable(bool value) noexcept;

/** @brief Set gpu.backend from "native" or "coreneuron". */
void set_backend(std::string_view name);

/**
 * True when solve_interleaved2 should use the CUDA launcher (cellorder.cu).
 * Requires native backend with device upload; scaffolding returns false until PR 12.
 */
bool use_cuda_launcher() noexcept;

namespace detail {
void reset_config_for_testing();
void set_enable_for_testing(bool value);
void set_backend_for_testing(Backend value);
}  // namespace detail

}  // namespace neuron::gpu
