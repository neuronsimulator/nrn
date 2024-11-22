#pragma once

#include "nrnwrap_Python.h"
#include <cassert>


inline bool is_python_string(PyObject* python_string) {
    return PyUnicode_Check(python_string) || PyBytes_Check(python_string);
}

class Py2NRNString {
  public:
    Py2NRNString(PyObject* python_string, bool disable_release = false);

    ~Py2NRNString() {
        if (!disable_release_ && str_) {
            free(str_);
        }
    }
    inline char* c_str() const {
        return str_;
    }
    inline bool err() const {
        return str_ == NULL;
    }

    void set_pyerr(PyObject* type, const char* message);
    char* get_pyerr();

  private:
    Py2NRNString();
    Py2NRNString(const Py2NRNString&);
    Py2NRNString& operator=(const Py2NRNString&);

    char* str_;
    bool disable_release_;
};

/** @brief For when hoc_execerror must handle the Python error.
 *  Idiom: PyErr2NRNString e;
 *         -- clean up any python objects --
 *         hoc_execerr_ext("hoc message : %s", e.c_str());
 *  e will be automatically deleted even though execerror does not return.
 */
class PyErr2NRNString {
  public:
    PyErr2NRNString() {
        str_ = NULL;
    }

    ~PyErr2NRNString() {
        if (str_) {
            free(str_);
        }
    }

    inline char* c_str() const {
        return str_;
    }

    char* get_pyerr();

  private:
    PyErr2NRNString(const PyErr2NRNString&);
    PyErr2NRNString& operator=(const PyErr2NRNString&);

    char* str_;
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
