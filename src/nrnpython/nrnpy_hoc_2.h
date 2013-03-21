// ugh. all this just for proper bool(hocobject)
// there has to be a better way to get working __nonzero__
// but putting it into hocobj_methods did not work.
static PyNumberMethods hocobj_as_number = {
        0,                   /* nb_add */
        0,                   /* nb_subtract */
        0,                   /* nb_multiply */
        0,                   /* nb_divide */
        0,                   /* nb_remainder */
        0,                /* nb_divmod */
        0,                   /* nb_power */
        0,        /* nb_negative */
        0,        /* nb_positive */
        0,        /* nb_absolute */
        (inquiry)hocobj_nonzero,      /* nb_nonzero */
        0,     /* nb_invert */
        0,                /* nb_lshift */
        0,                /* nb_rshift */
        0,                   /* nb_and */
        0,                   /* nb_xor */
        0,                    /* nb_or */
        0,                /* nb_coerce */
        0,        /* nb_int */
        0,       /* nb_long */
        0,      /* nb_float */
        0,        /* nb_oct */
        0,        /* nb_hex */
        0,                  /* nb_inplace_add */
        0,                  /* nb_inplace_subtract */
        0,                  /* nb_inplace_multiply */
        0,                  /* nb_inplace_divide */
        0,                  /* nb_inplace_remainder */
        0,                  /* nb_inplace_power */
        0,               /* nb_inplace_lshift */
        0,               /* nb_inplace_rshift */
        0,                  /* nb_inplace_and */
        0,                  /* nb_inplace_xor */
        0,                   /* nb_inplace_or */
        0,              /* nb_floor_divide */
        0,               /* nb_true_divide */
        0,             /* nb_inplace_floor_divide */
        0,              /* nb_inplace_true_divide */
#if PYTHON_API_VERSION > 1012
        0,      /* nb_index */
#endif
};

#undef ccast
#if PYTHON_API_VERSION < 1013
#define ccast (char*)
#else
#define ccast /**/
#endif

static PyTypeObject nrnpy_HocObjectType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    ccast "hoc.HocObject",         /*tp_name*/
    sizeof(PyHocObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)hocobj_dealloc,                        /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &hocobj_as_number,                         /*tp_as_number*/
    &hocobj_seqmeth,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    (hashfunc) hocobj_hash,                         /*tp_hash */
    (ternaryfunc)hocobj_call,                         /*tp_call*/
    0,                          /*tp_str*/
    hocobj_getattro,                         /*tp_getattro*/
    hocobj_setattro,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    ccast hocobj_docstring,         /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    (richcmpfunc) hocobj_richcmp,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    &hocobj_iter,		               /* tp_iter */
    &hocobj_iternext,		               /* tp_iternext */
    hocobj_methods,             /* tp_methods */
    0,//hocobj_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)hocobj_init,      /* tp_init */
    0,                         /* tp_alloc */
    hocobj_new,                 /* tp_new */
};
