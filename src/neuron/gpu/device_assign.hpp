#pragma once

namespace neuron::gpu {

/** Assign the default OpenACC/CUDA device (idempotent). */
void assign_device();

/** Device id selected by the last assign_device() call, or -1 if not yet assigned. */
int assigned_device_id() noexcept;

namespace detail {
void reset_device_assignment_for_testing();
}  // namespace detail

}  // namespace neuron::gpu