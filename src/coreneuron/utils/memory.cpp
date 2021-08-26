/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include "coreneuron/apps/corenrn_parameters.hpp"
#include "coreneuron/utils/memory.h"

#ifdef CORENEURON_ENABLE_GPU
#include <cuda_runtime_api.h>
#endif

#include <cassert>

namespace coreneuron {
bool unified_memory_enabled() {
#ifdef CORENEURON_ENABLE_GPU
    return corenrn_param.gpu;
#endif
    return false;
}

void* allocate_unified(std::size_t num_bytes) {
#ifdef CORENEURON_ENABLE_GPU
    // The build supports GPU execution, check if --gpu was passed to actually
    // enable it. We should not call CUDA APIs in GPU builds if --gpu was not passed.
    if (corenrn_param.gpu) {
        // Allocate managed/unified memory.
        void* ptr{nullptr};
        auto const code = cudaMallocManaged(&ptr, num_bytes);
        assert(code == cudaSuccess);
        return ptr;
    }
#endif
    // Either the build does not have GPU support or --gpu was not passed.
    // Allocate using standard operator new.
    // When we have C++17 support then propagate `alignment` here.
    return ::operator new(num_bytes);
}

void deallocate_unified(void* ptr, std::size_t num_bytes) {
    // See comments in allocate_unified to understand the different branches.
#ifdef CORENEURON_ENABLE_GPU
    if (corenrn_param.gpu) {
        // Deallocate managed/unified memory.
        auto const code = cudaFree(ptr);
        assert(code == cudaSuccess);
        return;
    }
#endif
    ::operator delete(ptr, num_bytes);
}
}  // namespace coreneuron
