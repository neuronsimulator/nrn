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

#include <vector>
#include <string.h>
#include "coreneuron/engine.h"
#include "coreneuron/utils/randoms/nrnran123.h"
#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrniv/output_spikes.h"
#include "coreneuron/nrniv/nrn_checkpoint.h"
#include "coreneuron/utils/endianness.h"
#include "coreneuron/utils/memory_utils.h"
#include "coreneuron/nrniv/nrnoptarg.h"
#include "coreneuron/utils/sdprintf.h"
#include "coreneuron/nrniv/nrn_stats.h"
#include "coreneuron/utils/reports/nrnreport.h"
#include "coreneuron/nrniv/nrn_acc_manager.h"
#include "coreneuron/nrniv/profiler_interface.h"
#include "coreneuron/nrniv/partrans.h"
#include "coreneuron/nrniv/multisend.h"
#include "coreneuron/utils/file_utils.h"
#include <string.h>
#include <climits>

#if 0
#include <fenv.h>
#define NRN_FEEXCEPT (FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW)
int nrn_feenableexcept() {
  int result = -1;
  result = feenableexcept(NRN_FEEXCEPT);
  return result;
}
#endif
namespace coreneuron {
void call_prcellstate_for_prcellgid(int prcellgid, int compute_gpu, int is_init);
void nrn_init_and_load_data(int argc,
                            char* argv[],
                            bool is_mapping_needed = false,
                            bool nrnmpi_under_nrncontrol = true,
                            bool run_setup_cleanup = true) {
#if defined(NRN_FEEXCEPT)
    nrn_feenableexcept();
#endif

    stop_profile();

// mpi initialisation
#if NRNMPI
    nrnmpi_init(nrnmpi_under_nrncontrol ? 1 : 0, &argc, &argv);
#endif

    // memory footprint after mpi initialisation
    report_mem_usage("After MPI_Init");

    // initialise default coreneuron parameters
    initnrn();

    // create mutex for nrn123, protect instance_count_
    nrnran123_mutconstruct();

    // set global variables
    // precedence is: set by user, globals.dat, 34.0
    celsius = nrnopt_get_dbl("--celsius");

#if _OPENACC
    if (!nrnopt_get_flag("--gpu") && nrnopt_get_int("--cell-permute") == 2) {
        fprintf(
            stderr,
            "compiled with _OPENACC does not allow the combination of --cell-permute=2 and missing --gpu\n");
        exit(1);
    }
#endif

// if multi-threading enabled, make sure mpi library supports it
#if NRNMPI
    if (nrnopt_get_flag("--threading")) {
        nrnmpi_check_threading_support();
    }
#endif

    // full path of files.dat file
    std::string filesdat(nrnopt_get_str("--datpath") + "/" + nrnopt_get_str("--filesdat"));

    // reads mechanism information from bbcore_mech.dat
    mk_mech(nrnopt_get_str("--datpath").c_str());

    // read the global variable names and set their values from globals.dat
    set_globals(nrnopt_get_str("--datpath").c_str(), nrnopt_get_flag("--seed"), nrnopt_get_int("--seed"));

    report_mem_usage("After mk_mech");

    // set global variables for start time, timestep and temperature
    std::string restore_path = nrnopt_get_str("--restore");
    t = restore_time(restore_path.c_str());

    if (nrnopt_get_dbl("--dt") != -1000.) {  // command line arg highest precedence
        dt = nrnopt_get_dbl("--dt");
    } else if (dt == -1000.) {  // not on command line and no dt in globals.dat
        dt = 0.025;             // lowest precedence
    }
    nrnopt_modify_dbl("--dt", dt);

    rev_dt = (int)(1. / dt);

    if (nrnopt_get_dbl("--celsius") != -1000.) {  // command line arg highest precedence
        celsius = nrnopt_get_dbl("--celsius");
    } else if (celsius == -1000.) {  // not on command line and no celsius in globals.dat
        celsius = 34.0;              // lowest precedence
    }
    nrnopt_modify_dbl("--celsius", celsius);

    // create net_cvode instance
    mk_netcvode();

    // One part done before call to nrn_setup. Other part after.
    if (nrnopt_get_flag("--pattern")) {
        nrn_set_extra_thread0_vdata();
    }

    report_mem_usage("Before nrn_setup");

    // set if need to interleave cells
    use_interleave_permute = nrnopt_get_int("--cell-permute");
    cellorder_nwarp = nrnopt_get_int("--nwarp");
    use_solve_interleave = nrnopt_get_int("--cell-permute");
#if LAYOUT == 1
    // permuting not allowed for AoS
    use_interleave_permute = 0;
    use_solve_interleave = 0;
#endif

    // pass by flag so existing tests do not need a changed nrn_setup prototype.
    nrn_setup_multiple = nrnopt_get_int("--multiple");
    nrn_setup_extracon = nrnopt_get_int("--extracon");
    // multisend options
    use_multisend_ = nrnopt_get_flag("--multisend") ? 1 : 0;
    n_multisend_interval = nrnopt_get_int("--ms-subintervals");
    use_phase2_ = (nrnopt_get_int("--ms-phases") == 2) ? 1 : 0;

    // reading *.dat files and setting up the data structures, setting mindelay
    nrn_setup(filesdat.c_str(), is_mapping_needed, nrn_need_byteswap, run_setup_cleanup);

    // Allgather spike compression and  bin queuing.
    nrn_use_bin_queue_ = nrnopt_get_flag("--binqueue");
    int spkcompress = nrnopt_get_int("--spkcompress");
    nrnmpi_spike_compress(spkcompress, (spkcompress ? true : false), use_multisend_);

    report_mem_usage("After nrn_setup ");

    // Invoke PatternStim
    if (nrnopt_get_flag("--pattern")) {
        nrn_mkPatternStim(nrnopt_get_str("--pattern").c_str());
    }

    /// Setting the timeout
    nrn_set_timeout(200.);

    // show all configuration parameters for current run
    nrnopt_show();
    if (nrnmpi_myid == 0) {
        std::cout << " Start time (t) = " << t << std::endl << std::endl;
    }

    // allocate buffer for mpi communication
    mk_spikevec_buffer(nrnopt_get_int("--spikebuf"));

    report_mem_usage("After mk_spikevec_buffer");

    if (nrnopt_get_flag("-gpu")) {
        setup_nrnthreads_on_device(nrn_threads, nrn_nthread);
    }

    if (nrn_have_gaps) {
        nrn_partrans::gap_update_indices();
    }

    // call prcellstate for prcellgid
    call_prcellstate_for_prcellgid(nrnopt_get_int("--prcellgid"), nrnopt_get_flag("-gpu"), 1);
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
                sprintf(prcellname, "%s_gpu_t%f", prprefix, t);
        } else {
            if (is_init)
                strcpy(prcellname, "cpu_init");
            else
                sprintf(prcellname, "cpu_t%f", t);
        }
        update_nrnthreads_on_host(nrn_threads, nrn_nthread);
        prcellstate(prcellgid, prcellname);
    }
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
}  // namespace coreneuron

