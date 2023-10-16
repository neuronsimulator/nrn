static PyType_Slot nrnpy_SectionType_slots[] = {
    {Py_tp_dealloc, (void*) NPySecObj_dealloc},
    {Py_tp_repr, (void*) pysec_repr},
    {Py_tp_hash, (void*) pysec_hash},
    {Py_tp_call, (void*) NPySecObj_call},
    {Py_tp_getattro, (void*) section_getattro},
    {Py_tp_setattro, (void*) section_setattro},
    {Py_tp_richcompare, (void*) pysec_richcmp},
    {Py_tp_iter, (void*) seg_of_section_iter},
    {Py_tp_methods, (void*) NPySecObj_methods},
    {Py_tp_init, (void*) NPySecObj_init},
    {Py_tp_new, (void*) NPySecObj_new},
    {Py_tp_doc, (void*) "Section objects"},
    {Py_sq_contains, (void*) NPySecObj_contains},
    {0, 0},
};
static PyType_Spec nrnpy_SectionType_spec = {
    "nrn.Section",
    sizeof(NPySecObj),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    nrnpy_SectionType_slots,
};


static PyType_Slot nrnpy_AllSegOfSecIterType_slots[] = {
    {Py_tp_dealloc, (void*) NPyAllSegOfSecIter_dealloc},
    {Py_tp_iter, (void*) allseg_of_sec_iter},
    {Py_tp_iternext, (void*) allseg_of_sec_next},
    {Py_tp_init, (void*) NPyAllSegOfSecIter_init},
    {Py_tp_new, (void*) NPyAllSegOfSecIter_new},
    {Py_tp_doc, (void*) "Iterate over all Segments of a Section, including x=0 and 1"},
    {0, 0},
};
static PyType_Spec nrnpy_AllSegOfSecIterType_spec = {
    "nrn.AllsegIter",
    sizeof(NPyAllSegOfSecIter),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    nrnpy_AllSegOfSecIterType_slots,
};

static PyType_Slot nrnpy_SegOfSecIterType_slots[] = {
    {Py_tp_dealloc, (void*) NPySegOfSecIter_dealloc},
    {Py_tp_iter, (void*) PyObject_SelfIter},
    {Py_tp_iternext, (void*) seg_of_sec_next},
    {Py_tp_doc,
     (void*) "Iterate over nonzero area Segments of a Section (does not include x=0 and 1)"},
    {0, 0},
};
static PyType_Spec nrnpy_SegOfSecIterType_spec = {
    "nrn.SegOfSecIter",
    sizeof(NPySegOfSecIter),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    nrnpy_SegOfSecIterType_slots,
};

static PyType_Slot nrnpy_SegmentType_slots[] = {
    {Py_tp_dealloc, (void*) NPySegObj_dealloc},
    {Py_tp_repr, (void*) pyseg_repr},
    {Py_tp_hash, (void*) pyseg_hash},
    {Py_tp_getattro, (void*) segment_getattro},
    {Py_tp_setattro, (void*) segment_setattro},
    {Py_tp_richcompare, (void*) pyseg_richcmp},
    {Py_tp_iter, (void*) mech_of_segment_iter},
    {Py_tp_methods, (void*) NPySegObj_methods},
    {Py_tp_members, (void*) NPySegObj_members},
    {Py_tp_init, (void*) NPySegObj_init},
    {Py_tp_new, (void*) NPySegObj_new},
    {Py_tp_doc, (void*) "Segment objects"},
    {0, 0},
};
static PyType_Spec nrnpy_SegmentType_spec = {
    "nrn.Segment",
    sizeof(NPySegObj),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    nrnpy_SegmentType_slots,
};

