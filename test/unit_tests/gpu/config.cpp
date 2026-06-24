#include <catch2/catch_test_macros.hpp>

#include "neuron/gpu/config.hpp"

namespace neuron {
int interleave_permute_type = 0;

int nrn_optimize_node_order(int type) {
    interleave_permute_type = type;
    return type;
}
}  // namespace neuron

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

TEST_CASE("native GPU requires cell permute type 2", "[gpu][config]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    neuron::gpu::detail::reset_config_for_testing();
    neuron::interleave_permute_type = 0;

    neuron::gpu::set_backend("native");
    CHECK(neuron::interleave_permute_type == 0);

    neuron::gpu::set_enable(true);
    CHECK(neuron::interleave_permute_type == 2);

    neuron::gpu::detail::reset_config_for_testing();
    neuron::interleave_permute_type = 1;
    neuron::gpu::set_enable(true);
    neuron::gpu::set_backend("native");
    CHECK(neuron::interleave_permute_type == 2);

    neuron::gpu::detail::reset_config_for_testing();
    neuron::interleave_permute_type = 2;
    neuron::gpu::set_enable(true);
    neuron::gpu::set_backend("native");
    CHECK(neuron::interleave_permute_type == 2);
#endif
}
