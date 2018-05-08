/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
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
}
#endif /* ifndef NRN_MEMORY_UTILS */
