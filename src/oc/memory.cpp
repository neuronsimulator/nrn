#include "memory.hpp"

#include <cstdlib>
#include <cstring>

// For hoc_warning and hoc_execerror
#include "oc_ansi.h"

#if HAVE_POSIX_MEMALIGN
#define HAVE_MEMALIGN 1
#endif
#if defined(DARWIN) /* posix_memalign seems not to work on Darwin 10.6.2 */
#undef HAVE_MEMALIGN
#endif
#if HAVE_MEMALIGN
#undef _XOPEN_SOURCE /* avoid warnings about redefining this */
#define _XOPEN_SOURCE 600
#endif

static bool emalloc_error = false;

void* hoc_Emalloc(std::size_t n) { /* check return from malloc */
    void* p = std::malloc(n);
    if (p == nullptr) {
        emalloc_error = true;
    }
    return p;
}

void* hoc_Ecalloc(std::size_t n, std::size_t size) { /* check return from calloc */
    if (n == 0) {
        return nullptr;
    }
    void* p = std::calloc(n, size);
    if (p == nullptr) {
        emalloc_error = true;
    }
    return p;
}

void* hoc_Erealloc(void* ptr, std::size_t size) { /* check return from realloc */
    if (!ptr) {
        return hoc_Emalloc(size);
    }
    void* p = std::realloc(ptr, size);
    if (p == nullptr) {
        std::free(ptr);
        emalloc_error = true;
    }
    return p;
}

void hoc_malchk(void) {
    if (emalloc_error) {
        emalloc_error = false;
        hoc_execerror("out of memory", nullptr);
    }
}

void* emalloc(std::size_t n) {
    void* p = hoc_Emalloc(n);
    if (emalloc_error) {
        hoc_malchk();
    }
    return p;
}

void* ecalloc(std::size_t n, std::size_t size) {
    void* p = hoc_Ecalloc(n, size);
    if (emalloc_error) {
        hoc_malchk();
    }
    return p;
}

void* erealloc(void* ptr, std::size_t size) {
    void* p = hoc_Erealloc(ptr, size);
    if (emalloc_error) {
        hoc_malchk();
    }
    return p;
}

void* nrn_cacheline_alloc(void** memptr, std::size_t size) {
#if HAVE_MEMALIGN
    static bool memalign_is_working = true;
    if (memalign_is_working) {
        if (posix_memalign(memptr, 64, size) != 0) {
            hoc_warning("posix_memalign not working, falling back to using malloc\n", nullptr);
            memalign_is_working = false;
            *memptr = hoc_Emalloc(size);
            hoc_malchk();
        }
    } else
#endif
    {
        *memptr = hoc_Emalloc(size);
        hoc_malchk();
    }
    return *memptr;
}

void* nrn_cacheline_calloc(void** memptr, std::size_t nmemb, std::size_t size) {
#if HAVE_MEMALIGN
    nrn_cacheline_alloc(memptr, nmemb * size);
    std::memset(*memptr, 0, nmemb * size);
#else
    *memptr = hoc_Ecalloc(nmemb, size);
    hoc_malchk();
#endif
    return *memptr;
}
