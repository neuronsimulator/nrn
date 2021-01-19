/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef _H_NRN_ASSERT
#define _H_NRN_ASSERT

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

/* Preserving original behaviour requires that we abort() on
 * parse failures.
 *
 * Relying on assert() (as in the original code) is fragile,
 * as this becomes a NOP if the source is compiled with
 * NDEBUG defined.
 */

/** Emit formatted message to stderr, then abort(). */
static void abortf(const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    abort();
}

/** assert()-like macro, independent of NDEBUG status */
#define nrn_assert(x) \
    ((x) || (abortf("%s:%d: Assertion '%s' failed.\n", __FILE__, __LINE__, #x), 0))

#endif
