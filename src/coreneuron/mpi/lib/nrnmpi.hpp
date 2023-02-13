#pragma once

// This file contains functions that does not go outside of the mpi library
namespace coreneuron {
extern int nrnmpi_numprocs_;
extern int nrnmpi_myid_;
void nrnmpi_spike_initialize();
}  // namespace coreneuron
