#ifndef _H_MEMORY_
#define _H_MEMORY_

#include <stdlib.h>
#include <string.h>
#include "coreneuron/nrniv/nrn_assert.h"

namespace coreneuron {
    /** Independent function to compute the needed chunkding,
        the chunk argument is the number of doubles the chunk is chunkded upon.
    */
    template<int chunk>
    inline int soa_padded_size(int cnt, int layout) {
        int imod = cnt % chunk;
        if (layout == 1) return cnt; 
        if (imod) {
          int idiv = cnt / chunk;
          return (idiv + 1) * chunk;
        }
        return cnt;
    }
    
    
    /** Check for the pointer alignment.
    */
    inline bool is_aligned(void* pointer, size_t alignment) {
        return (((uintptr_t)(const void *)(pointer)) % (alignment) == 0);
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
        if (n == 0) { return (void*)0; }
        nrn_assert(posix_memalign(&p, alignment, n*size) == 0);
        nrn_assert(is_aligned(p, alignment));
        memset(p, 1, n*size); // Avoid native division by zero (cyme...)
        return p;
    }
} //end name space

#endif
