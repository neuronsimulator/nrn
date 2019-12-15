
#undef ccast
#if PYTHON_API_VERSION < 1013
#define ccast (char*)
#else
#define ccast /**/
#endif

static PyTypeObject nrnpy_SectionType = {
    PyObject_HEAD_INIT(NULL)0,                /*ob_size*/
    ccast "nrn.Section",                      /*tp_name*/
    sizeof(NPySecObj),                        /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)NPySecObj_dealloc,            /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    pysec_repr,                               /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    (hashfunc)pysec_hash,                     /*tp_hash*/
    (ternaryfunc)NPySecObj_call,              /*tp_call*/
    0,                                        /*tp_str*/
    (getattrofunc)section_getattro,           /*tp_getattro*/
    (setattrofunc)section_setattro,           /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    ccast "Section objects",                  /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    (richcmpfunc)pysec_richcmp,               /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    (getiterfunc)seg_of_section_iter,         /* tp_iter */
    0,                                        /* tp_iternext */
    NPySecObj_methods,                        /* tp_methods */
    0,                                        /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)NPySecObj_init,                 /* tp_init */
    0,                                        /* tp_alloc */
    NPySecObj_new,                            /* tp_new */
};

static PyTypeObject nrnpy_AllSegOfSecIterType = {
    PyObject_HEAD_INIT(NULL)0,                /*ob_size*/
    ccast "nrn.AllSegOfSecIter",              /*tp_name*/
    sizeof(NPyAllSegOfSecIter),               /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)NPyAllSegOfSecIter_dealloc,   /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    ccast
    "Iterate over all Segments of a Section, including x=0 and 1", /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    (getiterfunc)allseg_of_sec_iter,          /* tp_iter */
    (iternextfunc)allseg_of_sec_next,         /* tp_iternext */
    0,                                        /* tp_methods */
    0,                                        /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)NPyAllSegOfSecIter_init,        /* tp_init */
    0,                                        /* tp_alloc */
    NPyAllSegOfSecIter_new,                   /* tp_new */
};

static PyTypeObject nrnpy_SegmentType = {
    PyObject_HEAD_INIT(NULL)0,                /*ob_size*/
    ccast "nrn.Segment",                      /*tp_name*/
    sizeof(NPySegObj),                        /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)NPySegObj_dealloc,            /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    pyseg_repr,                               /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    (hashfunc)pyseg_hash,                     /*tp_hash*/
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    (getattrofunc)segment_getattro,           /*tp_getattro*/
    (setattrofunc)segment_setattro,           /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Segment objects",                        /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    (richcmpfunc)pyseg_richcmp,               /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    (getiterfunc)mech_of_segment_iter,        /* tp_iter */
    0,                                        /* tp_iternext */
    NPySegObj_methods,                        /* tp_methods */
    NPySegObj_members,                        /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)NPySegObj_init,                 /* tp_init */
    0,                                        /* tp_alloc */
    NPySegObj_new,                            /* tp_new */
};


static PySequenceMethods rv_seqmeth = {
    rv_len, NULL, NULL, rv_getitem, NULL, rv_setitem, NULL, NULL, NULL, NULL
};

static PyTypeObject nrnpy_RangeType = {
    PyObject_HEAD_INIT(NULL)0,                /*ob_size*/
    ccast "nrn.RangeVar",                     /*tp_name*/
    sizeof(NPyRangeVar),                      /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)NPyRangeVar_dealloc,          /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    &rv_seqmeth,                              /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Range Variable Array objects",           /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    NPyRangeVar_methods,                      /* tp_methods */
    0,                                        /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)NPyRangeVar_init,               /* tp_init */
    0,                                        /* tp_alloc */
    NPyRangeVar_new,                          /* tp_new */
};

static PyTypeObject nrnpy_MechanismType = {
    PyObject_HEAD_INIT(NULL)0,                /*ob_size*/
    ccast "nrn.Mechanism",                    /*tp_name*/
    sizeof(NPyMechObj),                       /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)NPyMechObj_dealloc,           /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    pymech_repr,                              /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    (getattrofunc)mech_getattro,              /*tp_getattro*/
    (setattrofunc)mech_setattro,              /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Mechanism objects",                      /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    NPyMechObj_methods,                       /* tp_methods */
    NPyMechObj_members,                       /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)NPyMechObj_init,                /* tp_init */
    0,                                        /* tp_alloc */
    NPyMechObj_new,                           /* tp_new */
};

static PyTypeObject nrnpy_SegOfSecIterType = {
    PyObject_HEAD_INIT(NULL)0,                /*ob_size*/
    ccast "nrn.SegOfSecIter",                 /*tp_name*/
    sizeof(NPySegOfSecIter),                 /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)NPySegOfSecIter_dealloc,      /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    ccast "Iterate over nonzero area Segments of a Section (does not include x=0 or 1)", /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    (getiterfunc)PyObject_SelfIter,           /* tp_iter */
    (iternextfunc)seg_of_sec_next,            /* tp_iternext */
    0,                                        /* tp_methods */
    0,                                        /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    0,                                        /* tp_new */
};

static PyTypeObject nrnpy_MechOfSegIterType = {
    PyObject_HEAD_INIT(NULL)0,                /*ob_size*/
    ccast "nrn.MechOfSegIter",                /*tp_name*/
    sizeof(NPyMechOfSegIter),                /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)NPyMechOfSegIter_dealloc,     /*tp_dealloc*/
    0,                                        /*tp_print*/
    0,                                        /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    ccast "Iterate over Mechanisms in a Segment of a Section", /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    (getiterfunc)PyObject_SelfIter,           /* tp_iter */
    (iternextfunc)mech_of_seg_next,           /* tp_iternext */
    0,                                        /* tp_methods */
    0,                                        /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    0,                                        /* tp_new */
};


