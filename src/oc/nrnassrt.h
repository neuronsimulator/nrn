#ifndef nrnassrt_h
#define nrnassrt_h

/* nrn_assert is not deactivated by -DNDEBUG. Use when the assert expression
has side effects which need to be executed regardles of NDEBUG.
*/


#include <stdio.h>
#include <stdlib.h>

#if defined(hocassrt_h) /* hoc_execerror form */
#include "oc_ansi.h"

#if defined(__STDC__)
#define nrn_assert(ex)                                                                   \
    {                                                                                    \
        if (!(ex)) {                                                                     \
            fprintf(stderr, "Assertion failed: file %s, line %d\n", __FILE__, __LINE__); \
            hoc_execerror(#ex, (char*) 0);                                               \
        }                                                                                \
    }
#else
#define nrn_assert(ex)                                                                   \
    {                                                                                    \
        if (!(ex)) {                                                                     \
            fprintf(stderr, "Assertion failed: file %s, line %d\n", __FILE__, __LINE__); \
            hoc_execerror("ex", (char*) 0);                                              \
        }                                                                                \
    }
#endif

#else /* abort form */

#if defined(__STDC__)
#define nrn_assert(ex)                                                                   \
    {                                                                                    \
        if (!(ex)) {                                                                     \
            fprintf(stderr, "Assertion failed: file %s, line %d\n", __FILE__, __LINE__); \
            abort();                                                                     \
        }                                                                                \
    }
#else
#define nrn_assert(ex)                                                                   \
    {                                                                                    \
        if (!(ex)) {                                                                     \
            fprintf(stderr, "Assertion failed: file %s, line %d\n", __FILE__, __LINE__); \
            abort();                                                                     \
        }                                                                                \
    }
#endif

#endif


#endif
