#include "nb_defs.h"

struct _object;
using PyObject = struct _object;

extern "C" NB_EXPORT PyObject* PyInit_hoc();
#if !defined(MINHG)
extern "C" NB_EXPORT void modl_reg();
#endif
