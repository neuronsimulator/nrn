#include <stdio.h>

#ifdef CUDA_PROFILING
#include "coreneuron/nrniv/cuda_profile.h"
#endif

#ifdef CRAYPAT
#include <pat_api.h>
#endif

#if defined(_OPENACC)
#include <openacc.h>
#endif


static int cray_acc_debug_orig = 0;
static int cray_acc_debug_zero = 0;
extern int nrnmpi_myid;

void start_profile() {

    if(nrnmpi_myid == 0)
        printf("\n ----- GPU PROFILING STARTED -----\n");

#ifdef CRAYPAT
    PAT_record(PAT_STATE_ON);
#endif

#if defined(_OPENACC) && defined(_CRAYC)
    cray_acc_set_debug_global_level(cray_acc_debug_orig);
#endif

#ifdef CUDA_PROFILING
    start_cuda_profile();
#endif

}

void stop_profile() {

    if(nrnmpi_myid == 0)
        printf("\n ----- GPU PROFILING STOPPED -----\n");

#ifdef CRAYPAT
    PAT_record(PAT_STATE_OFF);
#endif

#if defined(_OPENACC) && defined(_CRAYC)
    cray_acc_debug_orig = cray_acc_get_debug_global_level();
    cray_acc_set_debug_global_level(cray_acc_debug_zero);
#endif

#ifdef CUDA_PROFILING
    stop_cuda_profile();
#endif

}
