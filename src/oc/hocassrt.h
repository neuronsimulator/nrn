
#ifndef hocassrt_h
#define hocassrt_h
#include <assert.h>
#undef assert
#undef _assert
#ifndef NDEBUG
#ifndef stderr
#include <stdio.h>
#endif

#include "oc_ansi.h"
#if defined(__STDC__)
#undef assert
#define assert(ex)                                                                       \
    {                                                                                    \
        if (!(ex)) {                                                                     \
            fprintf(stderr, "Assertion failed: file %s, line %d\n", __FILE__, __LINE__); \
            hoc_execerror(#ex, (char*) 0);                                               \
        }                                                                                \
    }
#else
#undef assert
#define assert(ex)                                                                       \
    {                                                                                    \
        if (!(ex)) {                                                                     \
            fprintf(stderr, "Assertion failed: file %s, line %d\n", __FILE__, __LINE__); \
            hoc_execerror("ex", (char*) 0);                                              \
        }                                                                                \
    }
#endif
#else
#define _assert(ex) ;
#define assert(ex)  ;
#endif
#endif
