/* do not want the redef in the dynamic load case */
#include <nrnmpiuse.h>

#if NRNMPI_DYNAMICLOAD
#include <nrnmpi_dynam.h>
#endif

#include <nrnmpi.h>

#if NRNMPI
#include <mpi.h>
#include <nrnmpidec.h>
#include "nrnmpi_impl.h"

#include <iostream>

#include "neuron/container/memory_usage.hpp"

static void sum_reduce_memory_usage(void* invec, void* inoutvec, int* len_, MPI_Datatype*) {
    int len = *len_;

    auto a = static_cast<neuron::container::MemoryUsage*>(invec);
    auto ab = static_cast<neuron::container::MemoryUsage*>(inoutvec);

    for (int i = 0; i < len; ++i) {
        ab[i] += a[i];
    }
}

void nrnmpi_memory_stats(neuron::container::MemoryStats& stats,
                         neuron::container::MemoryUsage const& local_memory_usage) {
    MPI_Op op;
    MPI_Op_create(sum_reduce_memory_usage, /* commute = */ 1, &op);

    MPI_Datatype memory_usage_mpitype;
    MPI_Type_contiguous(sizeof(neuron::container::MemoryUsage), MPI_BYTE, &memory_usage_mpitype);
    MPI_Type_commit(&memory_usage_mpitype);

    MPI_Allreduce(&local_memory_usage, &stats.total, 1, memory_usage_mpitype, op, nrnmpi_comm);

    MPI_Op_free(&op);
    MPI_Type_free(&memory_usage_mpitype);
}

void nrnmpi_print_memory_stats(neuron::container::MemoryStats const& memory_stats) {
    if (nrnmpi_myid_world == 0) {
        std::cout << format_memory_usage(memory_stats.total) << "\n";
    }
}
#endif
