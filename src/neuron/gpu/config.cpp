#include "neuron/gpu/config.hpp"

namespace neuron::gpu {

bool enabled() noexcept {
#if defined(NRN_ENABLE_GPU)
    // Runtime gpu.enable / gpu.backend dispatch is added in PR 10.
    return false;
#else
    return false;
#endif
}

bool use_cuda_launcher() noexcept {
#if defined(NRN_ENABLE_GPU)
    // CUDA kernel expects CoreNEURON NrnThread arena pointers (PR 9 upload).
    return false;
#else
    return false;
#endif
}

}  // namespace neuron::gpu