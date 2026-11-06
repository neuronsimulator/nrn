#pragma once
#include "nrndlldef.h"
#include "nrnmpiuse.h"

#ifdef __cplusplus
extern "C" {
#endif
/* by default nrnmpi_numprocs_world = nrnmpi_numprocs = nrnmpi_numsubworlds and
   nrnmpi_myid_world = nrnmpi_myid and the bulletin board and network communication do
   not easily coexist. ParallelContext.subworlds(nsmall) divides the world into
   nrnmpi_numprocs_world/small subworlds of size nsmall.
*/
extern NRN_DLLSYM int nrnmpi_numprocs_world; /* size of entire world. total size of all subworlds */
extern NRN_DLLSYM int nrnmpi_myid_world;     /* rank in entire world */
extern NRN_DLLSYM int nrnmpi_numprocs;       /* size of subworld */
extern NRN_DLLSYM int nrnmpi_myid;           /* rank in subworld */
extern NRN_DLLSYM int nrnmpi_numprocs_bbs;   /* number of subworlds */
extern NRN_DLLSYM int nrnmpi_myid_bbs;       /* rank in nrn_bbs_comm of rank 0 of a subworld */
#ifdef __cplusplus
}
#endif

struct NRNMPI_Spike {
    int gid;
    double spiketime;
};

#if NRNMPI

#ifdef __cplusplus
extern "C" {
#endif
extern NRN_DLLSYM int nrnmpi_use;                     /* NEURON does MPI init and terminate?*/
extern NRN_DLLSYM int nrn_cannot_use_threads_and_mpi; /* 0 if required <= provided from
                                                         MPI_Init_thread */
#ifdef __cplusplus
}
#endif

#include "nrnmpidec.h"

#endif /*NRNMPI*/
