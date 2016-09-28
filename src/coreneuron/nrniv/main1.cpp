/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * @file main1.cpp
 * @date 26 Oct 2014
 * @brief File containing main driver routine for CoreNeuron
 */

#include "coreneuron/utils/randoms/nrnran123.h"
#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrniv/output_spikes.h"
#include "coreneuron/utils/endianness.h"
#include "coreneuron/utils/memory_utils.h"
#include "coreneuron/nrniv/nrnoptarg.h"
#include "coreneuron/utils/sdprintf.h"
#include "coreneuron/nrniv/nrn_stats.h"
#include "coreneuron/utils/reports/nrnreport.h"
#include "coreneuron/nrniv/nrn_acc_manager.h"
#include "coreneuron/nrniv/profiler_interface.h"
#include "coreneuron/nrniv/partrans.h"
#include <string.h>

#if 0
#include <fenv.h>
#define NRN_FEEXCEPT (FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW)
int nrn_feenableexcept() {
  int result = -1;
  result = feenableexcept(NRN_FEEXCEPT);
  return result;
}
#endif

int main1(int argc, char** argv, char** env);
void nrn_init_and_load_data(int argc, char** argv, cn_input_params& input_params);
void call_prcellstate_for_prcellgid(int prcellgid, int compute_gpu, int is_init);

void nrn_init_and_load_data(int argc, char** argv, cn_input_params& input_params) {
#if defined(NRN_FEEXCEPT)
    nrn_feenableexcept();
#endif

#ifdef ENABLE_SELECTIVE_PROFILING
    stop_profile();
#endif

    // mpi initialisation
    nrnmpi_init(1, &argc, &argv);

    // memory footprint after mpi initialisation
    report_mem_usage("After MPI_Init");

    // initialise default coreneuron parameters
    initnrn();

    // create mutex for nrn123, protect instance_count_
    nrnran123_mutconstruct();

    // read command line parameters
    input_params.read_cb_opts(argc, argv);

    // set global variables
    celsius = input_params.celsius;
    t = input_params.celsius;

#if _OPENACC
    if (!input_params.compute_gpu && input_params.cell_interleave_permute == 2) {
        fprintf(
            stderr,
            "compiled with _OPENACC does not allow the combination of --cell_permute=2 and missing --gpu\n");
        exit(1);
    }
#endif

    // if multi-threading enabled, make sure mpi library supports it
    if (input_params.threading) {
        nrnmpi_check_threading_support();
    }

    // full path of files.dat file
    char filesdat_buf[1024];
    sd_ptr filesdat = input_params.get_filesdat_path(filesdat_buf, sizeof(filesdat_buf));

    // reads mechanism information from bbcore_mech.dat
    mk_mech(input_params.datpath);

    // read the global variable names and set their values from globals.dat
    set_globals(input_params.datpath);

    report_mem_usage("After mk_mech");

    // set global variables for start time, timestep and temperature
    t = input_params.tstart;

    if (input_params.dt != -1000.) {  // command line arg highest precedence
        dt = input_params.dt;
    } else if (dt == -1000.) {  // not on command line and no celsius in globals.dat
        dt = 0.025;             // lowest precedence
    }

    input_params.dt = dt;  // for printing

    rev_dt = (int)(1. / dt);

    if (input_params.celsius != -1000.) {  // command line arg highest precedence
        celsius = input_params.celsius;
    } else if (celsius == -1000.) {  // not on command line and no celsius in globals.dat
        celsius = 34.0;              // lowest precedence
    }

    input_params.celsius = celsius;  // for printing

    // create net_cvode instance
    mk_netcvode();

    // One part done before call to nrn_setup. Other part after.
    if (input_params.patternstim) {
        nrn_set_extra_thread0_vdata();
    }

    report_mem_usage("Before nrn_setup");

    // set if need to interleave cells
    use_interleave_permute = input_params.cell_interleave_permute;
    cellorder_nwarp = input_params.nwarp;
    use_solve_interleave = input_params.cell_interleave_permute;

    // pass by flag so existing tests do not need a changed nrn_setup prototype.
    nrn_setup_multiple = input_params.multiple;
    nrn_setup_extracon = input_params.extracon;

    // reading *.dat files and setting up the data structures, setting mindelay
    nrn_setup(input_params, filesdat, nrn_need_byteswap);

    report_mem_usage("After nrn_setup ");

    // Invoke PatternStim
    if (input_params.patternstim) {
        nrn_mkPatternStim(input_params.patternstim);
    }

    /// Setting the timeout
    nrn_set_timeout(200.);

    // show all configuration parameters for current run
    input_params.show_cb_opts();

    // allocate buffer for mpi communication
    mk_spikevec_buffer(input_params.spikebuf);

    report_mem_usage("After mk_spikevec_buffer");

    if (input_params.compute_gpu) {
        setup_nrnthreads_on_device(nrn_threads, nrn_nthread);
    }

    if (nrn_have_gaps) {
        nrn_partrans::gap_update_indices();
    }

    // call prcellstate for prcellgid
    call_prcellstate_for_prcellgid(input_params.prcellgid, input_params.compute_gpu, 1);
}

