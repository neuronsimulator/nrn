#ifndef nrnpython_h
#define nrnpython_h

#ifdef _WIN64
#define MS_WIN64
#define MS_WIN32
#define MS_WINDOWS
#endif

#include <../../nrnconf.h>
#include <nrnpython_config.h>
#if defined(USE_PYTHON)
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#include <Python.h>

#if (PY_MAJOR_VERSION >= 3)
#define myPyMODINIT_FUNC PyObject*
#else
#define myPyMODINIT_FUNC void

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif /*PyMODINIT_FUNC*/
#ifndef PY_FORMAT_SIZE_T
#define Py_ssize_t int
#endif /*PY_FORMAT_SIZE_T*/
#endif /* PY_MAJOR_VERSION */

#endif /*USE_PYTHON*/

#if (PY_MAJOR_VERSION >= 3)
#define PyString_Check PyUnicode_Check
#define PyString_AsString(o) nrnpy_PyString_AsString(o)
#define PyString_FromString PyUnicode_FromString
#define PyInt_Check PyLong_Check
#define PyInt_CheckExact PyLong_CheckExact
#define PyInt_AS_LONG PyLong_AsLong
#define PyInt_AsLong PyLong_AsLong
#define PyInt_FromLong PyLong_FromLong
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if (PY_MAJOR_VERSION >= 3)
char* nrnpy_PyString_AsString(PyObject*);
void nrnpy_pystring_asstring_free(const char*);
#else
#define nrnpy_pystring_asstring_free(a) /**/
#endif
extern PyObject* nrnpy_hoc_pop();
extern int nrnpy_numbercheck(PyObject*);

#if defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ > __SIZEOF_LONG__
#define castptr2long (long)(long long)
#else
#define castptr2long (long)
#endif

#if defined(__cplusplus)
}
#endif

#endif
