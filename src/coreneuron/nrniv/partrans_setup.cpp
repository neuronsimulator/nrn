#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrniv/partrans.h"

using namespace::nrn_partrans;

nrn_partrans::SetupInfo* nrn_partrans::setup_info_;

void nrn_partrans::gap_mpi_setup(int ngroup) {
  printf("%d gap_mpi_setup ngroup=%d\n", nrnmpi_myid, ngroup);
}

void nrn_partrans::gap_thread_setup(NrnThread& nt) {
  printf("%d gap_thread_setup tid=%d\n", nrnmpi_myid, nt.id);
}
