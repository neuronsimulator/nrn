/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <iostream>
#include <string>
#include <tuple>

#include "coreneuron/nrnconf.h"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/utils/nrn_assert.h"
#include "nrnmpi.hpp"
#if _OPENMP
#include <omp.h>
#endif
#include <mpi.h>
namespace coreneuron {

MPI_Comm nrnmpi_world_comm;
MPI_Comm nrnmpi_comm;
int nrnmpi_numprocs_;
int nrnmpi_myid_;

static bool nrnmpi_under_nrncontrol_{false};

static void nrn_fatal_error(const char* msg) {
    if (nrnmpi_myid_ == 0) {
        printf("%s\n", msg);
    }
    nrnmpi_abort_impl(-1);
}

nrnmpi_init_ret_t nrnmpi_init_impl(int* pargc, char*** pargv) {
    nrnmpi_under_nrncontrol_ = true;

    if (!nrnmpi_initialized_impl()) {
#if defined(_OPENMP)
        int required = MPI_THREAD_FUNNELED;
        int provided;
        nrn_assert(MPI_Init_thread(pargc, pargv, required, &provided) == MPI_SUCCESS);

        nrn_assert(required <= provided);
#else
        nrn_assert(MPI_Init(pargc, pargv) == MPI_SUCCESS);
#endif
    }
    nrn_assert(MPI_Comm_dup(MPI_COMM_WORLD, &nrnmpi_world_comm) == MPI_SUCCESS);
    nrn_assert(MPI_Comm_dup(nrnmpi_world_comm, &nrnmpi_comm) == MPI_SUCCESS);
    nrn_assert(MPI_Comm_rank(nrnmpi_world_comm, &nrnmpi_myid_) == MPI_SUCCESS);
    nrn_assert(MPI_Comm_size(nrnmpi_world_comm, &nrnmpi_numprocs_) == MPI_SUCCESS);
    nrnmpi_spike_initialize();

    if (nrnmpi_myid_ == 0) {
#if defined(_OPENMP)
        printf(" num_mpi=%d\n num_omp_thread=%d\n\n", nrnmpi_numprocs_, omp_get_max_threads());
#else
        printf(" num_mpi=%d\n\n", nrnmpi_numprocs_);
#endif
    }

    return {nrnmpi_numprocs_, nrnmpi_myid_};
}

void nrnmpi_finalize_impl(void) {
    if (nrnmpi_under_nrncontrol_) {
        if (nrnmpi_initialized_impl()) {
            MPI_Comm_free(&nrnmpi_world_comm);
            MPI_Comm_free(&nrnmpi_comm);
            MPI_Finalize();
        }
    }
}

// check if appropriate threading level supported (i.e. MPI_THREAD_FUNNELED)
void nrnmpi_check_threading_support_impl() {
    int th = 0;
    MPI_Query_thread(&th);
    if (th < MPI_THREAD_FUNNELED) {
        nrn_fatal_error(
            "\n Current MPI library doesn't support MPI_THREAD_FUNNELED,\
                    \n Run without enabling multi-threading!");
    }
}

bool nrnmpi_initialized_impl() {
    int flag = 0;
    MPI_Initialized(&flag);
    return flag != 0;
}

void nrnmpi_abort_impl(int errcode) {
    MPI_Abort(MPI_COMM_WORLD, errcode);
}

double nrnmpi_wtime_impl() {
    return MPI_Wtime();
}

/**
 * Return local mpi rank within a shared memory node
 *
 * When performing certain operations, we need to know the rank of mpi
 * process on a given node. This function uses MPI 3 MPI_Comm_split_type
 * function and MPI_COMM_TYPE_SHARED key to find out the local rank.
 */
int nrnmpi_local_rank_impl() {
    int local_rank = 0;
    if (nrnmpi_initialized_impl()) {
        MPI_Comm local_comm;
        MPI_Comm_split_type(
            MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, nrnmpi_myid_, MPI_INFO_NULL, &local_comm);
        MPI_Comm_rank(local_comm, &local_rank);
        MPI_Comm_free(&local_comm);
    }
    return local_rank;
}

/**
 * Return number of ranks running on single shared memory node
 *
 * We use MPI 3 MPI_Comm_split_type function and MPI_COMM_TYPE_SHARED key to
 * determine number of mpi ranks within a shared memory node.
 */
int nrnmpi_local_size_impl() {
    int local_size = 1;
    if (nrnmpi_initialized_impl()) {
        MPI_Comm local_comm;
        MPI_Comm_split_type(
            MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, nrnmpi_myid_, MPI_INFO_NULL, &local_comm);
        MPI_Comm_size(local_comm, &local_size);
        MPI_Comm_free(&local_comm);
    }
    return local_size;
}

/**
 * Write given buffer to a new file using MPI collective I/O
 *
 * For output like spikes, each rank has to write spike timing
 * information to a single file. This routine writes buffers
 * of length len1, len2, len3... at the offsets 0, 0+len1,
 * 0+len1+len2... offsets. This write op is a collective across
 * all ranks of the common MPI communicator used for spike exchange.
 *
 * @param filename Name of the file to write
 * @param buffer Buffer to write
 * @param length Length of the buffer to write
 */
void nrnmpi_write_file_impl(const std::string& filename, const char* buffer, size_t length) {
    MPI_File fh;
    MPI_Status status;

    // global offset into file
    unsigned long offset = 0;
    MPI_Exscan(&length, &offset, 1, MPI_UNSIGNED_LONG, MPI_SUM, nrnmpi_comm);

    int op_status = MPI_File_open(
        nrnmpi_comm, filename.c_str(), MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
    if (op_status != MPI_SUCCESS && nrnmpi_myid_ == 0) {
        std::cerr << "Error while opening output file " << filename << std::endl;
        abort();
    }

    op_status = MPI_File_write_at_all(fh, offset, buffer, length, MPI_BYTE, &status);
    if (op_status != MPI_SUCCESS && nrnmpi_myid_ == 0) {
        std::cerr << "Error while writing output " << std::endl;
        abort();
    }

    MPI_File_close(&fh);
}
}  // namespace coreneuron
