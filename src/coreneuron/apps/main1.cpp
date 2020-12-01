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

#include <cstring>
#include <climits>
#include <memory>
#include <vector>

#include "coreneuron/config/config.h"
#include "coreneuron/engine.h"
#include "coreneuron/utils/randoms/nrnran123.h"
#include "coreneuron/nrnconf.h"
#include "coreneuron/sim/fast_imem.hpp"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/mechanism/register_mech.hpp"
#include "coreneuron/io/output_spikes.hpp"
#include "coreneuron/io/nrn_checkpoint.hpp"
#include "coreneuron/utils/memory_utils.h"
#include "coreneuron/apps/corenrn_parameters.hpp"
#include "coreneuron/io/prcellstate.hpp"
#include "coreneuron/utils/nrnmutdec.h"
#include "coreneuron/utils/nrn_stats.h"
#include "coreneuron/io/reports/nrnreport.hpp"
#include "coreneuron/io/reports/binary_report_handler.hpp"
#include "coreneuron/io/reports/report_handler.hpp"
#include "coreneuron/io/reports/sonata_report_handler.hpp"
#include "coreneuron/gpu/nrn_acc_manager.hpp"
#include "coreneuron/utils/profile/profiler_interface.h"
#include "coreneuron/network/partrans.hpp"
#include "coreneuron/network/multisend.hpp"
#include "coreneuron/io/file_utils.hpp"
#include "coreneuron/io/nrn2core_direct.h"
#include "coreneuron/io/core2nrn_data_return.hpp"

extern "C" {
const char* corenrn_version() {
    return coreneuron::bbcore_write_version;
}

// the CORENRN_USE_LEGACY_UNITS determined by CORENRN_ENABLE_LEGACY_UNITS
bool corenrn_units_use_legacy() {
    return CORENRN_USE_LEGACY_UNITS;
}

void (*nrn2core_part2_clean_)();

#ifdef ISPC_INTEROP
// cf. utils/ispc_globals.c
extern double ispc_celsius;
#endif

/**
 * If "export OMP_NUM_THREADS=n" is not set then omp by default sets
 * the number of threads equal to the number of cores on this node.
 * If there are a number of mpi processes on this node as well, things
 * can go very slowly as there are so many more threads than cores.
 * Assume the NEURON users pc.nthread() is well chosen if
 * OMP_NUM_THREADS is not set.
 */
void set_openmp_threads(int nthread) {
#if defined(_OPENMP)
    if (!getenv("OMP_NUM_THREADS")) {
        omp_set_num_threads(nthread);
    }
#endif
}

/**
 * Convert char* containing arguments from neuron to char* argv[] for
 * coreneuron command line argument parser.
 */
char* prepare_args(int& argc, char**& argv, int use_mpi, const char* arg) {
    // first construct all arguments as string
    std::string args(arg);
    args.insert(0, " coreneuron ");
    args.append(" --skip-mpi-finalize ");
    if (use_mpi) {
        args.append(" --mpi ");
    }

    // we can't modify string with strtok, make copy
    char* first = strdup(args.c_str());
    const char* sep = " ";

    // first count the no of argument
    char* token = strtok(first, sep);
    argc = 0;
    while (token) {
        token = strtok(nullptr, sep);
        argc++;
    }
    free(first);

    // now build char*argv
    argv = new char*[argc];
    first = strdup(args.c_str());
    token = strtok(first, sep);
    for (int i = 0; token; i++) {
        argv[i] = token;
        token = strtok(nullptr, sep);
    }

    // return actual data to be freed
    return first;
}

}

