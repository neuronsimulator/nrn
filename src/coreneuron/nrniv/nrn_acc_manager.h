#ifndef _nrn_device_manager_
#define _nrn_device_manager_

#include "coreneuron/nrnoc/multicore.h"

void setup_nrnthreads_on_device(NrnThread *threads, int nthreads);
void update_nrnthreads_on_host(NrnThread *threads, int nthreads);
void update_nrnthreads_on_device(NrnThread *threads, int nthreads);
void modify_data_on_device(NrnThread *threads, int nthreads);
void dump_nt_to_file(char *filename, NrnThread *threads, int nthreads);
void finalize_data_on_device(NrnThread *, int nthreads);

#ifdef __cplusplus
extern "C" {
#endif

void update_matrix_from_gpu(NrnThread *_nt);
void update_matrix_to_gpu(NrnThread *_nt);
void update_net_receive_buffer(NrnThread *_nt);

#ifdef __cplusplus
}
#endif

#endif // _nrn_device_manager_
