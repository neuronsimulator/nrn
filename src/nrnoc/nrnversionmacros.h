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
