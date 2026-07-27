#include "neuron/gpu/config.hpp"
#include "neuron/gpu/trajectory.hpp"

#include <catch2/catch_test_macros.hpp>
#include <vector>

class PlayRecord;

// Stubs: unit test does not link full libnrniv (same pattern as other GPU unit tests).
std::vector<PlayRecord*>* nrn_fixed_record_list() {
    return nullptr;
}

using namespace neuron::gpu;

TEST_CASE("trajectory plan empty without fixed_record", "[gpu][trajectory]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    detail::reset_trajectory_plan_for_testing();
    detail::reset_config_for_testing();
    detail::set_enable_for_testing(true);
    detail::set_backend_for_testing(Backend::Native);

    trajectory_plan_rebuild();
    REQUIRE(trajectory_plan_valid());
    // No net_cvode / no records → complete empty plan.
    REQUIRE(trajectory_plan_complete());
    REQUIRE_FALSE(trajectory_plan_active());
    REQUIRE(trajectory_plan().channels.empty());
    REQUIRE(trajectory_default_chunk_size() == 50);

    trajectory_plan_invalidate();
    REQUIRE_FALSE(trajectory_plan_valid());
#endif
}

TEST_CASE("trajectory plan inactive when backend not native", "[gpu][trajectory]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    detail::reset_trajectory_plan_for_testing();
    detail::reset_config_for_testing();
    detail::set_enable_for_testing(true);
    detail::set_backend_for_testing(Backend::Coreneuron);

    trajectory_plan_rebuild();
    REQUIRE(trajectory_plan_valid());
    REQUIRE_FALSE(trajectory_plan_complete());
    REQUIRE_FALSE(trajectory_plan_active());
#endif
}
