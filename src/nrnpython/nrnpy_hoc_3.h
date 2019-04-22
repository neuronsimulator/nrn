#include "nrnpython.h"


static PyNumberMethods hocobj_as_number = {
     py_hocobj_add, /* nb_add */
     py_hocobj_sub, /* nb_subtract */
     py_hocobj_mul, /* nb_multiply */
     NULL, /* nb_remainder */
     NULL, /* nb_divmod */
     NULL, /* nb_power */
     py_hocobj_uneg, /* nb_negative */
     py_hocobj_upos, /* nb_positive */
     py_hocobj_uabs, /* nb_absolute */
     NULL, /* nb_bool */
     NULL, /* nb_invert */
     NULL, /* nb_lshift */
     NULL, /* nb_rshift */
     NULL, /* nb_and */
     NULL, /* nb_xor */
     NULL, /* nb_or */
     NULL, /* nb_int */
     NULL, /*nb_reserved */
     NULL, /* nb_float */

     NULL, /* nb_inplace_add */
     NULL, /* nb_inplace_subtract */
     NULL, /* nb_inplace_multiply */
     NULL, /* nb_inplace_remainder */
     NULL, /* nb_inplace_power */
     NULL, /* nb_inplace_lshift */
     NULL, /* nb_inplace_rshift */
     NULL, /* nb_inplace_and */
     NULL, /* nb_inplace_xor */
     NULL, /* nb_inplace_or */

     NULL, /* nb_floor_divide */
     py_hocobj_div, /* nb_true_divide */
     NULL, /* nb_inplace_floor_divide */
     NULL, /* nb_inplace_true_divide */

     NULL, /* nb_index */

     NULL, /* nb_matrix_multiply */
     NULL /* nb_inplace_matrix_multiply */
};

static PyType_Slot nrnpy_HocObjectType_slots[] = {
    {Py_tp_dealloc, (void*)hocobj_dealloc},
    {Py_tp_repr, (void*)hocobj_repr},
    {Py_tp_hash, (void*)hocobj_hash},
    {Py_tp_call, (void*)hocobj_call},
    {Py_tp_getattro, (void*)hocobj_getattro},
    {Py_tp_setattro, (void*)hocobj_setattro},
    {Py_tp_richcompare, (void*)hocobj_richcmp},
    {Py_tp_iter, (void*)&hocobj_iter},
    {Py_tp_iternext, (void*)&hocobj_iternext},
    {Py_tp_methods, (void*)hocobj_methods},
    {Py_tp_init, (void*)hocobj_init},
    {Py_tp_new, (void*)hocobj_new},
    {Py_tp_doc, (void*)hocobj_docstring},
    {Py_nb_bool, (void*)hocobj_nonzero},
    {Py_sq_length, (void*)hocobj_len},
    {Py_sq_item, (void*)hocobj_getitem},
    {Py_sq_ass_item, (void*)hocobj_setitem},
    {0, 0},
};
static PyType_Spec nrnpy_HocObjectType_spec = {
    "hoc.HocObject",
    sizeof(PyHocObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    nrnpy_HocObjectType_slots,
};


static struct PyModuleDef hocmodule = {PyModuleDef_HEAD_INIT, "hoc",
                                       "HOC interaction with Python", -1,
                                       HocMethods, NULL, NULL, NULL, NULL};
