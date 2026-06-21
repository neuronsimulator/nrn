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

TEST_CASE("native gpu configuration error when disabled", "[gpu][config]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    neuron::gpu::detail::reset_config_for_testing();
    CHECK(neuron::gpu::native_gpu_configuration_error() == nullptr);

    neuron::gpu::detail::set_enable_for_testing(true);
    neuron::gpu::detail::set_backend_for_testing(neuron::gpu::Backend::Native);
    // cvode_active_ is owned by the full simulator; standalone config tests only
    // verify the gate is inactive when native GPU is not enabled.
    neuron::gpu::detail::set_enable_for_testing(false);
    CHECK(neuron::gpu::native_gpu_configuration_error() == nullptr);
#endif
}
