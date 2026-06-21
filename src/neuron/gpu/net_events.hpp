#pragma once

#include <cstddef>

struct NrnThread;

namespace neuron::gpu {

/**
 * Deliver presynaptic spikes from the CPU priority queue to NET_RECEIVE blocks.
 * Cross-rank spikes arrive via MPI into per-rank queues on CPU; mechanism
 * NET_RECEIVE computation itself runs on GPU during the integration step.
 */
void deliver_net_events_host(NrnThread* nt);

/** Host-side POINT_PROCESS / NetCon event delivery after the integration half-step. */
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
