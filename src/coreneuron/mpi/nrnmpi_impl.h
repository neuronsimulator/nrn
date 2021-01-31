/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef nrnmpi_impl_h
#define nrnmpi_impl_h

#if NRNMPI

#include <mpi.h>
namespace coreneuron {
extern MPI_Comm nrnmpi_world_comm;
extern MPI_Comm nrnmpi_comm;
}  // namespace coreneuron
#endif  // NRNMPI

#endif
