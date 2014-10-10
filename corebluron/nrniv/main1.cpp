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
#include "corebluron/utils/endianness.h"
#include "corebluron/nrniv/nrnoptarg.h"
#include "corebluron/nrniv/nrn_assert.h"

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

int fatal_error(const char *msg) {
  if (nrnmpi_myid == 0) printf("\nERROR! %s\n\n",msg);
  nrnmpi_barrier();
  return -1;
}


int main1(int argc, char** argv, char** env) {
  (void)env; /* unused */

  nrnmpi_init(1, &argc, &argv);

  /// Getting essential inputs
  cb_input_params input_params;

  Get_cb_opts(argc, argv, &input_params);

  char prcellname[1024], datpath[1024], outpath[1024], filesdat[1024];

  /// Overall path for the .dat files
  if (!input_params.datpath) 
  {
    strcpy(datpath, ".");
  }
  else
  {
    strcpy(datpath, input_params.datpath);
  }

  /// Specific name for files.dat, if different
  if (!input_params.filesdat)
  {
    sprintf(filesdat, "%s/%s", datpath, "files.dat");
  }
  else
  {
    strcpy(filesdat, input_params.filesdat);
  }

  Print_MemUsage("after nrnmpi_init mallinfo");

  mk_mech(datpath);

  Print_MemUsage("after mk_mech mallinfo");

  mk_netcvode();

  t = input_params.tstart;
  dt = input_params.dt;
  celsius = input_params.celsius;
  if (nrnmpi_myid == 0)
  {
    printf("    t=%g tstop=%g dt=%g forwardskip=%g celsius=%g voltage=%g maxdelay=%g spikebuf=%d\n\
    datpath: %s\n    filesdat: %s\n", t, input_params.tstop, dt, input_params.forwardskip, celsius, input_params.voltage, input_params.maxdelay, input_params.spikebuf, datpath, filesdat);
    if (input_params.prcellgid >= 0)
      printf("    prcellstate will be called for gid %d\n", input_params.prcellgid);
    if (input_params.patternstim)
      printf("    patternstim is defined and will be read from the file: %s\n", input_params.patternstim);
  }

  // One part done before call to nrn_setup. Other part after.
  if (input_params.patternstim) {
    nrn_set_extra_thread0_vdata();
  }

  enum endian::endianness file_endian=endian::little_endian;
  if (endian::is_little_endian()) {
    if (nrn_need_byteswap){
      file_endian = endian::big_endian;
    }
  }else if (endian::is_big_endian()) {
    if (!nrn_need_byteswap){
      file_endian = endian::big_endian;
    }
  }else{
    return fatal_error("Could not figure out whether to byteswap.");
  }

  /// Assigning threads to a specific task by the first gid written in the file
  FILE *fp = fopen(filesdat,"r");
  if (!fp)
  {
    return fatal_error("No input file with nrnthreads, exiting...");
  }
  
  int iNumFiles = 0;
  nrn_assert(fscanf(fp, "%d\n", &iNumFiles)==1);
  if (nrnmpi_numprocs > iNumFiles)
  {
    return fatal_error("The number of CPUs cannot exceed the number of input files");
  }

  int ngrp = 0;
  int* grp = new int[iNumFiles / nrnmpi_numprocs + 1];

  /// For each file written in bluron
  for (int iNum = 0; iNum < iNumFiles; ++iNum)
  {    
    int iFile;
    nrn_assert(fscanf(fp, "%d\n", &iFile)==1);
    if ((iNum % nrnmpi_numprocs) == nrnmpi_myid)
    {
      grp[ngrp] = iFile;
      ngrp++;
    }
  }
  fclose(fp);

  /// Reading the files and setting up the data structures
  Print_MemUsage("before nrn_setup mallinfo");

  nrn_setup(ngrp, grp, datpath, file_endian, input_params.threading);

  Print_MemUsage("after nrn_setup mallinfo");

  delete [] grp;


  /// Invoke PatternStim
  if (input_params.patternstim) {
    nrn_mkPatternStim(input_params.patternstim);
  }

  double mindelay = BBS_netpar_mindelay(input_params.maxdelay);
  if (nrnmpi_myid == 0)
    printf("mindelay = %g\n", mindelay);

  Print_MemUsage("before spike buffer");

  mk_spikevec_buffer(input_params.spikebuf);

  Print_MemUsage("after spike buffer");

  nrn_finitialize(1, input_params.voltage);

  Print_MemUsage("after finitialize mallinfo");

  if (input_params.prcellgid >= 0) {
    sprintf(prcellname, "t%g", t);
    prcellstate(input_params.prcellgid, prcellname);
  }

  if (input_params.forwardskip > 0.0) {
    double savedt = dt;
    double savet = t;
    dt = input_params.forwardskip * 0.1;
    t = -1e9;
    for (int step=0; step < 10; ++step) {
      nrn_fixed_step();
    }
    if (input_params.prcellgid >= 0) {
      prcellstate(input_params.prcellgid, "fs");
    }
    dt = savedt;
    t = savet;
  }

  /// Solver execution
  double time = nrnmpi_wtime();
  BBS_netpar_solve(input_params.tstop);
  nrnmpi_barrier();

  if (input_params.prcellgid >= 0) {
    sprintf(prcellname, "t%g", t);
    prcellstate(input_params.prcellgid, prcellname);
  }

  if (nrnmpi_myid == 0)
    printf("Time to solution: %g\n", nrnmpi_wtime() - time);

  /// Outputting spikes
  if (!input_params.outpath)
    strcpy(outpath, ".");
  else
    strcpy(outpath, input_params.outpath);
  output_spikes(outpath);

  nrnmpi_barrier();

  nrnmpi_finalize();

  return 0;
}

const char* nrn_version(int) {
  return "version id unimplemented";
}
