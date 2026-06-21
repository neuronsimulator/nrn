#include <catch2/catch_test_macros.hpp>

#include "neuron/gpu/config.hpp"

TEST_CASE("gpu config defaults", "[gpu][config]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    neuron::gpu::detail::reset_config_for_testing();
    CHECK_FALSE(neuron::gpu::enabled());
    CHECK_FALSE(neuron::gpu::backend_native());
    CHECK(neuron::gpu::backend() == neuron::gpu::Backend::Coreneuron);
    CHECK_FALSE(neuron::gpu::use_cuda_launcher());
    CHECK(neuron::gpu::device_count() == 0);
#endif
}
