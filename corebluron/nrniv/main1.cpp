/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string.h>

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

void Print_MemUsage(const char *name)
{
  long long int input, max, min, avg;
  input = nrn_mallinfo();
  avg = nrnmpi_longlong_allreduce(input, 1);
  avg = (long long int)(avg / nrnmpi_numprocs);
  max = nrnmpi_longlong_allreduce(input, 2);
  min = nrnmpi_longlong_allreduce(input, 3);

  if (nrnmpi_myid == 0)
    printf("%s: max %lld, min %lld, avg %lld\n", name, max, min, avg);
}


int main1(int argc, char** argv, char** env) {
  (void)env; /* unused */

  nrnmpi_init(1, &argc, &argv);

  Print_MemUsage("after nrnmpi_init mallinfo");

  mk_mech("bbcore_mech.dat");

  Print_MemUsage("after mk_mech mallinfo");

  mk_netcvode();

  /// PatterStim option
  int need_patternstim = 0;

  if (argc > 1 && strcmp(argv[1], "-pattern") == 0) {
    // One part done before call to nrn_setup. Other part after.
    need_patternstim = 1;
  }

  if (need_patternstim) {
    nrn_set_extra_thread0_vdata();
  }

  /// Reading essential inputs
  double tstop, maxdelay, voltage;
  int iSpikeBuf;
  char *str = new char[128];
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
    str = fgets(str, 128, fp);
    assert(fscanf(fp, "  StartTime\t%lf\n", &t) == 1);
    assert(fscanf(fp, "  EndTime\t%lf\n", &tstop) == 1);
    assert(fscanf(fp, "  Dt\t\t%lf\n\n", &dt) == 1);
    str = fgets(str, 128, fp);
    assert(fscanf(fp, "  Celsius\t%lf\n", &celsius) == 1);
    assert(fscanf(fp, "  Voltage\t%lf\n", &voltage) == 1);
    assert(fscanf(fp, "  MaxDelay\t%lf\n", &maxdelay) == 1);
    assert(fscanf(fp, "  SpikeBuf\t%d\n", &iSpikeBuf) == 1);
    fclose(fp);
  }
  delete [] str;

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
  if (nrnmpi_numprocs > iNumFiles)
  {
    printf("\nERROR! The number of CPUs cannot exceed the number of input files\n\n");
    return -1;
  }

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
  Print_MemUsage("before nrn_setup mallinfo");

  nrn_setup(ngrp, grp, ".");

  Print_MemUsage("after nrn_setup mallinfo");

  delete [] grp;


  /// Invoke PatternStim
  if (need_patternstim) {
    nrn_mkPatternStim("out.std");
  }


  double mindelay = BBS_netpar_mindelay(maxdelay);
  if (nrnmpi_myid == 0)
    printf("mindelay = %g\n", mindelay);

  Print_MemUsage("before spike buffer");

  mk_spikevec_buffer(iSpikeBuf);

  Print_MemUsage("after spike buffer");

  nrn_finitialize(1, voltage);

  Print_MemUsage("after finitialize mallinfo");

  /// Solver execution
  double time = nrnmpi_wtime();
  BBS_netpar_solve(tstop);
  nrnmpi_barrier();

  if (nrnmpi_myid == 0)
    printf("Time to solution: %g\n", nrnmpi_wtime() - time);

  /// Outputting spikes
  output_spikes();

  nrnmpi_barrier();

  nrnmpi_finalize();

  return 0;
}

const char* nrn_version(int) {
  return "version id unimplemented";
}
