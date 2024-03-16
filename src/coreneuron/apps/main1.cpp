/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

/**
 * @file main1.cpp
 * @date 26 Oct 2014
 * @brief File containing main driver routine for CoreNeuron
 */

#include <cstring>
#include <climits>
#include <dlfcn.h>
#include <filesystem>
#include <memory>
#include <vector>

#include "coreneuron/config/config.h"
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
#include "coreneuron/utils/nrn_stats.h"
#include "coreneuron/io/reports/nrnreport.hpp"
#include "coreneuron/io/reports/report_handler.hpp"
#include "coreneuron/io/reports/sonata_report_handler.hpp"
#include "coreneuron/gpu/nrn_acc_manager.hpp"
#include "coreneuron/utils/profile/profiler_interface.h"
#include "coreneuron/network/partrans.hpp"
#include "coreneuron/network/multisend.hpp"
#include "coreneuron/io/nrn_setup.hpp"
#include "coreneuron/io/nrn2core_direct.h"
#include "coreneuron/io/core2nrn_data_return.hpp"
#include "coreneuron/utils/utils.hpp"

namespace fs = std::filesystem;

extern "C" {
const char* corenrn_version() {
    return coreneuron::bbcore_write_version;
}

void (*nrn2core_part2_clean_)();

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
char* prepare_args(int& argc, char**& argv, std::string& args) {
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
    argv = new char*[argc + 1];
    first = strdup(args.c_str());
    token = strtok(first, sep);
    for (int i = 0; token; i++) {
        argv[i] = token;
        token = strtok(nullptr, sep);
    }

    // make sure argv is terminated by NULL!
    argv[argc] = nullptr;

    // return actual data to be freed
    return first;
}
}

