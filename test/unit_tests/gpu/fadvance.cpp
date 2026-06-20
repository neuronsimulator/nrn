#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/fadvance_gpu.hpp"
#include "neuron/gpu/net_events.hpp"

#include "multicore.h"
#include "neuron/cache/model_data.hpp"
#include "neuron/container/node_data.hpp"
#include "neuron/model_data.hpp"

#include <catch2/catch_test_macros.hpp>

#include <vector>

using namespace neuron::gpu;

// Standalone GPU tests do not link container.cpp.
namespace neuron::container::detail {
std::vector<void*>* defer_delete_storage{};
}

namespace {
struct DeferDeleteScope {
    std::vector<void*> ptrs{};
    DeferDeleteScope() {
        neuron::container::detail::defer_delete_storage = &ptrs;
    }
    ~DeferDeleteScope() {
        for (void* ptr: ptrs) {
            operator delete[](ptr);
        }
        neuron::container::detail::defer_delete_storage = nullptr;
    }
};
}  // namespace

TEST_CASE("native gpu config gate", "[gpu][fadvance]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    detail::reset_config_for_testing();
    CHECK_FALSE(enabled());
    CHECK_FALSE(backend_native());
    detail::set_enable_for_testing(true);
    CHECK(enabled());
    detail::set_backend_for_testing(Backend::Native);
    CHECK(backend_native());
    detail::set_backend_for_testing(Backend::Coreneuron);
    CHECK_FALSE(backend_native());
#endif
}

TEST_CASE("fixed_step_thread records native dispatch", "[gpu][fadvance]") {
#if !defined(NRN_ENABLE_GPU) || !defined(_OPENACC)
    SKIP("NRN_ENABLE_GPU and OpenACC required");
#else
    DeferDeleteScope defer_delete{};
    detail::reset_config_for_testing();
    detail::reset_fixed_step_dispatch_for_testing();
    neuron::gpu::detail::reset_net_events_for_testing();
    detail::set_enable_for_testing(true);
    detail::set_backend_for_testing(Backend::Native);

    neuron::cache::Model cache{};
    neuron::container::Node::storage nodes{};
    auto node_token = nodes.issue_frozen_token();
    neuron::model_sorted_token sorted{cache, std::move(node_token)};

    NrnThread nt{};
    nt.id = 0;
    nt.ncell = 0;
    nt._dt = 0.025;
    nt._t = 0.0;

    device_token dev{sorted};
    fixed_step_thread(sorted, dev, nt);
    REQUIRE(detail::fixed_step_dispatch_count_for_testing() == 1);
    REQUIRE(neuron::gpu::detail::deliver_net_events_count_for_testing() == 1);
    REQUIRE(nt.compute_gpu == 0);
#endif
}