using namespace coreneuron;
extern "C" int solve_core(int argc, char** argv) {
#if NRNMPI
    nrnmpi_init(1, &argc, &argv);
#endif

    // read command line parameters and parameter config files
    nrnopt_parse(argc, (const char**)argv);
    std::vector<ReportConfiguration> configs;
    bool reports_needs_finalize = false;

    if (nrnopt_get_str("--report-conf").size()) {
        if (nrnopt_get_int("--multiple") > 1) {
            if (nrnmpi_myid == 0)
                printf("\n WARNING! : Can't enable reports with model duplications feature! \n");
        } else {
            configs = create_report_configurations(nrnopt_get_str("--report-conf").c_str(),
                                                   nrnopt_get_str("--outpath").c_str());
            reports_needs_finalize = configs.size();
        }
    }

    // initializationa and loading functions moved to separate
    nrn_init_and_load_data(argc, argv, configs.size() > 0);
    std::string checkpoint_path = nrnopt_get_str("--checkpoint");
    if (strlen(checkpoint_path.c_str())) {
        nrn_checkpoint_arg_exists = true;
    }
    std::string output_dir = nrnopt_get_str("--outpath");

    if (nrnmpi_myid == 0) {
        mkdir_p(output_dir.c_str());
    }
#if NRNMPI
    nrnmpi_barrier();
#endif
    bool compute_gpu = nrnopt_get_flag("-gpu");
    bool skip_mpi_finalize = nrnopt_get_flag("--skip-mpi-finalize");

// clang-format off
    #pragma acc data copyin(celsius, secondorder) if (compute_gpu)
    // clang-format on
    {
        double v = nrnopt_get_dbl("--voltage");

        // TODO : if some ranks are empty then restore will go in deadlock
        // phase (as some ranks won't have restored anything and hence return
        // false in checkpoint_initialize
        if (!checkpoint_initialize()) {
            nrn_finitialize(v != 1000., v);
        }

        report_mem_usage("After nrn_finitialize");
        double dt = nrnopt_get_dbl("--dt");
        double delay = nrnopt_get_dbl("--mindelay");
        double tstop = nrnopt_get_dbl("--tstop");

        if (tstop < t && nrnmpi_myid == 0) {
            printf("Error: Stop time (%lf) < Start time (%lf), restoring from checkpoint? \n",
                   tstop, t);
            abort();
        }

        // register all reports into reportinglib
        double min_report_dt = INT_MAX;
        for (int i = 0; i < configs.size(); i++) {
            register_report(dt, tstop, delay, configs[i]);
            if (configs[i].report_dt < min_report_dt) {
                min_report_dt = configs[i].report_dt;
            }
        }
        setup_report_engine(min_report_dt, delay);
        configs.clear();

        // call prcellstate for prcellgid
        call_prcellstate_for_prcellgid(nrnopt_get_int("--prcellgid"), compute_gpu, 0);

        // handle forwardskip
        if (nrnopt_get_dbl("--forwardskip") > 0.0) {
            handle_forward_skip(nrnopt_get_dbl("--forwardskip"), nrnopt_get_int("--prcellgid"));
        }

        start_profile();

        /// Solver execution
        BBS_netpar_solve(nrnopt_get_dbl("--tstop"));
        // Report global cell statistics
        report_cell_stats();

        // prcellstate after end of solver
        call_prcellstate_for_prcellgid(nrnopt_get_int("--prcellgid"), compute_gpu, 0);
    }

    // write spike information to outpath
    output_spikes(output_dir.c_str());

    write_checkpoint(nrn_threads, nrn_nthread, checkpoint_path.c_str(), nrn_need_byteswap);

    stop_profile();

    // must be done after checkpoint (to avoid deleting events)
    if (reports_needs_finalize) {
        finalize_report();
    }

    // Cleaning the memory
    nrn_cleanup();

    // tau needs to resume profile
    start_profile();

// mpi finalize
#if NRNMPI
    if (!skip_mpi_finalize) {
        nrnmpi_finalize();
    }
#endif

    finalize_data_on_device();

    return 0;
}
