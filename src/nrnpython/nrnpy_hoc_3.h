#include "nrnpython.h"

static PyType_Slot nrnpy_HocObjectType_slots[] = {
    {Py_tp_dealloc, (void*) hocobj_dealloc},
    {Py_tp_repr, (void*) hocobj_repr},
    {Py_tp_hash, (void*) hocobj_hash},
    {Py_tp_call, (void*) hocobj_call},
    {Py_tp_getattro, (void*) hocobj_getattro},
    {Py_tp_setattro, (void*) hocobj_setattro},
    {Py_tp_richcompare, (void*) hocobj_richcmp},
    {Py_tp_iter, (void*) &hocobj_iter},
    {Py_tp_iternext, (void*) &hocobj_iternext},
    {Py_tp_methods, (void*) hocobj_methods},
    {Py_tp_init, (void*) hocobj_init},
    {Py_tp_new, (void*) hocobj_new},
    {Py_tp_doc, (void*) hocobj_docstring},
    {Py_nb_bool, (void*) hocobj_nonzero},
    {Py_sq_length, (void*) hocobj_len},
    {Py_sq_item, (void*) hocobj_getitem},
    {Py_sq_ass_item, (void*) hocobj_setitem},
    {Py_nb_add, (PyObject*) py_hocobj_add},
    {Py_nb_subtract, (PyObject*) py_hocobj_sub},
    {Py_nb_multiply, (PyObject*) py_hocobj_mul},
    {Py_nb_negative, (PyObject*) py_hocobj_uneg},
    {Py_nb_positive, (PyObject*) py_hocobj_upos},
    {Py_nb_absolute, (PyObject*) py_hocobj_uabs},
    {Py_nb_true_divide, (PyObject*) py_hocobj_div},
    {0, 0},
};
static PyType_Spec nrnpy_HocObjectType_spec = {
    "hoc.HocObject",
    sizeof(PyHocObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    nrnpy_HocObjectType_slots,
};


static struct PyModuleDef hocmodule = {PyModuleDef_HEAD_INIT,
                                       "hoc",
                                       "HOC interaction with Python",
                                       -1,
                                       HocMethods,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL};
