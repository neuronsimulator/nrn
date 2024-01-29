#pragma once
#undef HAVE_PROTOTYPES
#if defined(__MINGW32__)
#undef _hypot
#define _hypot hypot
#endif
// https://docs.python.org/3/c-api/intro.html#include-files states: It is
// recommended to always define PY_SSIZE_T_CLEAN before including Python.h.
#ifndef PY_SSIZE_T_CLEAN
#define PY_SSIZE_T_CLEAN
#endif
#include <Python.h>
#undef snprintf
