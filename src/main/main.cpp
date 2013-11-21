#include "simcore/nrnconf.h"
#include "simcore/nrnoc/multicore.h"
#include "simcore/nrnoc/nrnoc_decl.h"
#include "simcore/nrnmpi/nrnmpi.h"
#include "simcore/nrniv/nrniv_decl.h"
#include "simcore/nrniv/output_spikes.h"

int main(int argc, char** argv, char** env) {
  nrnmpi_init(1, &argc, &argv);
  mk_mech("bbcore_mech.dat");
  mk_netcvode();
  nrn_setup(1, "./");
  t = 0;
  dt = 0.025;
  double mindelay = BBS_netpar_mindelay(10.0);
  printf("mindelay = %g\n", mindelay);
  mk_spikevec_buffer(10000);
  nrn_finitialize(1, -65.0);
  BBS_netpar_solve(100.);
  output_spikes();
  return 0;
}

void modl_reg() {
	// not right place, but plays role of nrnivmodl constructed
	// mod_func.c.
}

const char* nrn_version(int i) {
	return "version id unimplemented";
}
