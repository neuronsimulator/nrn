static PyTypeObject nrnpy_SectionType = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT(NULL, 0)
	"nrn.Section",		/*tp_name*/
	sizeof(NPySecObj),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)NPySecObj_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)0,		/*tp_getattr*/
	(setattrfunc)0,		/*tp_setattr*/
	0,			/*tp_reserved*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	(hashfunc) pysec_hash, /*tp_hash*/
	(ternaryfunc)NPySecObj_call, /*tp_call*/
	0,			/*tp_str*/
	(getattrofunc)section_getattro, /*tp_getattro*/
	(setattrofunc)section_setattro,	/*tp_setattro*/
	0,			/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
	"Section objects",	/*tp_doc*/
	0,			/*tp_traverse*/
	0,			/*tp_clear*/
	(richcmpfunc) pysec_richcmp,	/*tp_richcompare*/
	0,			/*tp_weaklistoffset*/
	(getiterfunc)section_iter, /*tp_iter*/
	0,			/*tp_iternext*/
	NPySecObj_methods,	/*tp_methods*/
	0,			/*tp_members*/
	0,			/*tp_getset*/
	0,			/*tp_base*/
	0,			/*tp_dict*/
	0,			/*tp_descr_get*/
	0,			/*tp_descr_set*/
	0,			/*tp_dictoffset*/
	(initproc)NPySecObj_init, /*tp_init*/
	0,			/*tp_alloc*/
	NPySecObj_new,		/*tp_new*/
	0,			/*tp_free*/
	0,			/*tp_is_gc*/
};

static PyTypeObject nrnpy_AllsegIterType = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT(NULL, 0)
	"nrn.AllsegIter",		/*tp_name*/
	sizeof(NPyAllsegIter),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)NPyAllsegIter_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)0,		/*tp_getattr*/
	(setattrfunc)0,		/*tp_setattr*/
	0,			/*tp_reserved*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	(hashfunc)0, /*tp_hash*/
	(ternaryfunc)0, /*tp_call*/
	0,			/*tp_str*/
	(getattrofunc)0, /*tp_getattro*/
	(setattrofunc)0,	/*tp_setattro*/
	0,			/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
	"Iterate over all Segments of a Section, including x=0 and 1",	/*tp_doc*/
	0,			/*tp_traverse*/
	0,			/*tp_clear*/
	(richcmpfunc)0,	/*tp_richcompare*/
	0,			/*tp_weaklistoffset*/
	(getiterfunc)allseg_iter, /*tp_iter*/
	(iternextfunc)allseg_next,/*tp_iternext*/
	0,			/*tp_methods*/
	0,			/*tp_members*/
	0,			/*tp_getset*/
	0,			/*tp_base*/
	0,			/*tp_dict*/
	0,			/*tp_descr_get*/
	0,			/*tp_descr_set*/
	0,			/*tp_dictoffset*/
	(initproc)NPyAllsegIter_init, /*tp_init*/
	0,			/*tp_alloc*/
	NPyAllsegIter_new,	/*tp_new*/
	0,			/*tp_free*/
	0,			/*tp_is_gc*/
};

static PyTypeObject nrnpy_SegmentType = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT(NULL, 0)
	"nrn.Segment",		/*tp_name*/
	sizeof(NPySegObj),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)NPySegObj_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)0,		/*tp_getattr*/
	(setattrfunc)0,		/*tp_setattr*/
	0,			/*tp_reserved*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	(hashfunc)pyseg_hash,	/*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	(getattrofunc)segment_getattro, /*tp_getattro*/
	(setattrofunc)segment_setattro,	/*tp_setattro*/
	0,			/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
	"Segment objects",	/*tp_doc*/
	0,			/*tp_traverse*/
	0,			/*tp_clear*/
	(richcmpfunc) pyseg_richcmp,	/* tp_richcompare */
	0,			/*tp_weaklistoffset*/
	(getiterfunc)segment_iter, /*tp_iter*/
	(iternextfunc)segment_next, /*tp_iternext*/
	NPySegObj_methods,	/*tp_methods*/
	NPySegObj_members,	/*tp_members*/
	0,			/*tp_getset*/
	0,			/*tp_base*/
	0,			/*tp_dict*/
	0,			/*tp_descr_get*/
	0,			/*tp_descr_set*/
	0,			/*tp_dictoffset*/
	(initproc)NPySegObj_init, /*tp_init*/
	0,			/*tp_alloc*/
	NPySegObj_new,		/*tp_new*/
	0,			/*tp_free*/
	0,			/*tp_is_gc*/
};

