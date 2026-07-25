#include "neuron/gpu/config.hpp"
#include "neuron/gpu/net_events.hpp"

#include "multicore.h"
#include "nrncvode.h"

#include <catch2/catch_test_macros.hpp>

using namespace neuron::gpu;

void deliver_net_events(NrnThread*) {}
void nrn_deliver_events(NrnThread*) {}
void nrn_spike_exchange(NrnThread*) {}

TEST_CASE("net_events host delivery wrappers", "[gpu][net_events]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    detail::reset_net_events_for_testing();
    NrnThread nt{};
    deliver_net_events_host(&nt);
    deliver_post_step_events_host(&nt);
    REQUIRE(detail::deliver_net_events_count_for_testing() == 1);
    REQUIRE(detail::deliver_post_step_events_count_for_testing() == 1);
#endif
}

TEST_CASE("spike_exchange_after_group is native-gated", "[gpu][net_events]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    detail::reset_net_events_for_testing();
    detail::reset_config_for_testing();
    NrnThread nt{};
    spike_exchange_after_group(&nt);
    REQUIRE(detail::spike_exchange_count_for_testing() == 0);

    detail::set_enable_for_testing(true);
    detail::set_backend_for_testing(Backend::Coreneuron);
    spike_exchange_after_group(&nt);
    REQUIRE(detail::spike_exchange_count_for_testing() == 0);

    detail::set_backend_for_testing(Backend::Native);
    spike_exchange_after_group(&nt);
#if NRNMPI
    REQUIRE(detail::spike_exchange_count_for_testing() == 1);
#else
    REQUIRE(detail::spike_exchange_count_for_testing() == 0);
#endif
#endif
}
