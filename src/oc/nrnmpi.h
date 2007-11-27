#ifndef nrnmpi_h
#define nrnmpi_h
#include "nrnmpiuse.h"

extern int nrnmpi_numprocs; /* size */
extern int nrnmpi_myid; /* rank */

#if NRNMPI

#if defined(__cplusplus)
extern "C" {
#endif

extern int nrnmpi_use; /* NEURON does MPI init and terminate?*/
extern void nrnmpi_init(int under_NEURON_control, int* pargc, char*** pargv);
extern void nrnmpi_terminate();
extern void nrnmpi_abort(int errcode);
extern double nrnmpi_wtime();

#if defined(__cplusplus)
}
#endif /*c++*/

#endif /*NRNMPI*/
#endif /*nrnmpi_h*/
