#pragma once
#include "nrnsemanticversion.h"
#define NRN_VERSION_INT(maj, min, pat)  (10000 * maj + 100 * min + pat)
#define NRN_VERSION                     NRN_VERSION_INT(NRN_VERSION_MAJOR, NRN_VERSION_MINOR, NRN_VERSION_PATCH)
#define NRN_VERSION_EQ(maj, min, pat)   (NRN_VERSION == NRN_VERSION_INT(maj, min, pat))
#define NRN_VERSION_NE(maj, min, pat)   (NRN_VERSION != NRN_VERSION_INT(maj, min, pat))
#define NRN_VERSION_GT(maj, min, pat)   (NRN_VERSION > NRN_VERSION_INT(maj, min, pat))
#define NRN_VERSION_LT(maj, min, pat)   (NRN_VERSION < NRN_VERSION_INT(maj, min, pat))
#define NRN_VERSION_GTEQ(maj, min, pat) (NRN_VERSION >= NRN_VERSION_INT(maj, min, pat))
#define NRN_VERSION_LTEQ(maj, min, pat) (NRN_VERSION <= NRN_VERSION_INT(maj, min, pat))

/* 8.2.0 is significant because all versions >=8.2.0 should contain definitions
 * of these macros, and doing #ifndef NRN_VERSION_GTEQ_8_2_0 is a more
 * descriptive way of writing #if defined(NRN_VERSION_GTEQ). Testing for 8.2.0
 * is likely to be a common pattern when adapting MOD file VERBATIM blocks for
 * C++ compatibility.
 */
#if NRN_VERSION_GTEQ(8, 2, 0)
#define NRN_VERSION_GTEQ_8_2_0
#endif

// Around NEURON 9.0.0 there are significant changes to data structures, which in some cases require
// that VERBATIM code is adapted.
#if NRN_VERSION_LT(9, 0, 0)
#error "Changes from NEURON 9+ may have been erroneously backported to an older branch"
#else
// Used to test if floating point mechanism data are accessed via a double* (_p) or via a Memb_list*
// and std::size_t pair (_ml and _iml)
#define NRN_MECHANISM_DATA_IS_SOA  // _ml and _iml are used
#endif
