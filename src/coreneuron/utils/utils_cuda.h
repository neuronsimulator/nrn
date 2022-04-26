/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include <stdio.h>
#include <cuda_runtime_api.h>

// From Random123 lib
#define CHECKLAST(MSG)                             \
    do {                                           \
        cudaError_t e = cudaGetLastError();        \
        if (e != cudaSuccess) {                    \
            fprintf(stderr,                        \
                    "%s:%d: CUDA Error: %s: %s\n", \
                    __FILE__,                      \
                    __LINE__,                      \
                    (MSG),                         \
                    cudaGetErrorString(e));        \
            exit(1);                               \
        }                                          \
    } while (0)
#define CHECKCALL(RET)                                                                             \
    do {                                                                                           \
        cudaError_t e = (RET);                                                                     \
        if (e != cudaSuccess) {                                                                    \
            fprintf(stderr, "%s:%d: CUDA Error: %s\n", __FILE__, __LINE__, cudaGetErrorString(e)); \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
