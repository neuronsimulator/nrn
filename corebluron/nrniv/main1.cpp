#include <vector>

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

  /// Assigning threads to a specific task by the first gid written in the file
  FILE *fp = fopen("files.dat","r");
  int iNumTasks;
  assert(fscanf(fp, "%d\n", &iNumTasks) == 1);

  std::vector<int> VecThreads;
  int ngrp = 0;
  int* grp;

  /// For each MPI task written in bluron
  for (int iNum = 0; iNum < iNumTasks; ++iNum)
  {    
    int iNumThread, iTask, iFirstGid;
    assert(fscanf(fp, "%d\t%d\n", &iTask, &iNumThread) == 2);
    
    /// For each nrn_thread get the file name by the first gid number
    for (int iThread = 0; iThread < iNumThread; ++iThread)
    {
      assert(fscanf(fp, "%d\n", &iFirstGid) == 1);
      if ((iNum % nrnmpi_numprocs) == nrnmpi_myid)
      {
        VecThreads.push_back(iFirstGid);
      }
    }
  }

  ngrp = VecThreads.size();
  grp = new int[ngrp];
  for (int iInt = 0; iInt < ngrp; ++iInt)
  {
    grp[iInt] = VecThreads[iInt];
  }

  /// Reading the files and setting up the data structures
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

  /// Solver execution
//  prcellstate(6967, "t0");
  double time = nrnmpi_wtime();
  BBS_netpar_solve(1000);
  nrnmpi_barrier();

  if (nrnmpi_myid == 0)
    printf("Time to solution: %g\n", nrnmpi_wtime() - time);

//  prcellstate(6967, "t1");
  
  /// Outputting spikes
  output_spikes();

  nrnmpi_barrier();

  return 0;
}

const char* nrn_version(int) {
  return "version id unimplemented";
}