namespace coreneuron {
void call_prcellstate_for_prcellgid(int prcellgid, int compute_gpu, int is_init);

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
            (*nrn2core_get_trajectory_requests_)(
                tid, bsize, n_pr, vpr, n_trajec, types, indices, pvars, varrays);
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

void nrn_init_and_load_data(int argc,
                            char* argv[],
                            CheckPoints& checkPoints,
                            bool is_mapping_needed,
                            bool run_setup_cleanup) {
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

    // set global variables
    // precedence is: set by user, globals.dat, 34.0
    celsius = corenrn_param.celsius;

#if CORENEURON_ENABLE_GPU
    if (!corenrn_param.gpu && corenrn_param.cell_interleave_permute == 2) {
        fprintf(stderr,
                "compiled with CORENEURON_ENABLE_GPU does not allow the combination of "
                "--cell-permute=2 and "
                "missing --gpu\n");
        exit(1);
    }
    if (!corenrn_param.gpu && corenrn_param.cuda_interface) {
        fprintf(stderr,
                "compiled with OpenACC/CUDA does not allow the combination of --cuda-interface and "
                "missing --gpu\n");
        exit(1);
    }
#endif

// if multi-threading enabled, make sure mpi library supports it
#if NRNMPI
    if (corenrn_param.mpi_enable && corenrn_param.threading) {
        nrnmpi_check_threading_support();
    }
#endif

    // full path of files.dat file
    std::string filesdat(corenrn_param.datpath + "/" + corenrn_param.filesdat);

    // read the global variable names and set their values from globals.dat
    set_globals(corenrn_param.datpath.c_str(), (corenrn_param.seed >= 0), corenrn_param.seed);

    // set global variables for start time, timestep and temperature
    if (!corenrn_embedded) {
        t = checkPoints.restore_time();
    }

    if (corenrn_param.dt != -1000.) {  // command line arg highest precedence
        dt = corenrn_param.dt;
    } else if (dt == -1000.) {  // not on command line and no dt in globals.dat
        dt = 0.025;             // lowest precedence
    }

    corenrn_param.dt = dt;

    rev_dt = (int) (1. / dt);

    if (corenrn_param.celsius != -1000.) {  // command line arg highest precedence
        celsius = corenrn_param.celsius;
    } else if (celsius == -1000.) {  // not on command line and no celsius in globals.dat
        celsius = 34.0;              // lowest precedence
    }

    corenrn_param.celsius = celsius;

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
    nrn_setup(filesdat.c_str(),
              is_mapping_needed,
              checkPoints,
              run_setup_cleanup,
              corenrn_param.datpath.c_str(),
              checkPoints.get_restore_path().c_str(),
              &corenrn_param.mindelay);

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

    // In direct mode there are likely trajectory record requests
    // to allow processing in NEURON after simulation by CoreNEURON
    if (corenrn_embedded) {
        // arg is additional vector size required (how many items will be
        // written to the double*) but NEURON can instead
        // specify that returns will be on a per time step basis.
        get_nrn_trajectory_requests(int((corenrn_param.tstop - t) / corenrn_param.dt) + 2);

        // In direct mode, CoreNEURON has exactly the behavior of
        // ParallelContext.psolve(tstop). Ie a sequence of such calls
        // without an intervening h.finitialize() continues from the end
        // of the previous call. I.e., all initial state, including
        // the event queue has been set up in NEURON. And, at the end
        // all final state, including the event queue will be sent back
        // to NEURON. Here there is some first time only
        // initialization and queue transfer.
        direct_mode_initialize();
        clear_spike_vectors();  // PreSyn send already recorded by NEURON
        (*nrn2core_part2_clean_)();
    }

    if (corenrn_param.gpu) {
        // Copy nrnthreads to device only after all the data are passed from NEURON and the
        // nrnthreads on CPU are properly set up
        setup_nrnthreads_on_device(nrn_threads, nrn_nthread);
    }

    if (corenrn_embedded) {
        // Run nrn_init of mechanisms only to allocate any extra data needed on the GPU after
        // nrnthreads are properly set up on the GPU
        allocate_data_in_mechanism_nrn_init();
    }

    if (corenrn_param.gpu) {
        if (nrn_have_gaps) {
            nrn_partrans::copy_gap_indices_to_device();
        }
    }

    // call prcellstate for prcellgid
    call_prcellstate_for_prcellgid(corenrn_param.prcellgid, corenrn_param.gpu, 1);
}

void call_prcellstate_for_prcellgid(int prcellgid, int compute_gpu, int is_init) {
    if (prcellgid >= 0) {
        std::string prcellname{compute_gpu ? "acc_gpu" : "cpu"};
        if (is_init) {
            prcellname += "_init";
        } else {
            prcellname += "_t";
            prcellname += std::to_string(t);
        }
        update_nrnthreads_on_host(nrn_threads, nrn_nthread);
        prcellstate(prcellgid, prcellname.c_str());
    }
}

/* perform forwardskip and call prcellstate for prcellgid */
void handle_forward_skip(double forwardskip, int prcellgid) {
    double savedt = dt;
    double savet = t;

    dt = forwardskip * 0.1;
    t = -1e9;
    dt2thread(-1.);

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
    return version::to_string();
}


static void trajectory_return() {
    if (nrn2core_trajectory_return_) {
        for (int tid = 0; tid < nrn_nthread; ++tid) {
            NrnThread& nt = nrn_threads[tid];
            TrajectoryRequests* tr = nt.trajec_requests;
            if (tr && tr->varrays) {
                (*nrn2core_trajectory_return_)(tid, tr->n_pr, tr->bsize, tr->vsize, tr->vpr, nt._t);
            }
        }
    }
}

std::unique_ptr<ReportHandler> create_report_handler(const ReportConfiguration& config,
                                                     const SpikesInfo& spikes_info) {
    std::unique_ptr<ReportHandler> report_handler;
    if (config.format == "SONATA") {
        report_handler = std::make_unique<SonataReportHandler>(spikes_info);
    } else {
        if (nrnmpi_myid == 0) {
            printf(" WARNING : Report name '%s' has unknown format: '%s'.\n",
                   config.name.data(),
                   config.format.data());
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

#if NRNMPI && defined(NRNMPI_DYNAMICLOAD)
static void* load_dynamic_mpi(const std::string& libname) {
    dlerror();
    void* handle = dlopen(libname.c_str(), RTLD_NOW | RTLD_GLOBAL);
    const char* error = dlerror();
    if (error) {
        std::string err_msg = std::string("Could not open dynamic MPI library: ") + error + "\n";
        throw std::runtime_error(err_msg);
    }
    return handle;
}
#endif

extern "C" void mk_mech_init(int argc, char** argv) {
    // reset all parameters to their default values
    corenrn_param.reset();

    // read command line parameters and parameter config files
    corenrn_param.parse(argc, argv);

#if NRNMPI
    if (corenrn_param.mpi_enable) {
#ifdef NRNMPI_DYNAMICLOAD
        // coreneuron rely on neuron to detect mpi library distribution and
        // the name of the library itself. Make sure the library name is specified
        // via CLI option.
        if (corenrn_param.mpi_lib.empty()) {
            throw std::runtime_error(
                "For dynamic MPI support you must pass '--mpi-lib "
                "/path/libcorenrnmpi_<name>.<suffix>` argument!\n");
        }

        // neuron can call coreneuron multiple times and hence we do not
        // want to initialize/load mpi library multiple times
        static bool mpi_lib_loaded = false;
        if (!mpi_lib_loaded) {
            auto mpi_handle = load_dynamic_mpi(corenrn_param.mpi_lib);
            mpi_manager().resolve_symbols(mpi_handle);
            mpi_lib_loaded = true;
        }
#endif
        auto ret = nrnmpi_init(&argc, &argv, corenrn_param.is_quiet());
        nrnmpi_numprocs = ret.numprocs;
        nrnmpi_myid = ret.myid;
    }
#endif

#ifdef CORENEURON_ENABLE_GPU
    if (corenrn_param.gpu) {
        init_gpu();
        cnrn_target_copyin(&celsius);
        cnrn_target_copyin(&pi);
        cnrn_target_copyin(&secondorder);
        nrnran123_initialise_global_state_on_device();
    }
#endif

    if (!corenrn_param.writeParametersFilepath.empty()) {
        std::ofstream out(corenrn_param.writeParametersFilepath, std::ios::trunc);
        out << corenrn_param.config_to_str(false, false);
        out.close();
    }

    // reads mechanism information from bbcore_mech.dat
    mk_mech((corenrn_param.datpath).c_str());
}

extern "C" int run_solve_core(int argc, char** argv) {
    Instrumentor::phase_begin("main");

    std::vector<ReportConfiguration> configs;
    std::vector<std::unique_ptr<ReportHandler>> report_handlers;
    SpikesInfo spikes_info;
    bool reports_needs_finalize = false;

    if (!corenrn_param.is_quiet()) {
        report_mem_usage("After mk_mech");
    }

    // Create outpath if it does not exist
    if (nrnmpi_myid == 0) {
        fs::create_directories(corenrn_param.outpath);
    }

    if (!corenrn_param.reportfilepath.empty()) {
        configs = create_report_configurations(corenrn_param.reportfilepath,
                                               corenrn_param.outpath,
                                               spikes_info);
        reports_needs_finalize = !configs.empty();
    }

    CheckPoints checkPoints{corenrn_param.checkpointpath, corenrn_param.restorepath};

    // initializationa and loading functions moved to separate
    {
        Instrumentor::phase p("load-model");
        nrn_init_and_load_data(argc, argv, checkPoints, !configs.empty());
    }

    std::string output_dir = corenrn_param.outpath;

    if (nrnmpi_myid == 0) {
        fs::create_directories(output_dir);
    }
#if NRNMPI
    if (corenrn_param.mpi_enable) {
        nrnmpi_barrier();
    }
#endif
    bool compute_gpu = corenrn_param.gpu;

    nrn_pragma_acc(update device(celsius, secondorder, pi) if (compute_gpu))
    nrn_pragma_omp(target update to(celsius, secondorder, pi) if (compute_gpu))
    {
        double v = corenrn_param.voltage;
        double dt = corenrn_param.dt;
        double delay = corenrn_param.mindelay;
        double tstop = corenrn_param.tstop;

        if (tstop < t && nrnmpi_myid == 0) {
            printf("Error: Stop time (%lf) < Start time (%lf), restoring from checkpoint? \n",
                   tstop,
                   t);
            abort();
        }

        // TODO : if some ranks are empty then restore will go in deadlock
        // phase (as some ranks won't have restored anything and hence return
        // false in checkpoint_initialize
        if (!corenrn_embedded && !checkPoints.initialize()) {
            nrn_finitialize(v != 1000., v);
        }

        if (!corenrn_param.is_quiet()) {
            report_mem_usage("After nrn_finitialize");
        }

        // register all reports with libsonata
        double min_report_dt = INT_MAX;
        for (size_t i = 0; i < configs.size(); i++) {
            std::unique_ptr<ReportHandler> report_handler = create_report_handler(configs[i],
                                                                                  spikes_info);
            if (report_handler) {
                report_handler->create_report(configs[i], dt, tstop, delay);
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

        if (!configs.empty()) {
            setup_report_engine(min_report_dt, delay);
            configs.clear();
        }

        // call prcellstate for prcellgid
        call_prcellstate_for_prcellgid(corenrn_param.prcellgid, compute_gpu, 0);

        // handle forwardskip
        if (corenrn_param.forwardskip > 0.0) {
            Instrumentor::phase p("handle-forward-skip");
            handle_forward_skip(corenrn_param.forwardskip, corenrn_param.prcellgid);
        }

        /// Solver execution
        Instrumentor::start_profile();
        Instrumentor::phase_begin("simulation");
        BBS_netpar_solve(corenrn_param.tstop);
        Instrumentor::phase_end("simulation");
        Instrumentor::stop_profile();

        // update cpu copy of NrnThread from GPU
        update_nrnthreads_on_host(nrn_threads, nrn_nthread);

        // direct mode and full trajectory gathering on CoreNEURON, send back.
        if (corenrn_embedded) {
            trajectory_return();
        }

        // Report global cell statistics
        if (!corenrn_param.is_quiet()) {
            report_cell_stats();
        }

        // prcellstate after end of solver
        call_prcellstate_for_prcellgid(corenrn_param.prcellgid, compute_gpu, 0);
    }

    // write spike information to outpath
    {
        Instrumentor::phase p("output-spike");
        output_spikes(output_dir.c_str(), spikes_info);
    }

    // copy weights back to NEURON NetCon
    if (nrn2core_all_weights_return_ && corenrn_embedded) {
        // first update weights from gpu
        update_weights_from_gpu(nrn_threads, nrn_nthread);

        // store weight pointers
        std::vector<double*> weights(nrn_nthread, nullptr);

        // could be one thread more (empty) than in NEURON but does not matter
        for (int i = 0; i < nrn_nthread; ++i) {
            weights[i] = nrn_threads[i].weights;
        }
        (*nrn2core_all_weights_return_)(weights);
    }

    if (corenrn_embedded) {
        core2nrn_data_return();
    }

    {
        Instrumentor::phase p("checkpoint");
        checkPoints.write_checkpoint(nrn_threads, nrn_nthread);
    }

    // must be done after checkpoint (to avoid deleting events)
    if (reports_needs_finalize) {
        finalize_report();
    }

    // cleanup threads on GPU
    if (corenrn_param.gpu) {
        delete_nrnthreads_on_device(nrn_threads, nrn_nthread);
        if (nrn_have_gaps) {
            nrn_partrans::delete_gap_indices_from_device();
        }
        nrnran123_destroy_global_state_on_device();
        cnrn_target_delete(&secondorder);
        cnrn_target_delete(&pi);
        cnrn_target_delete(&celsius);
    }

    // Cleaning the memory
    nrn_cleanup();

    // tau needs to resume profile
    Instrumentor::start_profile();

// mpi finalize
#if NRNMPI
    if (corenrn_param.mpi_enable && !corenrn_param.skip_mpi_finalize) {
        nrnmpi_finalize();
    }
#endif

    Instrumentor::phase_end("main");

    return 0;
}
