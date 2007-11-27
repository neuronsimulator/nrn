#ifndef nrnpython_h
#define nrnpython_h

#include <../../nrnconf.h>
#include <nrnpython_config.h>
#if defined(USE_PYTHON)
#define myPyMODINIT_FUNC void

#undef _POSIX_C_SOURCE
#include <Python.h>
#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
#ifndef PY_FORMAT_SIZE_T
#define Py_ssize_t int
#endif

#endif

#if defined(__cplusplus)
extern "C" {
#endif

extern PyObject* nrnpy_hoc_pop();
extern int nrnpy_numbercheck(PyObject*);

#if defined(__cplusplus)
}
#endif

#endif
