#pragma once

namespace neuron {
struct model_sorted_token;
}

struct NrnThread;

namespace neuron::gpu {

class device_token;

/** Native GPU per-thread fixed-step body (fuses lastpart when no gap transfer). */
void fixed_step_thread(model_sorted_token const& cache_token,
                       device_token const& dev,
                       NrnThread& nt);

/** Register hoc helpers used by share/lib/python/neuron/gpu.py. */
void register_hoc_functions();

namespace detail {
[[nodiscard]] std::size_t fixed_step_dispatch_count_for_testing();
void reset_fixed_step_dispatch_for_testing();
}  // namespace detail

}  // namespace neuron::gpu
