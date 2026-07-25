#include "neuron/gpu/net_events.hpp"

#include "neuron/gpu/config.hpp"

#include "nrncvode.h"

#include <atomic>

namespace neuron::gpu {
namespace {

std::atomic<std::size_t> g_deliver_net_events_count{0};
std::atomic<std::size_t> g_deliver_post_step_events_count{0};
std::atomic<std::size_t> g_spike_exchange_count{0};

}  // namespace

void deliver_net_events_host(NrnThread* nt) {
    ++g_deliver_net_events_count;
    deliver_net_events(nt);
}

void deliver_post_step_events_host(NrnThread* nt) {
    ++g_deliver_post_step_events_count;
    nrn_deliver_events(nt);
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
