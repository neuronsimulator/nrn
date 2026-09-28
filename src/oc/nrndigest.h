
#pragma once

#include <nrnconf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NRN_DIGEST NRN_ENABLE_DIGEST

#if NRN_DIGEST
extern int nrn_digest_;  // debugging differences on different machines.
extern void nrn_digest_dbl_array(const char* msg, int tid, double t, double* array, size_t sz);
#endif

#ifdef __cplusplus
}
#endif
