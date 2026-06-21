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

/** Number of GPUs to use per node (0 = all available). */
unsigned device_count() noexcept;

/** @brief Set gpu.device_count (0 = all available). */
void set_device_count(unsigned value) noexcept;

/**
 * True when solve_interleaved2 should use the CUDA launcher (cellorder.cu).
 * Requires native backend with device upload; scaffolding returns false until PR 12.
 */
bool use_cuda_launcher() noexcept;

/**
 * Phase B contract check for native GPU. Returns nullptr when configuration is
 * valid, otherwise a stable error message (fixed-step only; CVode must be off).
 */
[[nodiscard]] const char* native_gpu_configuration_error() noexcept;

namespace detail {
void reset_config_for_testing();
void set_enable_for_testing(bool value);
void set_backend_for_testing(Backend value);
void set_device_count_for_testing(unsigned value);
}  // namespace detail

}  // namespace neuron::gpu
