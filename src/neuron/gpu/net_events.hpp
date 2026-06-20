#pragma once

#include <cstddef>

struct NrnThread;

namespace neuron::gpu {

/** Host-side threshold delivery before mechanism GPU work (Phase B1). */
void deliver_net_events_host(NrnThread* nt);

/** Host-side event delivery after the integration half-step (Phase B1). */
void deliver_post_step_events_host(NrnThread* nt);

/** Host MPI spike exchange after a fixed-step group when native GPU is active. */
void spike_exchange_after_group(NrnThread* nt);

namespace detail {
[[nodiscard]] std::size_t deliver_net_events_count_for_testing();
[[nodiscard]] std::size_t deliver_post_step_events_count_for_testing();
[[nodiscard]] std::size_t spike_exchange_count_for_testing();
void reset_net_events_for_testing();
}  // namespace detail

}  // namespace neuron::gpu
