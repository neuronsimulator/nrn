#include "neuron/gpu/device_state.hpp"

#include "neuron/cache/model_data.hpp"
#include "neuron/container/node_data.hpp"
#include "neuron/container/soa_container.hpp"
#include "neuron/model_data.hpp"
#include "nrnoc/multicore.h"

#include <catch2/catch_test_macros.hpp>

#include <vector>

using namespace neuron::gpu;

// Standalone GPU tests do not link container.cpp.
namespace neuron::container::detail {
std::vector<void*>* defer_delete_storage{};
}

namespace {
// Scope-local defer-delete storage so Node::storage teardown is safe.
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

TEST_CASE("model_sorted_token registers GPU layout refcount", "[gpu][device_state]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    DeferDeleteScope defer_delete{};
    neuron::cache::Model cache{};
    neuron::container::Node::storage nodes{};
    auto node_token = nodes.issue_frozen_token();
    auto const baseline = detail::sorted_token_count_for_testing();
    {
        neuron::model_sorted_token sorted{cache, std::move(node_token)};
        REQUIRE(detail::sorted_token_count_for_testing() == baseline + 1);
        neuron::model_sorted_token copy{sorted};
        REQUIRE(detail::sorted_token_count_for_testing() == baseline + 2);
    }
    REQUIRE(detail::sorted_token_count_for_testing() == baseline);
#endif
}

TEST_CASE("device_token upload stub and teardown", "[gpu][device_state]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    DeferDeleteScope defer_delete{};
    neuron::cache::Model cache{};
    neuron::container::Node::storage nodes{};
    auto node_token = nodes.issue_frozen_token();
    {
        neuron::model_sorted_token sorted{cache, std::move(node_token)};
        device_token token{sorted};
        REQUIRE(token.is_on_device());
        REQUIRE(detail::is_on_device_for_testing());

        device_token copy{token};
        REQUIRE(copy.is_on_device());
    }
    auto const baseline = detail::sorted_token_count_for_testing();
    REQUIRE(detail::sorted_token_count_for_testing() == baseline);
    REQUIRE_FALSE(detail::is_on_device_for_testing());
#endif
}

TEST_CASE("ensure_on_device shares upload across calls", "[gpu][device_state]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    DeferDeleteScope defer_delete{};
    neuron::cache::Model cache{};
    neuron::container::Node::storage nodes{};
    auto node_token = nodes.issue_frozen_token();
    neuron::model_sorted_token sorted{cache, std::move(node_token)};
    device_token const& first = ensure_on_device(sorted);
    device_token const& second = ensure_on_device(sorted);
    REQUIRE(&first == &second);
    REQUIRE(first.is_on_device());
#endif
}

TEST_CASE("invalidate_device_state clears GPU mirrors", "[gpu][device_state]") {
#if !defined(NRN_ENABLE_GPU)
    SKIP("NRN_ENABLE_GPU required");
#else
    DeferDeleteScope defer_delete{};
    neuron::cache::Model cache{};
    neuron::container::Node::storage nodes{};
    auto node_token = nodes.issue_frozen_token();
    neuron::model_sorted_token sorted{cache, std::move(node_token)};
    ensure_on_device(sorted);
    REQUIRE(detail::is_on_device_for_testing());
    invalidate_device_state();
    REQUIRE_FALSE(detail::is_on_device_for_testing());
#endif
}

TEST_CASE("NrnThread exposes compute_gpu and stream_id", "[gpu][device_state]") {
    NrnThread nt{};
    nt.compute_gpu = 1;
    nt.stream_id = 3;
    REQUIRE(nt.compute_gpu == 1);
    REQUIRE(nt.stream_id == 3);
}
