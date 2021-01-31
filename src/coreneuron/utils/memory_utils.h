/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

/**
 * @file memory_utils.h
 * @date 25th Oct 2014
 * @brief Function prototypes for the functions providing
 * information about simulator memory usage
 *
 */

#ifndef NRN_MEMORY_UTILS
#define NRN_MEMORY_UTILS

namespace coreneuron {
/** @brief Reports current memory usage of the simulator to stdout
 *
 *  Current implementation is based on mallinfo. This routine prints
 *  min, max and avg memory usage across mpi comm world
 *  @param message string indicating current stage of the simulation
 *  @param all_ranks indicate whether to print info from all ranks
 *  @return Void
 */
void report_mem_usage(const char* message, bool all_ranks = false);

/** @brief Returns current memory usage in KBs
 *  @param Void
 *  @return memory usage in KBs
 */
double nrn_mallinfo(void);
}  // namespace coreneuron
#endif /* ifndef NRN_MEMORY_UTILS */
