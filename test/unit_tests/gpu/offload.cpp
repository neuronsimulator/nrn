#include "neuron/gpu/offload.hpp"

#include <catch2/catch_test_macros.hpp>

#include <vector>

#if defined(NRN_ENABLE_GPU) && defined(_OPENACC)
#include <openacc.h>
#endif

using namespace neuron::gpu;

TEST_CASE("neuron::gpu offload copyin, deviceptr, delete", "[gpu][offload]") {
#if !defined(NRN_ENABLE_GPU) || !defined(_OPENACC)
    SKIP("NRN_ENABLE_GPU with OpenACC required");
#else
    if (acc_get_num_devices(acc_device_nvidia) < 1) {
        SKIP("No NVIDIA GPU available");
    }
    acc_init(acc_device_nvidia);
    acc_set_device_num(0, acc_device_nvidia);

    alignas(64) double host[4]{1.0, 2.0, 3.0, 4.0};
    constexpr std::size_t len = 4;

    double* d_ptr = nrn_target_copyin(host, len);
    REQUIRE(d_ptr != nullptr);

    double* resolved = nrn_target_deviceptr(host);
    REQUIRE(resolved == d_ptr);

    double* present = nrn_target_is_present(host);
    REQUIRE(present == d_ptr);

    host[0] = 99.0;
    nrn_target_update_on_device(host, len);

    nrn_target_delete(host, len);

    double* after_delete = nrn_target_is_present(host);
    REQUIRE(after_delete == nullptr);
#endif
}