namespace coreneuron {
void call_prcellstate_for_prcellgid(int prcellgid, int compute_gpu, int is_init);

void nrn_init_and_load_data(int argc,
                            char* argv[],
                            bool is_mapping_needed = false,
                            bool run_setup_cleanup = true) {
#if defined(NRN_FEEXCEPT)
    nrn_feenableexcept();
#endif

    /// profiler like tau/vtune : do not measure from begining
    Instrumentor::stop_profile();

    // memory footprint after mpi initialisation
    if (!corenrn_param.is_quiet()) {
        report_mem_usage("After MPI_Init");
    }

    // initialise default coreneuron parameters
    initnrn();

    // create mutex for nrn123, protect instance_count_
    nrnran123_mutconstruct();

    // set global variables
    // precedence is: set by user, globals.dat, 34.0
    celsius = corenrn_param.celsius;

#if _OPENACC
    if (!corenrn_param.gpu && corenrn_param.cell_interleave_permute == 2) {
        fprintf(
            stderr,
            "compiled with _OPENACC does not allow the combination of --cell-permute=2 and missing --gpu\n");
        exit(1);
    }
#endif

// if multi-threading enabled, make sure mpi library supports it
#if NRNMPI
    if (corenrn_param.threading) {
        nrnmpi_check_threading_support();
    }
#endif

    // full path of files.dat file
    std::string filesdat(corenrn_param.datpath + "/" + corenrn_param.filesdat);

    // read the global variable names and set their values from globals.dat
    set_globals(corenrn_param.datpath.c_str(), (corenrn_param.seed>=0),
                corenrn_param.seed);

    // set global variables for start time, timestep and temperature
    std::string restore_path = corenrn_param.restorepath;
    t = restore_time(restore_path.c_str());

    if (corenrn_param.dt != -1000.) {  // command line arg highest precedence
        dt = corenrn_param.dt;
    } else if (dt == -1000.) {  // not on command line and no dt in globals.dat
        dt = 0.025;             // lowest precedence
    }

    corenrn_param.dt = dt;

    rev_dt = (int)(1. / dt);

    if (corenrn_param.celsius != -1000.) {  // command line arg highest precedence
        celsius = corenrn_param.celsius;
    } else if (celsius == -1000.) {  // not on command line and no celsius in globals.dat
        celsius = 34.0;              // lowest precedence
    }

    corenrn_param.celsius = celsius;

#ifdef ISPC_INTEROP
    ispc_celsius = celsius;
#endif
    // create net_cvode instance
    mk_netcvode();

    // One part done before call to nrn_setup. Other part after.

    if (!corenrn_param.patternstim.empty()) {
        nrn_set_extra_thread0_vdata();
    }

    if (!corenrn_param.is_quiet()) {
        report_mem_usage("Before nrn_setup");
    }

    // set if need to interleave cells
    interleave_permute_type = corenrn_param.cell_interleave_permute;
    cellorder_nwarp = corenrn_param.nwarp;
    use_solve_interleave = corenrn_param.cell_interleave_permute;

#if LAYOUT == 1
    // permuting not allowed for AoS
    interleave_permute_type = 0;
    use_solve_interleave = false;
#endif

    if (corenrn_param.gpu && interleave_permute_type == 0) {
        if (nrnmpi_myid == 0) {
            printf(
                " WARNING : GPU execution requires --cell-permute type 1 or 2. Setting it to 1.\n");
        }
        interleave_permute_type = 1;
        use_solve_interleave = true;
    }

    // multisend options
    use_multisend_ = corenrn_param.multisend ? 1 : 0;
    n_multisend_interval = corenrn_param.ms_subint;
    use_phase2_ = (corenrn_param.ms_phases == 2) ? 1 : 0;

    // reading *.dat files and setting up the data structures, setting mindelay
    nrn_setup(filesdat.c_str(), is_mapping_needed, run_setup_cleanup,
              corenrn_param.datpath.c_str(), restore_path.c_str(), &corenrn_param.mindelay);

    // Allgather spike compression and  bin queuing.
    nrn_use_bin_queue_ = corenrn_param.binqueue;
    int spkcompress = corenrn_param.spkcompress;
    nrnmpi_spike_compress(spkcompress, (spkcompress ? true : false), use_multisend_);

    if (!corenrn_param.is_quiet()) {
        report_mem_usage("After nrn_setup ");
    }

    // Invoke PatternStim
    if (!corenrn_param.patternstim.empty()) {
        nrn_mkPatternStim(corenrn_param.patternstim.c_str(), corenrn_param.tstop);
    }

    /// Setting the timeout
    nrn_set_timeout(200.);

    // show all configuration parameters for current run
    if (nrnmpi_myid == 0 && !corenrn_param.is_quiet()) {
        std::cout << corenrn_param << std::endl;
        std::cout << " Start time (t) = " << t << std::endl << std::endl;
    }

    // allocate buffer for mpi communication
    mk_spikevec_buffer(corenrn_param.spikebuf);

    if (!corenrn_param.is_quiet()) {
        report_mem_usage("After mk_spikevec_buffer");
    }

    if (corenrn_param.gpu) {
        setup_nrnthreads_on_device(nrn_threads, nrn_nthread);
    }

    if (nrn_have_gaps) {
        nrn_partrans::gap_update_indices();
    }

    // call prcellstate for prcellgid
    call_prcellstate_for_prcellgid(corenrn_param.prcellgid, corenrn_param.gpu, 1);
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

    // clear spikes generated during forward skip (with negative time)
    clear_spike_vectors();
}

std::string cnrn_version() {
    return coreneuron::version::to_string();
}

// bsize = 0 then per step transfer
// bsize > 1 then full trajectory save into arrays.
void get_nrn_trajectory_requests(int bsize) {
    if (nrn2core_get_trajectory_requests_) {
        for (int tid = 0; tid < nrn_nthread; ++tid) {
            NrnThread& nt = nrn_threads[tid];
            int n_pr;
            int n_trajec;
            int* types;
            int* indices;
            void** vpr;
            double** varrays;
            double** pvars;

            // bsize is passed by reference, the return value will determine if
            // per step return or entire trajectory return.
            (*nrn2core_get_trajectory_requests_)(tid, bsize, n_pr, vpr, n_trajec, types, indices,
                                                 pvars, varrays);
            delete_trajectory_requests(nt);
            if (n_trajec) {
                TrajectoryRequests* tr = new TrajectoryRequests;
                nt.trajec_requests = tr;
                tr->bsize = bsize;
                tr->n_pr = n_pr;
                tr->n_trajec = n_trajec;
                tr->vsize = 0;
                tr->vpr = vpr;
                tr->gather = new double*[n_trajec];
                tr->varrays = varrays;
                tr->scatter = pvars;
                for (int i = 0; i < n_trajec; ++i) {
                    tr->gather[i] = stdindex2ptr(types[i], indices[i], nt);
                }
                delete[] types;
                delete[] indices;
            }
        }
    }
}

static void trajectory_return() {
    if (nrn2core_trajectory_return_) {
        for (int tid = 0; tid < nrn_nthread; ++tid) {
            NrnThread& nt = nrn_threads[tid];
            TrajectoryRequests* tr = nt.trajec_requests;
            if (tr && tr->varrays) {
                (*nrn2core_trajectory_return_)(tid, tr->n_pr, tr->vsize, tr->vpr, nt._t);
            }
        }
    }
}

std::unique_ptr<ReportHandler> create_report_handler(ReportConfiguration& config) {
    std::unique_ptr<ReportHandler> report_handler;
    if (config.format == "Bin") {
        report_handler = std::make_unique<BinaryReportHandler>(config);
    } else if (config.format == "SONATA") {
        report_handler = std::make_unique<SonataReportHandler>(config);
    }
    else {
        if (nrnmpi_myid == 0) {
            printf(" WARNING : Report name '%s' has unknown format: '%s'.\n", config.name.data(), config.format.data());
        }
        return nullptr;
    }
    return report_handler;
}

}  // namespace coreneuron

