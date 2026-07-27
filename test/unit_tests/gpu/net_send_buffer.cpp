#include "neuron/gpu/config.hpp"
#include "neuron/gpu/net_send_buffer.hpp"
#include "neuron/gpu/offload.hpp"

#include "multicore.h"
#include "nrnoc_ml.h"

#include <catch2/catch_test_macros.hpp>

using namespace neuron::gpu;

TEST_CASE("NetSendBuffer capacity headroom and ensure_for_events", "[gpu][net_send]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    REQUIRE(net_send_buffer_sends_per_event_headroom() >= 1);

    // Heap Memb_list without delete of nested buffers via ~Memb_list (OpenACC TU).
    auto* const ml = new Memb_list{};
    ml->nodecount = 10;

    net_send_buffer_ensure(ml);
    REQUIRE(ml->_net_send_buffer != nullptr);
    int const base = ml->_net_send_buffer->_size;
    REQUIRE(base >= net_send_buffer_capacity(ml));
    REQUIRE(base >= 10 * net_send_buffer_sends_per_event_headroom());

    // Heavy self-event flush: min_events large → capacity ≥ min_events × headroom.
    int const min_events = 500;
    net_send_buffer_ensure_for_events(ml, min_events);
    REQUIRE(ml->_net_send_buffer->_size >=
            min_events * net_send_buffer_sends_per_event_headroom());

    // Adaptive high-water: record_peak grows when tight.
    auto* const nsb = ml->_net_send_buffer;
    int const peak = nsb->_size / 2 + 1;
    nsb->record_peak(peak);
    REQUIRE(nsb->_high_water >= peak);
    REQUIRE(nsb->_size >= peak * 2);

    // Intentionally leak Memb_list / NetSendBuffer: OpenACC-instrumented free of
    // never-uploaded buffers is unsafe in this TU (same pattern as net_receive tests).
    (void) ml;
#endif
}

TEST_CASE("NetSendBuffer reserve is monotonic", "[gpu][net_send]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    NetSendBuffer_t nsb(16);
    REQUIRE(nsb._size == 16);
    nsb.reserve(16);
    REQUIRE(nsb._size == 16);
    nsb.reserve(64);
    REQUIRE(nsb._size == 64);
    REQUIRE(nsb.reallocated == 1);
    nsb.reallocated = 0;
    nsb.reserve(32);  // no shrink
    REQUIRE(nsb._size == 64);
    REQUIRE(nsb.reallocated == 0);
#endif
}
