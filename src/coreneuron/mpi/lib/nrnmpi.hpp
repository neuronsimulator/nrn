#pragma once

// This file contains functions that does not go outside of the mpi library
namespace coreneuron {
extern int nrnmpi_numprocs_;
extern int nrnmpi_myid_;
void nrnmpi_spike_initialize();
}  // namespace coreneuron

extern "C" {
extern void nrnmpi_get_subworld_info(int* cnt, int* index, int* rank, int* numprocs, int* numprocs_world);
}