#pragma once

#include "nrnwrap_Python.h"
#include "nrn_export.hpp"
#include "neuron/unique_cstr.hpp"
#include <cassert>


inline bool is_python_string(PyObject* python_string) {
    return PyUnicode_Check(python_string) || PyBytes_Check(python_string);
}

class NRN_EXPORT Py2NRNString {
  public:
    [[nodiscard]] static neuron::unique_cstr as_ascii(PyObject* python_string);

    static void set_pyerr(PyObject* type, const char* message);
    [[nodiscard]] static neuron::unique_cstr get_pyerr();

  private:
    Py2NRNString() = delete;
    Py2NRNString(const Py2NRNString&) = delete;
    Py2NRNString& operator=(const Py2NRNString&) = delete;
};

extern void nrnpy_sec_referr();
#define CHECK_SEC_INVALID(sec)  \
    {                           \
        if (!sec->prop) {       \
            nrnpy_sec_referr(); \
            return NULL;        \
        }                       \
    }

extern void nrnpy_prop_referr();
#define CHECK_PROP_INVALID(propid) \
    {                              \
        if (!propid) {             \
            nrnpy_prop_referr();   \
            return NULL;           \
        }                          \
    }
