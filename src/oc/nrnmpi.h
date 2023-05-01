#ifndef nrnmpi_h
#define nrnmpi_h
#include "nrnmpiuse.h"

/* by default nrnmpi_numprocs_world = nrnmpi_numprocs = nrnmpi_numsubworlds and
   nrnmpi_myid_world = nrnmpi_myid and the bulletin board and network communication do
   not easily coexist. ParallelContext.subworlds(nsmall) divides the world into
   nrnmpi_numprocs_world/small subworlds of size nsmall.
*/
extern int nrnmpi_numprocs_world;      /* size of entire world. total size of all subworlds */
extern int nrnmpi_myid_world;          /* rank in entire world */
extern int nrnmpi_numprocs;            /* size of subworld */
extern int nrnmpi_myid;                /* rank in subworld */
extern int nrnmpi_numprocs_bbs;        /* number of subworlds */
extern int nrnmpi_myid_bbs;            /* rank in nrn_bbs_comm of rank 0 of a subworld */
extern int nrnmpi_subworld_change_cnt; /* increment from within void nrnmpi_subworld_size(int n) */
extern int nrnmpi_subworld_id;         /* subworld index on all ranks */
extern int nrnmpi_numprocs_subworld;   /* number of ranks in subworld on all ranks */

typedef struct {
    int gid;
    double spiketime;
} NRNMPI_Spike;

#if NRNMPI


extern int nrnmpi_use;                     /* NEURON does MPI init and terminate?*/
extern int nrn_cannot_use_threads_and_mpi; /* 0 if required <= provided from MPI_Init_thread */


#include "nrnmpidec.h"

#endif /*NRNMPI*/
#endif /*nrnmpi_h*/
