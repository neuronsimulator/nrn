#ifndef NRN_CORENRN_UTILS_H
#define NRN_CORENRN_UTILS_H

#include <string>
extern "C" {

void model_ready();
int count_distinct(double *data, int len);
extern void nrnbbcore_register_mapping();
bool file_exist(const std::string& path);


#if defined(HAVE_DLFCN_H)

bool is_coreneuron_loaded();
void* get_handle_for_lib(const char* path);
void* get_coreneuron_handle();
void check_coreneuron_compatibility(void* handle);

#endif

}

#endif //NRN_CORENRN_UTILS_H
