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

#ifndef _H_MEMORY_
#define _H_MEMORY_

#include <stdlib.h>
#include <string.h>
#include "coreneuron/nrniv/nrn_assert.h"

namespace coreneuron {
    /** Independent function to compute the needed chunkding,
        the chunk argument is the number of doubles the chunk is chunkded upon.
    */
    template <int chunk>
    inline int soa_padded_size(int cnt, int layout) {
        int imod = cnt % chunk;
        if (layout == 1)
            return cnt;
        if (imod) {
            int idiv = cnt / chunk;
            return (idiv + 1) * chunk;
        }
        return cnt;
    }

    /** Check for the pointer alignment.
    */
    inline bool is_aligned(void* pointer, size_t alignment) {
        return (((uintptr_t)(const void*)(pointer)) % (alignment) == 0);
    }

    /** Allocate the aligned memory.
    */
    inline void* emalloc_align(size_t size, size_t alignment) {
        void* memptr;
        nrn_assert(posix_memalign(&memptr, alignment, size) == 0);
        nrn_assert(is_aligned(memptr, alignment));
        return memptr;
    }

    /** Allocate the aligned memory and set it to 1.
    */
    inline void* ecalloc_align(size_t n, size_t alignment, size_t size) {
        void* p;
        if (n == 0) {
            return (void*)0;
        }
        nrn_assert(posix_memalign(&p, alignment, n * size) == 0);
        nrn_assert(is_aligned(p, alignment));
        memset(p, 1, n * size);  // Avoid native division by zero (cyme...)
        return p;
    }
}  // end name space

#endif
