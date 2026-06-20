#include <catch2/catch_test_macros.hpp>

#include "neuron/gpu/config.hpp"

TEST_CASE("gpu config defaults", "[gpu][config]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#endif
    CHECK_FALSE(neuron::gpu::enabled());
    CHECK_FALSE(neuron::gpu::use_cuda_launcher());
}