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

/** Initialize mechanisms and run simulation using CoreNEURON
 *
 * This is mainly used to build nrniv-core executable
 */
int solve_core(int argc, char** argv) {
    mk_mech_init(argc, argv);
    coreneuron::modl_reg();
    int ret = run_solve_core(argc, argv);
    coreneuron::nrn_cleanup_ion_map();
    return ret;
}

extern "C" {

/// global variables from coreneuron library
extern bool corenrn_embedded;
extern int corenrn_embedded_nthread;

/// parse arguments from neuron and prepare new one for coreneuron
char* prepare_args(int& argc, char**& argv, int use_mpi, const char* nrn_arg);

/// initialize standard mechanisms from coreneuron
void mk_mech_init(int argc, char** argv);

/// set openmp threads equal to neuron's pthread
void set_openmp_threads(int nthread);

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
                         const char* nrn_arg) {
    // set coreneuron's internal variable based on neuron arguments
    corenrn_embedded = true;
    corenrn_embedded_nthread = nthread;
    coreneuron::nrn_have_gaps = have_gaps != 0;
    coreneuron::nrn_use_fast_imem = use_fast_imem != 0;

    // set number of openmp threads
    set_openmp_threads(nthread);

    // pre-process argumnets from neuron and prepare new for coreneuron
    int argc;
    char** argv;
    char* new_arg = prepare_args(argc, argv, use_mpi, nrn_arg);

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
