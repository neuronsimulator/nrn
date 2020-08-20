#ifndef nrnbbcore_write_h
#define nrnbbcore_write_h

#include "nrnbbcore_write/data/cell_group.h"

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

// Mechanism type to be used from stdindex2ptr (in CoreNeuron) and nrn_dblpntr2nrncore.
// Values of the mechanism types should be negative numbers to avoid any conflict with
// mechanism types of Memb_list(>0) or time(0) passed to CoreNeuron
enum mech_type {voltage = -1, i_membrane_ = -2};

#endif
