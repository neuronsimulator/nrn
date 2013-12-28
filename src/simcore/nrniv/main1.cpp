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
  (void)env; /* unused */
  printf("enter main1 mallinfo %d\n", nrn_mallinfo());
  nrnmpi_init(1, &argc, &argv);
  printf("after nrnmpi_init mallinfo %d\n", nrn_mallinfo());
  mk_mech("bbcore_mech.dat");
  printf("after mk_mech mallinfo %d\n", nrn_mallinfo());
  mk_netcvode();

  // args are one gid in each group for the <gid>_[12].dat files.
  int ngrp = argc-1;
  int* grp;
  if (ngrp) {
    grp = new int[ngrp];
    for (int i = 0; i < ngrp; ++i) {
      grp[i] = atoi(argv[i+1]);
    }
  }else{
    ngrp = 1;
    grp = new int[ngrp];
    grp[0] = 0;
  }

  printf("before nrn_setup mallinfo %d\n", nrn_mallinfo());
  nrn_setup(ngrp, grp, ".");
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

const char* nrn_version(int) {
  return "version id unimplemented";
}
