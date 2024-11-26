#include "nrnpy_utils.h"

#include <tuple>
#include <nanobind/nanobind.h>

namespace nb = nanobind;

inline std::tuple<nb::object, nb::object, nb::object> fetch_pyerr() {
    PyObject* ptype = NULL;
    PyObject* pvalue = NULL;
    PyObject* ptraceback = NULL;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);

    return std::make_tuple(nb::steal(ptype), nb::steal(pvalue), nb::steal(ptraceback));
}


Py2NRNString::Py2NRNString(PyObject* python_string, bool disable_release) {
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

void Py2NRNString::set_pyerr(PyObject* type, const char* message) {
    nb::object err_type;
    nb::object err_value;
    nb::object err_traceback;

    if (err()) {
        std::tie(err_type, err_value, err_traceback) = fetch_pyerr();
    }
    if (err_value && err_type) {
        auto umes = nb::steal(
            PyUnicode_FromFormat("%s (Note: %S: %S)", message, err_type.ptr(), err_value.ptr()));
        PyErr_SetObject(type, umes.ptr());
    } else {
        PyErr_SetString(type, message);
    }
}

char* Py2NRNString::get_pyerr() {
    if (err()) {
        auto [ptype, pvalue, ptraceback] = fetch_pyerr();
        if (pvalue) {
            auto pstr = nb::steal(PyObject_Str(pvalue.ptr()));
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
    }
    PyErr_Clear();  // in case could not turn pvalue into c_str.
    return str_;
}

char* PyErr2NRNString::get_pyerr() {
    if (PyErr_Occurred()) {
        auto [ptype, pvalue, ptraceback] = fetch_pyerr();
        if (pvalue) {
            auto pstr = nb::steal(PyObject_Str(pvalue.ptr()));
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
    }
    PyErr_Clear();  // in case could not turn pvalue into c_str.
    return str_;
}
