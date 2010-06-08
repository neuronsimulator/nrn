#ifndef nrnmpi_impl_h
#define nrnmpi_impl_h

#include <mpi.h>

extern void* hoc_Emalloc(unsigned long size);
extern void* hoc_Erealloc(void* buf, unsigned long size);
extern void hoc_malchk();

extern MPI_Comm nrnmpi_world_comm;
extern MPI_Comm nrnmpi_comm;

#endif
