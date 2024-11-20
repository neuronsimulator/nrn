#pragma once

#include "nrnwrap_Python.h"
#include <cassert>

#include <nanobind/nanobind.h>

namespace nb = nanobind;

inline bool is_python_string(PyObject* python_string) {
    return PyUnicode_Check(python_string) || PyBytes_Check(python_string);
}

class Py2NRNString {
  public:
    Py2NRNString(PyObject* python_string, bool disable_release = false) {
        disable_release_ = disable_release;
        str_ = NULL;
        if (PyUnicode_Check(python_string)) {
            auto py_bytes = nb::steal(PyUnicode_AsASCIIString(python_string));
            if (py_bytes) {
                str_ = strdup(PyBytes_AsString(py_bytes.ptr()));
                if (!str_) {  // errno is ENOMEM
                    PyErr_SetString(PyExc_MemoryError, "strdup in Py2NRNString");
                }
            }
        } else if (PyBytes_Check(python_string)) {
            str_ = strdup(PyBytes_AsString(python_string));
            // assert(strlen(str_) == PyBytes_Size(python_string))
            // not checking for embedded '\0'
            if (!str_) {  // errno is ENOMEM
                PyErr_SetString(PyExc_MemoryError, "strdup in Py2NRNString");
            }
        } else {  // Neither Unicode or PyBytes
            PyErr_SetString(PyExc_TypeError, "Neither Unicode or PyBytes");
        }
    }

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
    inline void set_pyerr(PyObject* type, const char* message) {
        nb::object err_type;
        nb::object err_value;

        if (err()) {
            PyObject* ptype = NULL;
            PyObject* pvalue = NULL;
            PyObject* ptraceback = nullptr;
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);
            err_type = nb::steal(ptype);
            err_value = nb::steal(pvalue);
            Py_XDECREF(ptraceback);
        }
        if (err_value && err_type) {
            auto umes = nb::steal(PyUnicode_FromFormat(
                "%s (Note: %S: %S)", message, err_type.ptr(), err_value.ptr()));
            PyErr_SetObject(type, umes.ptr());
        } else {
            PyErr_SetString(type, message);
        }
    }

    inline char* get_pyerr() {
        if (err()) {
            PyObject* ptype = NULL;
            PyObject* pvalue = NULL;
            PyObject* ptraceback = NULL;
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);
            if (pvalue) {
                auto pstr = nb::steal(PyObject_Str(pvalue));
                if (pstr) {
                    const char* err_msg = PyUnicode_AsUTF8(pstr.ptr());
                    if (err_msg) {
                        str_ = strdup(err_msg);
                    } else {
                        str_ = strdup("get_pyerr failed at PyUnicode_AsUTF8");
                    }
                } else {
                    str_ = strdup("get_pyerr failed at PyObject_Str");
                }
            } else {
                str_ = strdup("get_pyerr failed at PyErr_Fetch");
            }
            Py_XDECREF(ptype);
            Py_XDECREF(pvalue);
            Py_XDECREF(ptraceback);
        }
        PyErr_Clear();  // in case could not turn pvalue into c_str.
        return str_;
    }

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

    inline char* get_pyerr() {
        PyObject* ptype = NULL;
        PyObject* pvalue = NULL;
        PyObject* ptraceback = NULL;
        if (PyErr_Occurred()) {
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);
            if (pvalue) {
                PyObject* pstr = PyObject_Str(pvalue);
                if (pstr) {
                    const char* err_msg = PyUnicode_AsUTF8(pstr);
                    if (err_msg) {
                        str_ = strdup(err_msg);
                    } else {
                        str_ = strdup("get_pyerr failed at PyUnicode_AsUTF8");
                    }
                    Py_XDECREF(pstr);
                } else {
                    str_ = strdup("get_pyerr failed at PyObject_Str");
                }
            } else {
                str_ = strdup("get_pyerr failed at PyErr_Fetch");
            }
        }
        PyErr_Clear();  // in case could not turn pvalue into c_str.
        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);
        return str_;
    }

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
