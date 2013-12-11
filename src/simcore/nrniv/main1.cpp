#include "simcore/nrnconf.h"
#include "simcore/nrnoc/multicore.h"
#include "simcore/nrnoc/nrnoc_decl.h"
#include "simcore/nrnmpi/nrnmpi.h"
#include "simcore/nrniv/nrniv_decl.h"
#include "simcore/nrniv/output_spikes.h"

#define HAVE_MALLINFO 1
#if HAVE_MALLINFO
#include <malloc.h>
int nrn_mallinfo() {
  struct mallinfo m = mallinfo();
  return m.hblkhd + m.uordblks;
  return 0;
}
#else
int nrn_mallinfo() { return 0; }
#endif

int main1(int argc, char** argv, char** env) {
  printf("enter main1 mallinfo %d\n", nrn_mallinfo());
  nrnmpi_init(1, &argc, &argv);
  mk_mech("bbcore_mech.dat");
  mk_netcvode();
  nrn_setup(1, "./");
  printf("after nrn_setup mallinfo %d\n", nrn_mallinfo());
  t = 0;
  dt = 0.025;
  double mindelay = BBS_netpar_mindelay(10.0);
  printf("mindelay = %g\n", mindelay);
  mk_spikevec_buffer(10000);
  nrn_finitialize(1, -65.0);
  printf("after finitialize mallinfo %d\n", nrn_mallinfo());
  BBS_netpar_solve(100.);
  output_spikes();
  return 0;
}

const char* nrn_version(int i) {
	return "version id unimplemented";
}
