#ifndef nrnmpi_impl_h
#define nrnmpi_impl_h

#include <mpi.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void* hoc_Emalloc(size_t size);
extern void hoc_malchk();
extern void* hoc_Erealloc(void* buf, size_t size);

extern MPI_Comm nrnmpi_world_comm;
extern MPI_Comm nrnmpi_comm;

#if defined(__cplusplus)
}
#endif

#endif
