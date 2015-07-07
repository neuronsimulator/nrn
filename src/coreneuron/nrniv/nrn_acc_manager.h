#ifndef _nrn_device_manager_
#define _nrn_device_manager_

#include "coreneuron/nrnoc/multicore.h"

void setup_nrnthreads_on_device(NrnThread *threads, int nthreads);
void update_nrnthreads_on_host(NrnThread *threads, int nthreads);
void modify_data_on_device(NrnThread *threads, int nthreads);
void dump_nt_to_file(char *filename, NrnThread *threads, int nthreads);

#endif // _nrn_device_manager_
