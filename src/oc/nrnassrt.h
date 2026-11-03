#pragma once

/* nrn_assert is not deactivated by -DNDEBUG. Use when the assert expression
has side effects which need to be executed regardles of NDEBUG.
*/


#include <stdio.h>
#include <stdlib.h>

#if defined(hocassrt_h) /* hoc_execerror form */
#include "oc_ansi.h"

#define nrn_assert(ex)                                                                   \
    do {                                                                                 \
        if (!(ex)) {                                                                     \
            fprintf(stderr, "Assertion failed: file %s, line %d\n", __FILE__, __LINE__); \
            hoc_execerror(#ex, nullptr);                                                 \
        }                                                                                \
    } while (0)
#else /* abort form */
#define nrn_assert(ex)                                                                   \
    do {                                                                                 \
        if (!(ex)) {                                                                     \
            fprintf(stderr, "Assertion failed: file %s, line %d\n", __FILE__, __LINE__); \
            abort();                                                                     \
        }                                                                                \
    } while (0)
#endif
