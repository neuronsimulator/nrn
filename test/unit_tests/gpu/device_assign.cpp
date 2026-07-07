#include <catch2/catch_test_macros.hpp>

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_assign.hpp"

TEST_CASE("gpu device_count config", "[gpu][device_assign]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    neuron::gpu::detail::reset_config_for_testing();
    neuron::gpu::detail::reset_device_assignment_for_testing();
    CHECK(neuron::gpu::device_count() == 0);
    neuron::gpu::set_device_count(2);
    CHECK(neuron::gpu::device_count() == 2);
#endif
}

TEST_CASE("gpu assign_device idempotent", "[gpu][device_assign]") {
#if !defined(NRN_ENABLE_GPU) || !defined(_OPENACC)
    SKIP("NRN_ENABLE_GPU and OpenACC required");
#else
    neuron::gpu::detail::reset_config_for_testing();
    neuron::gpu::detail::reset_device_assignment_for_testing();
    CHECK(neuron::gpu::assigned_device_id() == -1);
    neuron::gpu::assign_device();
    auto const first = neuron::gpu::assigned_device_id();
    REQUIRE(first >= 0);
    neuron::gpu::assign_device();
    CHECK(neuron::gpu::assigned_device_id() == first);
#endif
}