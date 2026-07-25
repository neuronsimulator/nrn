#include "neuron/gpu/net_events.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/net_receive_buffer.hpp"

#include "multicore.h"
#include "nrncvode.h"

#include <atomic>

namespace neuron::gpu {
namespace {

std::atomic<std::size_t> g_deliver_net_events_count{0};
std::atomic<std::size_t> g_deliver_post_step_events_count{0};
std::atomic<std::size_t> g_spike_exchange_count{0};

}  // namespace

namespace {

/** Stage 3: order + upload NetReceiveBuffer, then run registered net_buf_receive. */
void flush_net_receive_buffers(NrnThread* nt) {
    if (!nt || !nt->compute_gpu || net_buf_receive.empty()) {
        return;
    }
    update_net_receive_buffer(nt);
    for (auto const& entry: net_buf_receive) {
        if (entry.first) {
            (*entry.first)(nt);
        }
    }
}

}  // namespace

void deliver_net_events_host(NrnThread* nt) {
    ++g_deliver_net_events_count;
    // Start-of-step delivery (til = t + 0.5*dt) enqueues NET_RECEIVE when compute_gpu.
    // Must flush before setup_tree_matrix / nrn_cur so synaptic g is visible this step.
    deliver_net_events(nt);
    flush_net_receive_buffers(nt);
}

void deliver_post_step_events_host(NrnThread* nt) {
    ++g_deliver_post_step_events_count;
    // End-of-step delivery (til = t) enqueues; flush for next step and for end-of-run dumps.
    nrn_deliver_events(nt);
    flush_net_receive_buffers(nt);
}

void spike_exchange_after_group(NrnThread* nt) {
    if (!enabled() || !backend_native()) {
        return;
    }
#if NRNMPI
    ++g_spike_exchange_count;
    nrn_spike_exchange(nt);
#else
    (void) nt;
#endif
}

namespace detail {

std::size_t deliver_net_events_count_for_testing() {
    return g_deliver_net_events_count.load();
}

std::size_t deliver_post_step_events_count_for_testing() {
    return g_deliver_post_step_events_count.load();
}

std::size_t spike_exchange_count_for_testing() {
    return g_spike_exchange_count.load();
}

void reset_net_events_for_testing() {
    g_deliver_net_events_count.store(0);
    g_deliver_post_step_events_count.store(0);
    g_spike_exchange_count.store(0);
}

}  // namespace detail

}  // namespace neuron::gpu
