#include "neuron/gpu/device_state.hpp"
#include "neuron/gpu/upload.hpp"
#include "neuron/gpu/offload.hpp"

#include "coreneuron/permute/cellorder.hpp"
#include "neuron/cache/model_data.hpp"
#include "neuron/container/node_data.hpp"
#include "neuron/model_data.hpp"

#include <catch2/catch_test_macros.hpp>

#include <vector>

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

// Standalone GPU tests do not link container.cpp.
namespace neuron::container::detail {
std::vector<void*>* defer_delete_storage{};
}

TEST_CASE("upload mirrors sorted node SOA vectors", "[gpu][upload]") {
#if !defined(NRN_ENABLE_GPU) || !defined(_OPENACC)
    SKIP("NRN_ENABLE_GPU with OpenACC required");
#else
    if (acc_get_num_devices(acc_device_nvidia) < 1) {
        SKIP("No NVIDIA GPU available");
    }
    acc_init(acc_device_nvidia);
    acc_set_device_num(0, acc_device_nvidia);

    DeferDeleteScope defer_delete{};
    neuron::cache::Model cache{};
    neuron::container::Node::storage nodes{};
    neuron::container::Node::owning_handle n0{nodes};
    neuron::container::Node::owning_handle n1{nodes};
    n0.v() = 10.0;
    n1.v() = 20.0;

    auto node_token = nodes.issue_frozen_token();
    neuron::model_sorted_token sorted{cache, std::move(node_token)};
    device_token token{sorted};
    REQUIRE(token.is_on_device());
    REQUIRE(detail::mirror_count_for_testing() > 0);
    REQUIRE(detail::is_present_for_testing(&n0.v()));
    REQUIRE(detail::is_present_for_testing(&n1.v()));
#endif
}

TEST_CASE("InterleaveInfo permute-2 upload patches device pointers", "[gpu][upload][interleave]") {
#if !defined(NRN_ENABLE_GPU) || !defined(_OPENACC)
    SKIP("NRN_ENABLE_GPU with OpenACC required");
#else
    if (acc_get_num_devices(acc_device_nvidia) < 1) {
        SKIP("No NVIDIA GPU available");
    }
    acc_init(acc_device_nvidia);
    acc_set_device_num(0, acc_device_nvidia);

    neuron::InterleaveInfo info{};
    info.nwarp = 1;
    info.nstride = 2;
    info.stride = new int[2]{1, 1};
    info.firstnode = new int[2]{0, 1};
    info.lastnode = new int[2]{0, 1};
    info.stridedispl = new int[2]{0, 2};
    info.cellsize = new int[1]{2};

    UploadState state{};
    detail::upload_interleave_info_for_testing(2, info, 0, state);
    REQUIRE(state.mirror_count() >= 6);
    REQUIRE(state.is_present(info.stride));
    REQUIRE(state.is_present(info.firstnode));
    REQUIRE(state.is_present(info.stridedispl));
    REQUIRE(state.is_present(&info));
    REQUIRE(nrn_target_deviceptr(&info) != nullptr);
    REQUIRE(nrn_target_deviceptr(info.stride) != nullptr);

    state.teardown();
    delete[] info.stride;
    delete[] info.firstnode;
    delete[] info.lastnode;
    delete[] info.stridedispl;
    delete[] info.cellsize;
    info.stride = nullptr;
    info.firstnode = nullptr;
    info.lastnode = nullptr;
    info.stridedispl = nullptr;
    info.cellsize = nullptr;
#endif
}