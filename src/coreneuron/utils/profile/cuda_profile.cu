/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include "cuda_profiler_api.h"
#include <stdio.h>

void print_gpu_memory_usage() {
    size_t free_byte;
    size_t total_byte;

    cudaError_t cuda_status = cudaMemGetInfo(&free_byte, &total_byte);

    if (cudaSuccess != cuda_status) {
        printf("Error: cudaMemGetInfo fails, %s \n", cudaGetErrorString(cuda_status));
        exit(1);
    }

    double free_db = (double)free_byte;
    double total_db = (double)total_byte;
    double used_db = total_db - free_db;
    printf("\n  => GPU MEMORY USAGE (MB) : Used = %f, Free = %f MB, Total = %f",
           used_db / 1024.0 / 1024.0, free_db / 1024.0 / 1024.0, total_db / 1024.0 / 1024.0);
    fflush(stdout);
}

void start_cuda_profile() {
    cudaProfilerStart();
}

void stop_cuda_profile() {
    cudaProfilerStop();
}
