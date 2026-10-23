#pragma once

// Limited-API helpers for libnrnpython / hoc (Py_LIMITED_API=0x030C0000).
// Include after Python.h. PyTypeObject is opaque; extra type data uses
// PEP 697 negative basicsize.

#include <cstdio>
#include <cstring>
#include <string>

inline PyObject* nrnpy_tp_alloc(PyTypeObject* type, Py_ssize_t nitems = 0) {
    using allocfunc_t = PyObject* (*) (PyTypeObject*, Py_ssize_t);
    auto alloc = reinterpret_cast<allocfunc_t>(PyType_GetSlot(type, Py_tp_alloc));
    if (!alloc) {
        return PyType_GenericAlloc(type, nitems);
    }
    return alloc(type, nitems);
}

template <class T>
inline void nrnpy_tp_free(T* self) {
    auto* py = reinterpret_cast<PyObject*>(self);
    using freefunc_t = void (*)(void*);
    auto free_slot = reinterpret_cast<freefunc_t>(PyType_GetSlot(Py_TYPE(py), Py_tp_free));
    if (free_slot) {
        free_slot(py);
    } else {
        PyObject_Free(py);
    }
}

inline PyObject* nrnpy_type_mro(PyTypeObject* type) {
    return PyObject_GetAttrString(reinterpret_cast<PyObject*>(type), "__mro__");
}

inline int nrnpy_run_simple_string(const char* command) {
    PyObject* main_mod = PyImport_AddModule("__main__");
    if (!main_mod) {
        return -1;
    }
    PyObject* dict = PyModule_GetDict(main_mod);
    PyObject* code = Py_CompileString(command, "<string>", Py_file_input);
    if (!code) {
        return -1;
    }
    PyObject* result = PyEval_EvalCode(code, dict, dict);
    Py_DECREF(code);
    if (!result) {
        return -1;
    }
    Py_DECREF(result);
    return 0;
}

inline int nrnpy_run_file(FILE* fp, const char* filename) {
    std::string src;
    char buf[4096];
    while (true) {
        size_t n = std::fread(buf, 1, sizeof buf, fp);
        if (n) {
            src.append(buf, n);
        }
        if (n < sizeof buf) {
            break;
        }
    }
    PyObject* main_mod = PyImport_AddModule("__main__");
    if (!main_mod) {
        return -1;
    }
    PyObject* dict = PyModule_GetDict(main_mod);
    // CPython sets __file__ in __main__ for `python script.py`; PyEval_EvalCode does not.
    if (PyObject* fileobj = PyUnicode_DecodeFSDefault(filename)) {
        if (PyDict_SetItemString(dict, "__file__", fileobj) < 0) {
            Py_DECREF(fileobj);
            PyErr_Print();
            return -1;
        }
        Py_DECREF(fileobj);
    } else {
        PyErr_Clear();
    }
    PyObject* code = Py_CompileString(src.c_str(), filename, Py_file_input);
    if (!code) {
        PyErr_Print();
        return -1;
    }
    PyObject* result = PyEval_EvalCode(code, dict, dict);
    Py_DECREF(code);
    if (!result) {
        PyErr_Print();
        return -1;
    }
    Py_DECREF(result);
    return 0;
}
