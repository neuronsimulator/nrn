#ifndef nrnmpi_h
#define nrnmpi_h
#include "simcore/nrnmpi/nrnmpiuse.h"

/* by default nrnmpi_numprocs_world = nrnmpi_numprocs = nrnmpi_numsubworlds and
   nrnmpi_myid_world = nrnmpi_myid and the bulletin board and network communication do
   not easily coexist. ParallelContext.subworlds(nsmall) divides the world into
   nrnmpi_numprocs_world/small subworlds of size nsmall.
*/
extern int nrnmpi_numprocs_world; /* size of entire world. total size of all subworlds */
extern int nrnmpi_myid_world; /* rank in entire world */
extern int nrnmpi_numprocs; /* size of subworld */
extern int nrnmpi_myid; /* rank in subworld */
extern int nrnmpi_numprocs_bbs; /* number of subworlds */
extern int nrnmpi_myid_bbs; /* rank in nrn_bbs_comm of rank 0 of a subworld */

typedef struct {
	int gid;
	double spiketime;
} NRNMPI_Spike;
	        
#if NRNMPI

#if defined(__cplusplus)
extern "C" {
#endif

extern int nrnmpi_use; /* NEURON does MPI init and terminate?*/

#if defined(__cplusplus)
}
#endif /*c++*/

#include "simcore/nrnmpi/nrnmpidec.h"

#endif /*NRNMPI*/
#endif /*nrnmpi_h*/