static PyType_Slot nrnpy_MechOfSegIterType_slots[] = {
    {Py_tp_dealloc, (void*) NPyMechOfSegIter_dealloc},
    {Py_tp_iter, (void*) PyObject_SelfIter},
    {Py_tp_iternext, (void*) mech_of_seg_next},
    {Py_tp_doc, (void*) "Iterate over Mechanisms in a Segment of a Section"},
    {0, 0},
};
static PyType_Spec nrnpy_MechOfSegIterType_spec = {
    "nrn.MechOfSegIter",
    sizeof(NPyMechOfSegIter),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    nrnpy_MechOfSegIterType_slots,
};

static PyType_Slot nrnpy_MechanismType_slots[] = {
    {Py_tp_dealloc, (void*) NPyMechObj_dealloc},
    {Py_tp_repr, (void*) pymech_repr},
    {Py_tp_getattro, (void*) mech_getattro},
    {Py_tp_setattro, (void*) mech_setattro},
    {Py_tp_iter, (void*) var_of_mech_iter},
    {Py_tp_methods, (void*) NPyMechObj_methods},
    {Py_tp_members, (void*) NPyMechObj_members},
    {Py_tp_init, (void*) NPyMechObj_init},
    {Py_tp_new, (void*) NPyMechObj_new},
    {Py_tp_doc, (void*) "Mechanism objects"},
    {0, 0},
};
static PyType_Spec nrnpy_MechanismType_spec = {
    "nrn.Mechanism",
    sizeof(NPyMechObj),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    nrnpy_MechanismType_slots,
};

static PyType_Slot nrnpy_MechFuncType_slots[] = {
    {Py_tp_dealloc, (void*) NPyMechFunc_dealloc},
    {Py_tp_repr, (void*) pymechfunc_repr},
    {Py_tp_methods, (void*) NPyMechFunc_methods},
    {Py_tp_call, (void*) NPyMechFunc_call},
    {Py_tp_doc, (void*) "Mechanism Function"},
    {0, 0},
};
static PyType_Spec nrnpy_MechFuncType_spec = {
    "nrn.MechFunc",
    sizeof(NPyMechFunc),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    nrnpy_MechFuncType_slots,
};

static PyType_Slot nrnpy_VarOfMechIterType_slots[] = {
    {Py_tp_dealloc, (void*) NPyVarOfMechIter_dealloc},
    {Py_tp_iter, (void*) PyObject_SelfIter},
    {Py_tp_iternext, (void*) var_of_mech_next},
    {Py_tp_doc, (void*) "Iterate over variables  in a Mechanism"},
    {0, 0},
};
static PyType_Spec nrnpy_VarOfMechIterType_spec = {
    "nrn.VarOfMechIter",
    sizeof(NPyVarOfMechIter),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    nrnpy_VarOfMechIterType_slots,
};

static PyType_Slot nrnpy_RangeType_slots[] = {
    {Py_tp_dealloc, (void*) NPyRangeVar_dealloc},
    {Py_tp_methods, (void*) NPyRangeVar_methods},
    {Py_tp_init, (void*) NPyRangeVar_init},
    {Py_tp_new, (void*) NPyRangeVar_new},
    {Py_tp_doc, (void*) "Range Variable Array objects"},
    {Py_sq_length, (void*) rv_len},
    {Py_sq_item, (void*) rv_getitem},
    {Py_sq_ass_item, (void*) rv_setitem},
    {0, 0},
};
static PyType_Spec nrnpy_RangeType_spec = {
    "nrn.RangeVar",
    sizeof(NPyRangeVar),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    nrnpy_RangeType_slots,
};

static struct PyModuleDef nrnmodule = {PyModuleDef_HEAD_INIT,
                                       "nrn",
                                       "NEURON interaction with Python",
                                       -1,
                                       nrnpy_methods,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL};

/*
limited namespace version of nrn module which will not have the mechanism
names added. (At least one ModelDB model has a mechanism called 'cas')
*/
static struct PyModuleDef nrnsectionmodule = {PyModuleDef_HEAD_INIT,
                                              "_neuron_section",
                                              "NEURON interaction with Python",
                                              -1,
                                              nrnpy_methods,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL};
