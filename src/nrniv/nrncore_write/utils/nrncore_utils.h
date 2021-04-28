#ifndef NRN_NRNCORE_UTILS_H
#define NRN_NRNCORE_UTILS_H

#include <string>

class NrnThread;

void model_ready();
int count_distinct(double *data, int len);
extern void nrnbbcore_register_mapping();
bool file_exist(const std::string& path);
extern "C" int nrn_dblpntr2nrncore(double* pd, NrnThread& nt, int& type, int& index);


#if defined(HAVE_DLFCN_H)

bool is_coreneuron_loaded();
void* get_handle_for_lib(const char* path);
void* get_coreneuron_handle();
void check_coreneuron_compatibility(void* handle);

#endif


#endif //NRN_NRNCORE_UTILS_H

