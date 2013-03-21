static PyNumberMethods hocobj_as_number = {
	0,	//binaryfunc nb_add;
	0,	//binaryfunc nb_subtract;
	0,	//binaryfunc nb_multiply;
	0,	//binaryfunc nb_remainder;  
	0,	//binaryfunc nb_divmod;
	0,	//ternaryfunc nb_power;
	0,	//unaryfunc nb_negative;
	0,	//unaryfunc nb_positive;
	0,	//unaryfunc nb_absolute;
	(inquiry)hocobj_nonzero,	//inquiry nb_bool;
	0,	//unaryfunc nb_invert;
	0,	//binaryfunc nb_lshift;
	0,	//binaryfunc nb_rshift;
	0,	//binaryfunc nb_and;
	0,	//binaryfunc nb_xor;   
	0,	//binaryfunc nb_or;  
	0,	//unaryfunc nb_int;
	0,	//void *nb_reserved;  /* the slot formerly known as nb_long */
	0,	//unaryfunc nb_float;

	0,	//binaryfunc nb_inplace_add;     
	0,	//binaryfunc nb_inplace_subtract;
	0,	//binaryfunc nb_inplace_multiply;
	0,	//binaryfunc nb_inplace_remainder;
	0,	//ternaryfunc nb_inplace_power;
	0,	//binaryfunc nb_inplace_lshift;
	0,	//binaryfunc nb_inplace_rshift;
	0,	//binaryfunc nb_inplace_and;
	0,	//binaryfunc nb_inplace_xor;
	0,	//binaryfunc nb_inplace_or;

	0,	//binaryfunc nb_floor_divide;
	0,	//binaryfunc nb_true_divide;
	0,	//binaryfunc nb_inplace_floor_divide;
	0,	//binaryfunc nb_inplace_true_divide;

	0,	//unaryfunc nb_index;
};

static PyTypeObject nrnpy_HocObjectType = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT(NULL, 0)
	"hoc.HocObject",		/*tp_name*/
	sizeof(PyHocObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)hocobj_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)0,         /*tp_getattr*/
	(setattrfunc)0,		/*tp_setattr*/
	0,			/*tp_reserved*/
	0,			/*tp_repr*/
	&hocobj_as_number,	/*tp_as_number*/
	&hocobj_seqmeth,	/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	(hashfunc) hocobj_hash,			/*tp_hash*/
        (ternaryfunc)hocobj_call, /*tp_call*/
        0,                      /*tp_str*/
        hocobj_getattro,	/*tp_getattro*/
        hocobj_setattro,	/*tp_setattro*/
        0,			/*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
        hocobj_docstring,	/*tp_doc*/
        0,			/*tp_traverse*/
        0,			/*tp_clear*/
        (richcmpfunc) hocobj_richcmp,			/*tp_richcompare*/
        0,			/*tp_weaklistoffset*/
        &hocobj_iter,		/*tp_iter*/
        &hocobj_iternext,	/*tp_iternext*/
        hocobj_methods,		/*tp_methods*/
        0,			/*tp_members*/
        0,			/*tp_getset*/
        0,			/*tp_base*/
        0,			/*tp_dict*/
        0,			/*tp_descr_get*/
        0,			/*tp_descr_set*/
        0,			/*tp_dictoffset*/
        (initproc)hocobj_init,	/*tp_init*/
        0,			/*tp_alloc*/
        hocobj_new,		/*tp_new*/
        0,			/*tp_free*/
        0,			/*tp_is_gc*/
};

static struct PyModuleDef hocmodule = {
	PyModuleDef_HEAD_INIT,
	"hoc",
	"HOC interaction with Python",
	-1,
	HocMethods,
	NULL,
	NULL,
	NULL,
	NULL
};
