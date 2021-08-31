/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#ifndef nrnmpi_h
#define nrnmpi_h

#include <string>

#include "coreneuron/mpi/nrnmpiuse.h"

namespace coreneuron {
/* by default nrnmpi_numprocs_world = nrnmpi_numprocs = nrnmpi_numsubworlds and
   nrnmpi_myid_world = nrnmpi_myid and the bulletin board and network communication do
   not easily coexist. ParallelContext.subworlds(nsmall) divides the world into
   nrnmpi_numprocs_world/small subworlds of size nsmall.
*/
extern int nrnmpi_numprocs_world; /* size of entire world. total size of all subworlds */
extern int nrnmpi_myid_world;     /* rank in entire world */
extern int nrnmpi_numprocs;       /* size of subworld */
extern int nrnmpi_myid;           /* rank in subworld */
extern int nrnmpi_numprocs_bbs;   /* number of subworlds */
extern int nrnmpi_myid_bbs;       /* rank in nrn_bbs_comm of rank 0 of a subworld */

extern void nrn_abort(int errcode);
extern void nrn_fatal_error(const char* msg);
extern double nrn_wtime(void);
extern int nrnmpi_local_rank();
extern int nrnmpi_local_size();
}  // namespace coreneuron

#if defined(NRNMPI)

namespace coreneuron {
typedef struct {
    int gid;
    double spiketime;
} NRNMPI_Spike;

extern bool nrnmpi_use; /* NEURON does MPI init and terminate?*/

// Write given buffer to a new file using MPI collective I/O
extern void nrnmpi_write_file(const std::string& filename, const char* buffer, size_t length);

}  // namespace coreneuron
#include "coreneuron/mpi/nrnmpidec.h"

#endif /*NRNMPI*/
#endif /*nrnmpi_h*/
