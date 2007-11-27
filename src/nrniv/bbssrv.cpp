#include <nrnmpiuse.h>
#if NRNMPI
#include "../parallel/bbssrvmpi.cpp"
#else
#include "../parallel/bbssrv.cpp"
#endif
