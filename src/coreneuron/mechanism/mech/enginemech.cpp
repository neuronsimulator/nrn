/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/

/**
 * \file
 * \brief Provides interface function for CoreNEURON mechanism library and NEURON
 *
 * libcorenrnmech is a interface library provided to building standalone executable
 * special-core. Also, it is used by NEURON to run CoreNEURON via dlopen to execute
 * models via in-memory transfer.
 */

#include <cstdlib>
#include <coreneuron/engine.h>
#include <string>
#include <iostream>

namespace coreneuron {

/** Mechanism registration function
 *
 * If external mechanisms present then use modl_reg function generated
 * in mod_func.cpp otherwise use empty one.
 */
#ifdef ADDITIONAL_MECHS
extern void modl_reg();
#else
void modl_reg() {}
#endif

/// variables defined in coreneuron library
extern bool nrn_have_gaps;
extern bool nrn_use_fast_imem;

/// function defined in coreneuron library
extern void nrn_cleanup_ion_map();
}  // namespace coreneuron

extern "C" {

/// global variables from coreneuron library
extern bool corenrn_embedded;
extern int corenrn_embedded_nthread;

/// parse arguments from neuron and prepare new one for coreneuron
char* prepare_args(int& argc, char**& argv, std::string& args);

/// initialize standard mechanisms from coreneuron
void mk_mech_init(int argc, char** argv);

/// set openmp threads equal to neuron's pthread
void set_openmp_threads(int nthread);

/**
 * Add MPI library loading CLI argument for CoreNEURON
 *
 * CoreNEURON requires `--mpi-lib` CLI argument with the
 * path of library. In case of `solve_core()` call from MOD
 * file, such CLI argument may not present. In this case, we
 * additionally check `NRN_CORENRN_MPI_LIB` env variable set
 * by NEURON.
 *
 * @param mpi_lib path of coreneuron MPI library to load
 * @param args char* argv[] in std::string form
 */
void add_mpi_library_arg(const char* mpi_lib, std::string& args) {
    std::string corenrn_mpi_lib;

    // check if user or neuron has provided one
    if (mpi_lib != nullptr) {
        corenrn_mpi_lib = std::string(mpi_lib);
    }

    // if mpi library is not provided / empty then try to use what
    // neuron might have detected and set via env var `NRN_CORENRN_MPI_LIB`
    if (corenrn_mpi_lib.empty()) {
        char* lib = getenv("NRN_CORENRN_MPI_LIB");
        if (lib != nullptr) {
            corenrn_mpi_lib = std::string(lib);
        }
    }

    // add mpi library argument if found
    if (!corenrn_mpi_lib.empty()) {
        args.append(" --mpi-lib ");
        args.append(corenrn_mpi_lib);
    }
}

/** Run CoreNEURON in embedded mode with NEURON
 *
 * @param nthread Number of Pthreads on NEURON side
 * @param have_gaps True if gap junctions are used
 * @param use_mpi True if MPI is used on NEURON side
 * @param use_fast_imem True if fast imembrance calculation enabled
 * @param nrn_arg Command line arguments passed by NEURON
 * @return 1 if embedded mode is used otherwise 0
 * \todo Change return type semantics
 */
int corenrn_embedded_run(int nthread,
                         int have_gaps,
                         int use_mpi,
                         int use_fast_imem,
                         const char* mpi_lib,
                         const char* nrn_arg) {
    bool corenrn_skip_write_model_to_disk = false;
    const std::string corenrn_skip_write_model_to_disk_arg{"--skip-write-model-to-disk"};
    // If "only_simulate_str" exists in "nrn_arg" then avoid transferring any data between NEURON
    // and CoreNEURON Instead run the CoreNEURON simulation only with the coredat files provided
    // "only_simulate_str" is an internal string and shouldn't be made public to the CoreNEURON CLI
    // options so it is removed from "nrn_arg" first construct all arguments as string
    std::string filtered_nrn_arg{nrn_arg};
    const auto ind =
        static_cast<std::string>(filtered_nrn_arg).find(corenrn_skip_write_model_to_disk_arg);
    if (ind != std::string::npos) {
        corenrn_skip_write_model_to_disk = true;
        filtered_nrn_arg.erase(ind, corenrn_skip_write_model_to_disk_arg.size());
    }
    // set coreneuron's internal variable based on neuron arguments
    corenrn_embedded = !corenrn_skip_write_model_to_disk;
    corenrn_embedded_nthread = nthread;
    coreneuron::nrn_have_gaps = have_gaps != 0;
    coreneuron::nrn_use_fast_imem = use_fast_imem != 0;

    // set number of openmp threads
    set_openmp_threads(nthread);

    filtered_nrn_arg.insert(0, " coreneuron ");
    filtered_nrn_arg.append(" --skip-mpi-finalize ");

    if (use_mpi) {
        filtered_nrn_arg.append(" --mpi ");
    }

    add_mpi_library_arg(mpi_lib, filtered_nrn_arg);

    // pre-process argumnets from neuron and prepare new for coreneuron
    int argc;
    char** argv;

    char* new_arg = prepare_args(argc, argv, filtered_nrn_arg);

    // initialize internal arguments
    mk_mech_init(argc, argv);

    // initialize extra arguments built into special-core
    static bool modl_reg_called = false;
    if (!modl_reg_called) {
        coreneuron::modl_reg();
        modl_reg_called = true;
    }
    // run simulation
    run_solve_core(argc, argv);

    // free temporary string created from prepare_args
    free(new_arg);

    // delete array for argv
    delete[] argv;

    return corenrn_embedded ? 1 : 0;
}
}

/** Initialize mechanisms and run simulation using CoreNEURON
 *
 * NOTE: this type of usage via MOD file exist in neurodamus
 * where we delete entire model and call coreneuron via file
 * mode.
 */
int solve_core(int argc, char** argv) {
    // first construct argument as string
    std::string args;
    for (int i = 0; i < argc; i++) {
        args.append(argv[i]);
        args.append(" ");
    }

    // add mpi library argument
    add_mpi_library_arg("", args);

    // pre-process arguments and prepare it fore coreneuron
    int new_argc;
    char** new_argv;
    char* arg_to_free = prepare_args(new_argc, new_argv, args);

    mk_mech_init(new_argc, new_argv);
    coreneuron::modl_reg();
    int ret = run_solve_core(new_argc, new_argv);

    coreneuron::nrn_cleanup_ion_map();
    free(arg_to_free);
    delete[] new_argv;

    return ret;
}
