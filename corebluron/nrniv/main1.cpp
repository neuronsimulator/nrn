#include "corebluron/nrnconf.h"
#include "corebluron/nrnoc/multicore.h"
#include "corebluron/nrnoc/nrnoc_decl.h"
#include "corebluron/nrnmpi/nrnmpi.h"
#include "corebluron/nrniv/nrniv_decl.h"
#include "corebluron/nrniv/output_spikes.h"

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

extern void prcellstate(int gid, const char* suffix);

int main1(int argc, char** argv, char** env) {
  (void)env; /* unused */
  printf("enter main1 mallinfo %d\n", nrn_mallinfo());
  nrnmpi_init(1, &argc, &argv);
  printf("after nrnmpi_init mallinfo %d\n", nrn_mallinfo());
  mk_mech("bbcore_mech.dat");
  printf("after mk_mech mallinfo %d\n", nrn_mallinfo());
  mk_netcvode();

  FILE *fp = fopen("files.txt","r");
  int iNumFiles;
  assert(fscanf(fp, "%d\n", &iNumFiles) == 1);

  int ngrp = 0;
  int* grp = new int[iNumFiles / nrnmpi_numprocs + 1];

  for (int iNum = 0; iNum < iNumFiles; ++iNum)
  {    
    int iFile;
    assert(fscanf(fp, "%d\n", &iFile) == 1);
    if ((iNum % nrnmpi_numprocs) == nrnmpi_myid)
    {
      grp[ngrp] = iFile;
      ngrp++;
    }
  }

  printf("before nrn_setup mallinfo %d\n", nrn_mallinfo());
  nrn_setup(ngrp, grp, ".");
  printf("after nrn_setup mallinfo %d\n", nrn_mallinfo());

  t = 0;
  dt = 0.025;
  celsius = 34;
  double mindelay = BBS_netpar_mindelay(10.0);
  printf("mindelay = %g\n", mindelay);
  mk_spikevec_buffer(10000);

  nrn_finitialize(1, -65.0);
  printf("after finitialize mallinfo %d\n", nrn_mallinfo());

  prcellstate(14833, "t0");
  BBS_netpar_solve(100);
  prcellstate(14833, "t1");
  
  output_spikes();

  nrnmpi_barrier();

  return 0;
}

const char* nrn_version(int) {
  return "version id unimplemented";
}
