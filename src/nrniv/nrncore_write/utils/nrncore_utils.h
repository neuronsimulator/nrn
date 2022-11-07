#pragma once

#include <string>

struct NrnThread;

void model_ready();
int count_distinct(double* data, int len);
extern void nrnbbcore_register_mapping();
extern "C" int nrn_dblpntr2nrncore(double* pd, NrnThread& nt, int& type, int& index);

#include "nrnwrap_dlfcn.h"
#if defined(HAVE_DLFCN_H)

bool is_coreneuron_loaded();
void* get_coreneuron_handle();
void check_coreneuron_compatibility(void* handle);

#endif
