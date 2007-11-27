#ifndef nrnrt_h
#define nrnrt_h
#include "nrnrtuse.h"

#if NRN_REALTIME
#if defined(__cplusplus)
extern "C" {
#endif

void nrn_setscheduler();
void nrn_maintask_init();
void nrn_maintask_delete();
int nrn_rt_run(double tstop);
void nrn_rtrun_thread_init();

extern int nrn_realtime_;
extern double nrn_rtstep_time_;
extern int nrnrt_overrun_;

#if defined(__cplusplus)
}
#endif /*c++*/
#endif /* NRN_REALTIME */

#if NRN_6229
#define NRN_DAQ 1
#endif

#endif /*nrnmpi_h*/
