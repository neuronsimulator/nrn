#include "neuron/gpu/config.hpp"

#include "neuron/gpu/device_state.hpp"
#include "node_order_optim/node_order_optim.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <string>

extern int cvode_active_;
extern int nrn_nthread;

namespace neuron::gpu {
namespace {

struct RuntimeConfig {
    bool enable{false};
    Backend backend{Backend::Coreneuron};
    unsigned device_count{0};
};

RuntimeConfig& config() {
    static RuntimeConfig instance;
    return instance;
}

Backend parse_backend(std::string_view name) {
    std::string lowered{name};
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (lowered == "native") {
        return Backend::Native;
    }
    if (lowered == "coreneuron") {
        return Backend::Coreneuron;
    }
    throw std::invalid_argument("neuron::gpu::set_backend: expected 'native' or 'coreneuron'");
}

}  // namespace

bool enabled() noexcept {
#if defined(NRN_ENABLE_GPU)
    return config().enable;
#else
    return false;
#endif
}

bool backend_native() noexcept {
#if defined(NRN_ENABLE_GPU)
    return config().backend == Backend::Native;
#else
    return false;
#endif
}

Backend backend() noexcept {
    return config().backend;
}

void ensure_native_gpu_cell_permute() noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native()) {
        return;
    }
    if (neuron::interleave_permute_type == 2) {
        return;
    }
    int const previous = neuron::interleave_permute_type;
    neuron::nrn_optimize_node_order(2);
    if (previous != 2) {
        fprintf(stderr,
                "neuron::gpu: native GPU requires cell permute type 2 "
                "(interleave_permute_type was %d); using permute 2\n",
                previous);
        if (detail::is_on_device_for_testing()) {
            invalidate_device_state();
        }
    }
#else
#endif
}

void set_enable(bool value) noexcept {
#if defined(NRN_ENABLE_GPU)
    config().enable = value;
    if (value) {
        ensure_native_gpu_cell_permute();
    }
#else
    (void) value;
#endif
}

void set_backend(std::string_view name) {
#if defined(NRN_ENABLE_GPU)
    config().backend = parse_backend(name);
    ensure_native_gpu_cell_permute();
#else
    (void) name;
#endif
}

unsigned device_count() noexcept {
#if defined(NRN_ENABLE_GPU)
    return config().device_count;
#else
    return 0;
#endif
}

void set_device_count(unsigned value) noexcept {
#if defined(NRN_ENABLE_GPU)
    config().device_count = value;
#else
    (void) value;
#endif
}

bool use_cuda_launcher() noexcept {
#if defined(NRN_ENABLE_GPU)
    return enabled() && backend_native();
#else
    return false;
#endif
}

void warn_native_gpu_multithread_policy() noexcept {
#if defined(NRN_ENABLE_GPU)
    (void) nrn_nthread;
#endif
}

const char* native_gpu_configuration_error() noexcept {
#if defined(NRN_ENABLE_GPU)
    if (!enabled() || !backend_native()) {
        return nullptr;
    }
    if (cvode_active_) {
        return "native GPU backend (gpu.backend='native') requires fixed-step integration; "
               "deactivate CVode before enabling native GPU";
    }
#endif
    return nullptr;
}

namespace detail {

void reset_config_for_testing() {
    config() = RuntimeConfig{};
}

void set_enable_for_testing(bool value) {
    config().enable = value;
}

void set_backend_for_testing(Backend value) {
    config().backend = value;
}

void set_device_count_for_testing(unsigned value) {
    config().device_count = value;
}

}  // namespace detail

}  // namespace neuron::gpu