/// The following high-level functions are marked as "extern C"
/// for compat with C, namely Neuron mod files.
/// They split the previous solve_core so that intermediate init of external mechanisms can occur.
/// See mech/corenrnmech.cpp for the new all-in-one solve_core (not compiled into the coreneuron
/// lib since with nrnivmodl-core we have 'future' external mechanisms)

using namespace coreneuron;


extern "C" void mk_mech_init(int argc, char** argv) {
    // read command line parameters and parameter config files
    corenrn_param.parse(argc, argv);

#if NRNMPI
    if (corenrn_param.mpi_enable) {
        nrnmpi_init(&argc, &argv);
    }
#endif

#ifdef _OPENACC
    if (corenrn_param.gpu) {
        init_gpu();
    }
#endif

    if (!corenrn_param.writeParametersFilepath.empty()) {
        std::ofstream out(corenrn_param.writeParametersFilepath, std::ios::trunc);
        out << corenrn_param.app.config_to_str(false, false);
        out.close();
    }

    // reads mechanism information from bbcore_mech.dat

    mk_mech((corenrn_param.datpath).c_str());
}

extern "C" int run_solve_core(int argc, char** argv) {
    Instrumentor::phase_begin("main");

    std::vector<ReportConfiguration> configs;
    std::vector<std::unique_ptr<ReportHandler> > report_handlers;
    std::string spikes_population_name;
    bool reports_needs_finalize = false;

    if (!corenrn_param.is_quiet()) {
        report_mem_usage("After mk_mech");
    }

    // Create outpath if it does not exist
    if (nrnmpi_myid == 0) {
        mkdir_p(corenrn_param.outpath.c_str());
    }

    if (!corenrn_param.reportfilepath.empty()) {
        configs = create_report_configurations(corenrn_param.reportfilepath,
                                               corenrn_param.outpath,
                                               spikes_population_name);
        reports_needs_finalize = configs.size();
    }

    // initializationa and loading functions moved to separate
    {
        Instrumentor::phase p("load-model");
        nrn_init_and_load_data(argc, argv, !configs.empty());
    }

    nrn_checkpoint_arg_exists = !corenrn_param.checkpointpath.empty();
    if (nrn_checkpoint_arg_exists) {
        if (nrnmpi_myid == 0) {
            mkdir_p(corenrn_param.checkpointpath.c_str());
        }
    }

    std::string output_dir = corenrn_param.outpath;

    if (nrnmpi_myid == 0) {
        mkdir_p(output_dir.c_str());
    }
#if NRNMPI
    nrnmpi_barrier();
#endif
    bool compute_gpu = corenrn_param.gpu;
    bool skip_mpi_finalize = corenrn_param.skip_mpi_finalize;

// clang-format off
    #pragma acc data copyin(celsius, secondorder) if (compute_gpu)
    // clang-format on
    {
        double v = corenrn_param.voltage;
        double dt = corenrn_param.dt;
        double delay = corenrn_param.mindelay;
        double tstop = corenrn_param.tstop;

        if (tstop < t && nrnmpi_myid == 0) {
            printf("Error: Stop time (%lf) < Start time (%lf), restoring from checkpoint? \n",
                   tstop, t);
            abort();
        }

        // In direct mode there are likely trajectory record requests
        // to allow processing in NEURON after simulation by CoreNEURON
        if (corenrn_embedded) {
            // arg is vector size required but NEURON can instead
            // specify that returns will be on a per time step basis.
            get_nrn_trajectory_requests(int(tstop / dt) + 2);
            (*nrn2core_part2_clean_)();
        }

        // TODO : if some ranks are empty then restore will go in deadlock
        // phase (as some ranks won't have restored anything and hence return
        // false in checkpoint_initialize
        if (!checkpoint_initialize()) {
            nrn_finitialize(v != 1000., v);
        }

        if (!corenrn_param.is_quiet()) {
            report_mem_usage("After nrn_finitialize");
        }

        // register all reports into reportinglib
        double min_report_dt = INT_MAX;
        for (size_t i = 0; i < configs.size(); i++) {
            std::unique_ptr<ReportHandler> report_handler = create_report_handler(configs[i]);
            if(report_handler) {
                report_handler->create_report(dt, tstop, delay);
                report_handlers.push_back(std::move(report_handler));
            }
            if (configs[i].report_dt < min_report_dt) {
                min_report_dt = configs[i].report_dt;
            }
        }
        // Set the buffer size if is not the default value. Otherwise use report.conf on
        // register_report
        if (corenrn_param.report_buff_size != corenrn_param.report_buff_size_default) {
            set_report_buffer_size(corenrn_param.report_buff_size);
        }
        setup_report_engine(min_report_dt, delay);
        configs.clear();

        // call prcellstate for prcellgid
        call_prcellstate_for_prcellgid(corenrn_param.prcellgid, compute_gpu, 0);

        // handle forwardskip
        if (corenrn_param.forwardskip > 0.0) {
            handle_forward_skip(corenrn_param.forwardskip, corenrn_param.prcellgid);
        }

        /// Solver execution
        Instrumentor::start_profile();
        Instrumentor::phase_begin("simulation");
        BBS_netpar_solve(corenrn_param.tstop);
        Instrumentor::phase_end("simulation");
        Instrumentor::stop_profile();

        // direct mode and full trajectory gathering on CoreNEURON, send back.
        if (corenrn_embedded) {
            trajectory_return();
        }

        // Report global cell statistics
        report_cell_stats();

        // prcellstate after end of solver
        call_prcellstate_for_prcellgid(corenrn_param.prcellgid, compute_gpu, 0);
    }

    // write spike information to outpath
    {
        Instrumentor::phase p("output-spike");
        output_spikes(output_dir.c_str(), spikes_population_name);
    }

    // copy weights back to NEURON NetCon
    if (nrn2core_all_weights_return_) {

        // first update weights from gpu
        update_weights_from_gpu(nrn_threads, nrn_nthread);

        // store weight pointers
        std::vector<double*> weights(nrn_nthread, NULL);

        // could be one thread more (empty) than in NEURON but does not matter
        for (int i=0; i < nrn_nthread; ++i) {
          weights[i] = nrn_threads[i].weights;
        }
        (*nrn2core_all_weights_return_)(weights);
    }

    core2nrn_data_return();

    {
        Instrumentor::phase p("checkpoint");
        write_checkpoint(nrn_threads, nrn_nthread, corenrn_param.checkpointpath.c_str());
    }

    // must be done after checkpoint (to avoid deleting events)
    if (reports_needs_finalize) {
        finalize_report();
    }

    // Cleaning the memory
    nrn_cleanup();

    // tau needs to resume profile
    Instrumentor::start_profile();

// mpi finalize
#if NRNMPI
    if (!skip_mpi_finalize) {
        nrnmpi_finalize();
    }
#endif

    finalize_data_on_device();
    Instrumentor::phase_end("main");

    return 0;
}
