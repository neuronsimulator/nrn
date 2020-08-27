#ifndef nrncore_write_h
#define nrncore_write_h

#include "nrncore_write/data/cell_group.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern void nrncore_netpar_cellgroups_helper(CellGroup*);

int nrncore_run(const char* arg);
int nrncore_is_enabled();
int nrncore_psolve(double tstop);

#if defined(__cplusplus)
}
#endif

#endif // nrncore_write_h
