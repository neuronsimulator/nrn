#include <stdio.h>
#include "coreneuron/nrnmpi/nrnmpi.h"
#ifdef CUDA_PROFILING
#include "coreneuron/nrniv/cuda_profile.h"
#endif

#ifdef CRAYPAT
#include <pat_api.h>
#endif

#ifdef TAU
#include <TAU.h>
#endif

#if defined(_OPENACC)
#include <openacc.h>
namespace coreneuron {

static int cray_acc_debug_orig = 0;
static int cray_acc_debug_zero = 0;
} //namespace coreneuron
#endif
namespace coreneuron {


void start_profile() {
    if (nrnmpi_myid == 0)
        printf("\n ----- PROFILING STARTED -----\n");

#ifdef CRAYPAT
    PAT_record(PAT_STATE_ON);
#endif

#if defined(_OPENACC) && defined(_CRAYC)
    cray_acc_set_debug_global_level(cray_acc_debug_orig);
#endif

#ifdef CUDA_PROFILING
    start_cuda_profile();
#endif

#ifdef TAU
    TAU_ENABLE_INSTRUMENTATION();
#endif
}

void stop_profile() {
    if (nrnmpi_myid == 0)
        printf("\n ----- PROFILING STOPPED -----\n");

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

#ifdef TAU
    TAU_DISABLE_INSTRUMENTATION();
#endif
}
}//namespace coreneuron
