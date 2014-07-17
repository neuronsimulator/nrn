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

int main1(int argc, char** argv, char** env) {
  (void)env; /* unused */
  printf("enter main1 mallinfo %d\n", nrn_mallinfo());
  nrnmpi_init(1, &argc, &argv);
  printf("after nrnmpi_init mallinfo %d\n", nrn_mallinfo());
  mk_mech("bbcore_mech.dat");
  printf("after mk_mech mallinfo %d\n", nrn_mallinfo());
  mk_netcvode();

  /// Reading essential inputs
  double tstop, maxdelay, voltage;
  int iSpikeBuf;
  char str[128];
  FILE *fp = fopen("inputs.dat","r");
  if (!fp)
  {
    if (nrnmpi_myid == 0)
      printf("\nWARNING! No input data, applying defaults...\n\n");

    t = 0.;
    tstop = 100.;
    dt = 0.025;
    celsius = 34.;
    voltage = -65.;
    maxdelay = 10.;
    iSpikeBuf = 400000;  
  }
  else
  {
    fgets(str, 128, fp);
    fscanf(fp, "  StartTime\t%lf\n", &t);
    fscanf(fp, "  EndTime\t%lf\n", &tstop);
    fscanf(fp, "  Dt\t\t%lf\n\n", &dt);
    fgets(str, 128, fp);
    fscanf(fp, "  Celcius\t%lf\n", &celsius);
    fscanf(fp, "  Voltage\t%lf\n", &voltage);
    fscanf(fp, "  MaxDelay\t%lf\n", &maxdelay);
    fscanf(fp, "  SpikeBuf\t%d\n", &iSpikeBuf);
    fclose(fp);
  }

  /// Assigning threads to a specific task by the first gid written in the file
  fp = fopen("files.dat","r");
  if (!fp)
  {
    if (nrnmpi_myid == 0)
      printf("\nERROR! No input file with nrnthreads, exiting...\n\n");

    nrnmpi_barrier();
    return -1;
  }
  
  int iNumFiles;
  assert(fscanf(fp, "%d\n", &iNumFiles) == 1);

  int ngrp = 0;
  int* grp = new int[iNumFiles / nrnmpi_numprocs + 1];

  /// For each file written in bluron
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
  fclose(fp);

  /// Reading the files and setting up the data structures
  printf("before nrn_setup mallinfo %d\n", nrn_mallinfo());
  nrn_setup(ngrp, grp, ".");
  printf("after nrn_setup mallinfo %d\n", nrn_mallinfo());

  delete [] grp;

  double mindelay = BBS_netpar_mindelay(maxdelay);
  printf("mindelay = %g\n", mindelay);
  mk_spikevec_buffer(iSpikeBuf);

  nrn_finitialize(1, voltage);
  printf("after finitialize mallinfo %d\n", nrn_mallinfo());

  /// Solver execution
  prcellstate(83519, "t0");

  double time = nrnmpi_wtime();
  BBS_netpar_solve(tstop);//17.015625);
  nrnmpi_barrier();

  if (nrnmpi_myid == 0)
    printf("Time to solution: %g\n", nrnmpi_wtime() - time);

  prcellstate(83519, "t1");
  
  /// Outputting spikes
  output_spikes();

  nrnmpi_barrier();

  return 0;
}

const char* nrn_version(int) {
  return "version id unimplemented";
}
