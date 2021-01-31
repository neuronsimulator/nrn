/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <cstring>
#include <sys/time.h>

#include "coreneuron/nrnconf.h"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/mpi/mpispike.hpp"
#include "coreneuron/mpi/nrnmpi_def_cinc.h"
#include "coreneuron/utils/nrn_assert.h"
#if _OPENMP
#include <omp.h>
#endif
#if NRNMPI
#include <mpi.h>
#endif
namespace coreneuron {

#if NRNMPI
MPI_Comm nrnmpi_world_comm;
MPI_Comm nrnmpi_comm;
MPI_Comm nrn_bbs_comm;
static MPI_Group grp_bbs;
static MPI_Group grp_net;

extern void nrnmpi_spike_initialize();

static int nrnmpi_under_nrncontrol_;

void nrnmpi_init(int* pargc, char*** pargv) {
    nrnmpi_use = true;
    nrnmpi_under_nrncontrol_ = 1;

    int flag = 0;
    MPI_Initialized(&flag);

    if (!flag) {
#if defined(_OPENMP)
        int required = MPI_THREAD_FUNNELED;
        int provided;
        nrn_assert(MPI_Init_thread(pargc, pargv, required, &provided) == MPI_SUCCESS);

        nrn_assert(required <= provided);
#else
        nrn_assert(MPI_Init(pargc, pargv) == MPI_SUCCESS);
#endif
    }
    grp_bbs = MPI_GROUP_NULL;
    grp_net = MPI_GROUP_NULL;
    nrn_assert(MPI_Comm_dup(MPI_COMM_WORLD, &nrnmpi_world_comm) == MPI_SUCCESS);
    nrn_assert(MPI_Comm_dup(nrnmpi_world_comm, &nrnmpi_comm) == MPI_SUCCESS);
    nrn_assert(MPI_Comm_dup(nrnmpi_world_comm, &nrn_bbs_comm) == MPI_SUCCESS);
    nrn_assert(MPI_Comm_rank(nrnmpi_world_comm, &nrnmpi_myid_world) == MPI_SUCCESS);
    nrn_assert(MPI_Comm_size(nrnmpi_world_comm, &nrnmpi_numprocs_world) == MPI_SUCCESS);
    nrnmpi_numprocs = nrnmpi_numprocs_bbs = nrnmpi_numprocs_world;
    nrnmpi_myid = nrnmpi_myid_bbs = nrnmpi_myid_world;
    nrnmpi_spike_initialize();

    if (nrnmpi_myid == 0) {
#if defined(_OPENMP)
        printf(" num_mpi=%d\n num_omp_thread=%d\n\n", nrnmpi_numprocs_world, omp_get_max_threads());
#else
        printf(" num_mpi=%d\n\n", nrnmpi_numprocs_world);
#endif
    }
}

void nrnmpi_finalize(void) {
    if (nrnmpi_under_nrncontrol_) {
        int flag = 0;
        MPI_Initialized(&flag);
        if (flag) {
            MPI_Comm_free(&nrnmpi_world_comm);
            MPI_Comm_free(&nrnmpi_comm);
            MPI_Comm_free(&nrn_bbs_comm);
            MPI_Finalize();
        }
    }
}

void nrnmpi_terminate() {
    if (nrnmpi_use) {
        if (nrnmpi_under_nrncontrol_) {
            MPI_Finalize();
        }
        nrnmpi_use = false;
    }
}

// check if appropriate threading level supported (i.e. MPI_THREAD_FUNNELED)
void nrnmpi_check_threading_support() {
    int th = 0;
    if (nrnmpi_use) {
        MPI_Query_thread(&th);
        if (th < MPI_THREAD_FUNNELED) {
            nrn_fatal_error(
                "\n Current MPI library doesn't support MPI_THREAD_FUNNELED,\
                        \n Run without enabling multi-threading!");
        }
    }
}

/* so src/nrnpython/inithoc.cpp does not have to include a c++ mpi.h */
int nrnmpi_wrap_mpi_init(int* flag) {
    return MPI_Initialized(flag);
}

#endif

// TODO nrn_wtime(), nrn_abort(int) and nrn_fatal_error() to be moved to tools

double nrn_wtime() {
#if NRNMPI
    if (nrnmpi_use) {
        return MPI_Wtime();
    } else
#endif
    {
        struct timeval time1;
        gettimeofday(&time1, nullptr);
        return (time1.tv_sec + time1.tv_usec / 1.e6);
    }
}

void nrn_abort(int errcode) {
#if NRNMPI
    int flag;
    MPI_Initialized(&flag);
    if (flag) {
        MPI_Abort(MPI_COMM_WORLD, errcode);
    } else
#endif
    {
        abort();
    }
}

void nrn_fatal_error(const char* msg) {
    if (nrnmpi_myid == 0) {
        printf("%s\n", msg);
    }
    nrn_abort(-1);
}

int nrnmpi_initialized() {
    int flag = 0;
#if NRNMPI
    MPI_Initialized(&flag);
#endif
    return flag;
}

/**
 * Return local mpi rank within a shared memory node
 *
 * When performing certain operations, we need to know the rank of mpi
 * process on a given node. This function uses MPI 3 MPI_Comm_split_type
 * function and MPI_COMM_TYPE_SHARED key to find out the local rank.
 */
int nrnmpi_local_rank() {
    int local_rank = 0;
#if NRNMPI
    MPI_Comm local_comm;
    MPI_Comm_split_type(
        MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, nrnmpi_myid_world, MPI_INFO_NULL, &local_comm);
    MPI_Comm_rank(local_comm, &local_rank);
    MPI_Comm_free(&local_comm);
#endif
    return local_rank;
}

/**
 * Return number of ranks running on single shared memory node
 *
 * We use MPI 3 MPI_Comm_split_type function and MPI_COMM_TYPE_SHARED key to
 * determine number of mpi ranks within a shared memory node.
 */
int nrnmpi_local_size() {
    int local_size = 1;
#if NRNMPI
    MPI_Comm local_comm;
    MPI_Comm_split_type(
        MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, nrnmpi_myid_world, MPI_INFO_NULL, &local_comm);
    MPI_Comm_size(local_comm, &local_size);
    MPI_Comm_free(&local_comm);
#endif
    return local_size;
}


}  // namespace coreneuron
