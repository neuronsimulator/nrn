#include "neuron/gpu/config.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace neuron::gpu {
namespace {

struct RuntimeConfig {
    bool enable{false};
    Backend backend{Backend::Coreneuron};
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

void set_enable(bool value) noexcept {
#if defined(NRN_ENABLE_GPU)
    config().enable = value;
#else
    (void) value;
#endif
}

void set_backend(std::string_view name) {
#if defined(NRN_ENABLE_GPU)
    config().backend = parse_backend(name);
#else
    (void) name;
#endif
}

bool use_cuda_launcher() noexcept {
#if defined(NRN_ENABLE_GPU)
    return enabled() && backend_native();
#else
    return false;
#endif
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

}  // namespace detail

}  // namespace neuron::gpu