void call_prcellstate_for_prcellgid(int prcellgid, int compute_gpu, int is_init) {
    char prcellname[1024];
#ifdef ENABLE_CUDA
    const char* prprefix = "cu";
#else
    const char* prprefix = "acc";
#endif

    if (prcellgid >= 0) {
        if (compute_gpu) {
            if (is_init)
                sprintf(prcellname, "%s_gpu_init", prprefix);
            else
                sprintf(prcellname, "%s_gpu_t%g", prprefix, t);
        } else {
            if (is_init)
                strcpy(prcellname, "cpu_init");
            else
                sprintf(prcellname, "cpu_t%g", t);
        }
        update_nrnthreads_on_host(nrn_threads, nrn_nthread);
        prcellstate(prcellgid, prcellname);
    }
}

int main1(int argc, char** argv, char** env) {
    (void)env; /* unused */

    // Initial data loading
    cn_input_params input_params;

    // initializationa and loading functions moved to separate
    nrn_init_and_load_data(argc, argv, input_params);

    #pragma acc data copyin(celsius, secondorder) if (input_params.compute_gpu)
    {
        nrn_finitialize(input_params.voltage != 1000., input_params.voltage);

        report_mem_usage("After nrn_finitialize");

#ifdef ENABLE_REPORTING
        ReportGenerator* r = NULL;
#endif

        // if reports are enabled using ReportingLib
        if (input_params.report) {
#ifdef ENABLE_REPORTING
            if (input_params.multiple > 1) {
                if (nrnmpi_myid == 0)
                    printf(
                        "\n WARNING! : Can't enable reports with model duplications feature! \n");
            } else {
                r = new ReportGenerator(input_params.report, input_params.tstart,
                                        input_params.tstop, input_params.dt, input_params.mindelay,
                                        input_params.dt_report, input_params.outpath);
                r->register_report();
            }
#else
            if (nrnmpi_myid == 0)
                printf("\n WARNING! : Can't enable reports, recompile with ReportingLib! \n");
#endif
        }

        // call prcellstate for prcellgid
        call_prcellstate_for_prcellgid(input_params.prcellgid, input_params.compute_gpu, 0);

        // handle forwardskip
        if (input_params.forwardskip > 0.0) {
            handle_forward_skip(input_params.forwardskip, input_params.prcellgid);
        }

#ifdef ENABLE_SELECTIVE_PROFILING
        start_profile();
#endif

        /// Solver execution
        BBS_netpar_solve(input_params.tstop);

        // Report global cell statistics
        report_cell_stats();

#ifdef ENABLE_SELECTIVE_PROFILING
        stop_profile();
#endif

        // prcellstate after end of solver
        call_prcellstate_for_prcellgid(input_params.prcellgid, input_params.compute_gpu, 0);

#ifdef ENABLE_REPORTING
        if (input_params.report && r)
            delete r;
#endif
    }

    // write spike information to input_params.outpath
    output_spikes(input_params.outpath);

    // Cleaning the memory
    nrn_cleanup();

    // mpi finalize
    nrnmpi_finalize();

    finalize_data_on_device();

    return 0;
}

/* perform forwardskip and call prcellstate for prcellgid */
void handle_forward_skip(double forwardskip, int prcellgid) {
    double savedt = dt;
    double savet = t;

    dt = forwardskip * 0.1;
    t = -1e9;

    for (int step = 0; step < 10; ++step) {
        nrn_fixed_step_minimal();
    }

    if (prcellgid >= 0) {
        prcellstate(prcellgid, "fs");
    }

    dt = savedt;
    t = savet;
    dt2thread(-1.);
}

const char* nrn_version(int) {
    return "version id unimplemented";
}
