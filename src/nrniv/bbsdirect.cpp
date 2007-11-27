#include <nrnmpiuse.h>
#if NRNMPI
#include "../parallel/bbsdirectmpi.cpp"
#else
#include "../parallel/bbsdirect.cpp"
#endif
