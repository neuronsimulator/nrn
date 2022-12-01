/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

/**
 * @file memory_utils.cpp
 * @date 25th Oct 2014
 *
 * @brief Provides functionality to report current memory usage
 * of the simulator using interface provided by malloc.h
 *
 * Memory utilisation report is based on the use of mallinfo
 * interface defined in malloc.h. For 64 bit platform, this
 * is not portable and hence it will be replaced with new
 * glibc implementation of malloc_info.
 *
 * @see http://man7.org/linux/man-pages/man3/malloc_info.3.html
 */

#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include "coreneuron/utils/memory_utils.h"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/mpi/core/nrnmpi.hpp"
#include "coreneuron/apps/corenrn_parameters.hpp"

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>
#elif defined HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef CORENEURON_ENABLE_GPU
#include "cuda_profiler_api.h"
#endif

namespace coreneuron {
double nrn_mallinfo(void) {
    // -ve mem usage for non-supported platforms
    double mbs = -1.0;

// on os x returns the current resident set size (physical memory in use)
#if defined(__APPLE__) && defined(__MACH__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &info, &infoCount) !=
        KERN_SUCCESS)
        return (size_t) 0L; /* Can't access? */
    return info.resident_size / (1024.0 * 1024.0);
#elif defined(MINGW)
    mbs = -1;
#else
    std::ifstream file("/proc/self/statm");
    if (file.is_open()) {
        unsigned long long int data_size;
        file >> data_size >> data_size;
        file.close();
        mbs = (data_size * sysconf(_SC_PAGESIZE)) / (1024.0 * 1024.0);
    } else {
#if defined HAVE_MALLOC_H
// The mallinfo2() function was added in glibc 2.33
#if defined(__GLIBC__) && (__GLIBC__ >= 2 && __GLIBC_MINOR__ >= 33)
        struct mallinfo2 m = mallinfo2();
#else
        struct mallinfo m = mallinfo();
#endif
        mbs = (m.hblkhd + m.uordblks) / (1024.0 * 1024.0);
#endif
    }
#endif
    return mbs;
}

void report_mem_usage(const char* message, bool all_ranks) {
    double mem_max, mem_min, mem_avg;  // min, max, avg memory

    // current memory usage on this rank
    double cur_mem = nrn_mallinfo();

/* @todo: avoid three all reduce class */
#if NRNMPI
    if (corenrn_param.mpi_enable) {
        mem_avg = nrnmpi_dbl_allreduce(cur_mem, 1) / nrnmpi_numprocs;
        mem_max = nrnmpi_dbl_allreduce(cur_mem, 2);
        mem_min = nrnmpi_dbl_allreduce(cur_mem, 3);
    } else
#endif
    {
        mem_avg = mem_max = mem_min = cur_mem;
    }

    // all ranks prints information if all_ranks is true
    if (all_ranks) {
        printf(" Memory (MBs) (Rank : %2d) : %30s : Cur %.4lf, Max %.4lf, Min %.4lf, Avg %.4lf \n",
               nrnmpi_myid,
               message,
               cur_mem,
               mem_max,
               mem_min,
               mem_avg);
    } else if (nrnmpi_myid == 0) {
        printf(" Memory (MBs) : %25s : Max %.4lf, Min %.4lf, Avg %.4lf \n",
               message,
               mem_max,
               mem_min,
               mem_avg);
#ifdef CORENEURON_ENABLE_GPU
        if (corenrn_param.gpu) {
            size_t free_byte, total_byte;
            cudaError_t cuda_status = cudaMemGetInfo(&free_byte, &total_byte);
            if (cudaSuccess != cuda_status) {
                std::printf("cudaMemGetInfo failed: %s\n", cudaGetErrorString(cuda_status));
            }
            constexpr double MiB{1. / (1024. * 1024.)};
            std::printf(" GPU Memory (MiBs) : Used = %f, Free = %f, Total = %f\n",
                        (total_byte - free_byte) * MiB,
                        free_byte * MiB,
                        total_byte * MiB);
        }
#endif
    }
    fflush(stdout);
}
}  // namespace coreneuron
