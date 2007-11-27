#include <nrnmpiuse.h>
#if NRNMPI
#include "../parallel/bbsclimpi.cpp"
#else
#include "../parallel/bbsrcli.cpp"
#endif
