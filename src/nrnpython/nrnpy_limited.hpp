#pragma once

// Limited-API helpers for libnrnpython / hoc (Py_LIMITED_API=0x030C0000).
// Include after Python.h. PyTypeObject is opaque; extra type data uses
// PEP 697 negative basicsize.

#include <cstdio>
#include <cstring>
#include <string>

#ifndef PyList_GET_ITEM
#define PyList_GET_ITEM(op, i) PyList_GetItem((op), (i))
#endif
#ifndef PyTuple_GET_SIZE
#define PyTuple_GET_SIZE(op) PyTuple_Size(op)
#endif
#ifndef PyFloat_AS_DOUBLE
#define PyFloat_AS_DOUBLE(op) PyFloat_AsDouble(op)
#endif

#if defined(Py_LIMITED_API)
#define PyUnicode_AsUTF8(op) PyUnicode_AsUTF8AndSize((op), nullptr)

template <class T>
inline void nrn_Py_IncRef(T* o) {
    Py_IncRef(reinterpret_cast<PyObject*>(o));
}
#undef Py_INCREF
#define Py_INCREF(op) nrn_Py_IncRef(op)

template <class T>
inline void nrn_Py_DecRef(T* o) {
    Py_DecRef(reinterpret_cast<PyObject*>(o));
}
#undef Py_DECREF
#define Py_DECREF(op) nrn_Py_DecRef(op)

template <class T>
inline void nrn_Py_XIncRef(T* o) {
    if (o) {
        Py_IncRef(reinterpret_cast<PyObject*>(o));
    }
}
#undef Py_XINCREF
#define Py_XINCREF(op) nrn_Py_XIncRef(op)

template <class T>
inline void nrn_Py_XDecRef(T* o) {
    if (o) {
        Py_DecRef(reinterpret_cast<PyObject*>(o));
    }
}
#undef Py_XDECREF
#define Py_XDECREF(op) nrn_Py_XDecRef(op)
#endif

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

#if defined(Py_LIMITED_API)
#undef PyObject_New
#define PyObject_New(type, typeobj) ((type*) nrnpy_tp_alloc((typeobj), 0))
#endif

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

inline PyObject* nrnpy_run_string(const char* str, int start, PyObject* globals, PyObject* locals) {
    PyObject* code = Py_CompileString(str, "<string>", start);
    if (!code) {
        return nullptr;
    }
    PyObject* result = PyEval_EvalCode(code, globals, locals);
    Py_DECREF(code);
    return result;
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

inline int nrnpy_run_interactive_console() {
    static const char* lines[] = {
        "import code as nrn_limited_code\n",
        "nrn_limited_interpreter = "
        "nrn_limited_code.InteractiveConsole(locals=globals())\n",
        "nrn_limited_interpreter.interact(\"\")\n",
    };
    for (const char* line: lines) {
        if (nrnpy_run_simple_string(line)) {
            PyErr_Print();
            return -1;
        }
    }
    return 0;
}