static PyTypeObject nrnpy_RangeType = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT(NULL, 0)
	"nrn.RangeVar",		/*tp_name*/
	sizeof(NPyRangeVar),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)NPyRangeVar_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)0,		/*tp_getattr*/
	(setattrfunc)0,		/*tp_setattr*/
	0,			/*tp_reserved*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	&rv_seqmeth,		/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	0,			/*tp_getattro*/
	0,			/*tp_setattro*/
	0,			/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
	"Range Variable Array objects",	/*tp_doc*/
	0,			/*tp_traverse*/
	0,			/*tp_clear*/
	0,			/*tp_richcompare*/
	0,			/*tp_weaklistoffset*/
	0,			/*tp_iter*/
	0,			/*tp_iternext*/
	NPyRangeVar_methods,	/*tp_methods*/
	0,			/*tp_members*/
	0,			/*tp_getset*/
	0,			/*tp_base*/
	0,			/*tp_dict*/
	0,			/*tp_descr_get*/
	0,			/*tp_descr_set*/
	0,			/*tp_dictoffset*/
	(initproc)NPyRangeVar_init, /*tp_init*/
	0,			/*tp_alloc*/
	NPyRangeVar_new,	/*tp_new*/
	0,			/*tp_free*/
	0,			/*tp_is_gc*/
};

static PyTypeObject nrnpy_MechanismType = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT(NULL, 0)
	"nrn.Mechanism",		/*tp_name*/
	sizeof(NPyMechObj),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)NPyMechObj_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)0,		/*tp_getattr*/
	(setattrfunc)0,		/*tp_setattr*/
	0,			/*tp_reserved*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	(getattrofunc)mech_getattro, /*tp_getattro*/
	(setattrofunc)mech_setattro, /*tp_setattro*/
	0,			/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
	"Mechanism objects",	/*tp_doc*/
	0,			/*tp_traverse*/
	0,			/*tp_clear*/
	0,			/*tp_richcompare*/
	0,			/*tp_weaklistoffset*/
	0,			/*tp_iter*/
	(iternextfunc)mech_next, /*tp_iternext*/
	NPyMechObj_methods,	/*tp_methods*/
	NPyMechObj_members,	/*tp_members*/
	0,			/*tp_getset*/
	0,			/*tp_base*/
	0,			/*tp_dict*/
	0,			/*tp_descr_get*/
	0,			/*tp_descr_set*/
	0,			/*tp_dictoffset*/
	(initproc)NPyMechObj_init, /*tp_init*/
	0,			/*tp_alloc*/
	NPyMechObj_new,		/*tp_new*/
	0,			/*tp_free*/
	0,			/*tp_is_gc*/
};


static struct PyModuleDef nrnmodule = {
	PyModuleDef_HEAD_INIT,
	"nrn",
	"NEURON interaction with Python",
	-1,
	nrnpy_methods,
	NULL,
	NULL,
	NULL,
	NULL
};

/*
limited namespace version of nrn module which will not have the mechanism
names added. (At least one ModelDB model has a mechanism called 'cas')
*/
static struct PyModuleDef nrnsectionmodule = {
	PyModuleDef_HEAD_INIT,
	"_neuron_section",
	"NEURON interaction with Python",
	-1,
	nrnpy_methods,
	NULL,
	NULL,
	NULL,
	NULL
};


