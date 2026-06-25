#pragma once

struct NrnThread;

namespace neuron {
struct model_sorted_token;
}

namespace neuron::gpu {

/** True when post-solve must run on the host (sparse13, extracellular). */
[[nodiscard]] bool post_solve_needs_host_fallback(NrnThread const& nt);

/** GPU second-order ion correction, voltage update, capacity current, and fast_imem. */
void post_solve_on_device(model_sorted_token const& sorted_token, NrnThread& nt);

}  // namespace neuron::gpu