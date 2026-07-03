#pragma once

#include <cstdint>
#include <string>

namespace neuron::gpu {

enum class MechanismGpuPhase : unsigned {
    None = 0,
    Current = 1u << 0,
    Jacobian = 1u << 1,
    Solve = 1u << 2,
};

constexpr MechanismGpuPhase operator|(MechanismGpuPhase lhs, MechanismGpuPhase rhs) noexcept {
    return static_cast<MechanismGpuPhase>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
}

constexpr MechanismGpuPhase& operator|=(MechanismGpuPhase& lhs, MechanismGpuPhase rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

constexpr bool has_gpu_phase(MechanismGpuPhase phases, MechanismGpuPhase flag) noexcept {
    return (static_cast<unsigned>(phases) & static_cast<unsigned>(flag)) != 0;
}

/** Per-mod registration from NMODL OpenACC codegen or built-in mechanisms (e.g. capacitance). */
void register_mechanism_gpu_phases(int type, MechanismGpuPhase phases) noexcept;

[[nodiscard]] bool mechanism_current_on_device(int type) noexcept;
[[nodiscard]] bool mechanism_jacobian_on_device(int type) noexcept;
[[nodiscard]] bool mechanism_solve_on_device(int type) noexcept;

/**
 * After model init, report whether the loaded model qualifies for GPU-native fixed-step
 * integration and which gates block qualification. Intended for pc.gpu_qualification().
 */
[[nodiscard]] std::string native_gpu_qualification_report();

/** True when Gates A–E pass for every active thread (Gate F is informational only). */
[[nodiscard]] bool model_qualifies_for_full_gpu_native() noexcept;

/**
 * Print qualification report to stderr and abort via hoc_execerror when native GPU is
 * enabled and the model is not qualified. No-op when GPU is off or backend is not native.
 */
void require_gpu_native_qualification_or_stop();

/** @deprecated Use native_gpu_qualification_report(); kept for pc.gpu_fixed_step_phases(). */
[[nodiscard]] std::string native_gpu_fixed_step_phase_report();

namespace detail {
void reset_mechanism_gpu_phases_for_testing() noexcept;
}  // namespace detail

}  // namespace neuron::gpu