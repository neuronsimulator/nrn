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
