#include <nrnmpiuse.h>
#if NRNMPI
#include "../parallel/bbssrv2mpi.cpp"
#else
#include "../parallel/bbslsrv2.cpp"
#endif
