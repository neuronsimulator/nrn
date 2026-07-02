#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/download.hpp"
#include "neuron/gpu/offload.hpp"

#include "neuron/cache/model_data.hpp"
#include "neuron/container/node_data.hpp"
#include "neuron/model_data.hpp"

#include <catch2/catch_test_macros.hpp>

#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
#include <openacc.h>
#endif

using namespace neuron::gpu;

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

namespace neuron::container::detail {
std::vector<void*>* defer_delete_storage{};
}

TEST_CASE("sync_state_to_host_for_host_reads pulls SOA voltages", "[gpu][download]") {
#if !defined(NRN_ENABLE_GPU) || !defined(_OPENACC)
    SKIP("NRN_ENABLE_GPU with OpenACC required");
#else
    if (acc_get_num_devices(acc_device_nvidia) < 1) {
        SKIP("No NVIDIA GPU available");
    }
    acc_init(acc_device_nvidia);
    acc_set_device_num(0, acc_device_nvidia);

    detail::reset_config_for_testing();
    detail::set_enable_for_testing(true);
    detail::set_backend_for_testing(Backend::Native);

    DeferDeleteScope defer_delete{};
    neuron::cache::Model cache{};
    neuron::container::Node::storage& nodes = neuron::model().node_data();
    neuron::container::Node::owning_handle n0{nodes};
    neuron::container::Node::owning_handle n1{nodes};
    n0.v() = 10.0;
    n1.v() = 20.0;

    auto node_token = nodes.issue_frozen_token();
    neuron::model_sorted_token sorted{cache, std::move(node_token)};
    device_token token{sorted};
    REQUIRE(token.is_on_device());
    REQUIRE(detail::is_present_for_testing(&n0.v()));
    REQUIRE(detail::is_present_for_testing(&n1.v()));

    // Push new values to the device mirror, then corrupt host copies.
    n0.v() = 110.0;
    n1.v() = 120.0;
    nrn_target_update_on_device(&n0.v(), 1);
    nrn_target_update_on_device(&n1.v(), 1);

    n0.v() = 10.0;
    n1.v() = 20.0;

    sync_state_to_host_for_host_reads();

    REQUIRE(n0.v() == 110.0);
    REQUIRE(n1.v() == 120.0);
#endif
}