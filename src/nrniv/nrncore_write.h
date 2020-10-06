#ifndef nrncore_write_h
#define nrncore_write_h

#include "nrncore_write/data/cell_group.h"


extern void nrncore_netpar_cellgroups_helper(CellGroup*);

int nrncore_run(const char* arg);
int nrncore_is_enabled();
int nrncore_psolve(double tstop);


#endif // nrncore_write_h
