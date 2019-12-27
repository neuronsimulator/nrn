#ifndef nrnmpi_impl_h
#define nrnmpi_impl_h

#include <mpi.h>

extern void* hoc_Emalloc(size_t size);
extern void* hoc_Erealloc(void* buf, size_t size);
extern void hoc_malchk();

extern MPI_Comm nrnmpi_world_comm;
extern MPI_Comm nrnmpi_comm;

#if 0
#define guard(f) assert(f == MPI_SUCCESS)
#else
#define guard(f) {int _i = f; if (_i != MPI_SUCCESS) {printf("%s %d\n", #f, _i); assert(0);}}
#endif

#endif
