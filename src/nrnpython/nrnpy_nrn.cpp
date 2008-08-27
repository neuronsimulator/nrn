#include <nrnpython.h>
#include <structmember.h>
#include <InterViews/resource.h>
#include <nrnoc2iv.h>

extern "C" {
#include <membfunc.h>
#include <parse.h>
extern void sec_free(hoc_Item*);
extern Symlist* hoc_built_in_symlist;
double* nrnpy_rangepointer(Section*, Symbol*, double, int*);

typedef struct {
	PyObject_HEAD
	Section* sec_;
	int allseg_iter_;
} NPySecObj;

typedef struct {
	PyObject_HEAD
	NPySecObj* pysec_;
	double x_;
	int alliter_;
} NPySegObj;

typedef struct {
	PyObject_HEAD
	NPySegObj* pyseg_;
	Prop* prop_;
	int first_iter_;
}NPyMechObj;
	
typedef struct {
	PyObject_HEAD
	NPyMechObj* pymech_;
	int index_;
}NPyRVItr;
	
typedef struct {
	PyObject_HEAD
	NPySegObj* pyseg_;
	Symbol* sym_;
}NPyRangeVar;

static PyTypeObject* psection_type;
static PyTypeObject* psegment_type;
static PyTypeObject* pmech_generic_type;
static PyTypeObject* range_type;
static PyObject* pmech_types;

extern Section* nrnpy_newsection(NPySecObj*);
extern void simpleconnectsection();
extern void nrn_change_nseg(Section*, int);
extern double section_length(Section*);
extern double nrn_ra(Section*);
extern int can_change_morph(Section*);
extern void nrn_length_change(Section*, double);
extern int diam_changed;
extern void mech_insert1(Section*, int);
extern PyObject* nrn_hocobj_ptr(double*);
extern PyObject* nrnpy_forall(PyObject* self, PyObject* args);
extern Symbol* nrnpy_pyobj_sym_;
extern PyObject* nrnpy_hoc2pyobject(Object*);
static void nrnpy_reg_mech(int);
extern void (*nrnpy_reg_mech_p_)(int);
static void o2loc(Object*, Section**, double*);
extern void (*nrnpy_o2loc_p_)(Object*, Section**, double*);
static void nrnpy_unreg_mech(int);


static void NPySecObj_dealloc(NPySecObj* self) {
//printf("NPySecObj_dealloc %lx\n", (long)self);
	if (self->sec_) {
		self->sec_->prop->dparam[9]._pvoid = 0;
		if (!self->sec_->prop->dparam[0].sym) {
			sec_free(self->sec_->prop->dparam[8].itm);
		}else{
			section_unref(self->sec_);
		}
	}
	self->ob_type->tp_free((PyObject*)self);
}

static void NPySegObj_dealloc(NPySegObj* self) {
//printf("NPySegObj_dealloc %lx\n", (long)self);
	Py_XDECREF(self->pysec_);
	self->ob_type->tp_free((PyObject*)self);
}

static void NPyRangeVar_dealloc(NPyRangeVar* self) {
//printf("NPyRangeVar_dealloc %lx\n", (long)self);
	Py_XDECREF(self->pyseg_);
	self->ob_type->tp_free((PyObject*)self);
}

static void NPyMechObj_dealloc(NPyMechObj* self) {
//printf("NPyMechObj_dealloc %lx %s\n", (long)self, self->ob_type->tp_name);
	Py_XDECREF(self->pyseg_);
	self->ob_type->tp_free((PyObject*)self);
}

// A new (or inited) python Section object with no arg creates a nrnoc section
// which persists only while the python object exists. The nrnoc section structure
// has no hoc Symbol but the Python Section pointer is filled in
// and secname(sec) returns the Python Section object name from the hoc section.
// If a Python Section object is created from an existing nrnoc section
// (with a filled in Symbol) the nrnoc section will continue to exist as long
// as 
// 
PyObject* NPySecObj_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
	NPySecObj* self;
	self = (NPySecObj*)type->tp_alloc(type, 0);
//printf("NPySecObj_new %lx\n", (long)self);
	if (self != NULL) {
		self->sec_ = nrnpy_newsection(self);
		self->allseg_iter_ = 0;
	}
	return (PyObject*)self;
}

PyObject* nrnpy_newsecobj(PyObject* args, PyObject* kwds) {
	return NPySecObj_new(psection_type, args, kwds);
}

static PyObject* NPySegObj_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
	NPySecObj* pysec;
	double x;
	if (!PyArg_ParseTuple(args, "O!d", psection_type, &pysec, &x)){
		return NULL;
	}
	if (x > 1.0 && x < 1.0001) { x = 1.0; }
	if (x < 0. || x > 1.0) {
		PyErr_SetString(PyExc_ValueError,
			"segment position range is 0 <= x <= 1");
		return NULL;
	}
	NPySegObj* self;
	self = (NPySegObj*)type->tp_alloc(type, 0);
//printf("NPySegObj_new %lx\n", (long)self);
	if (self != NULL) {
		self->pysec_ = pysec;
		self->x_ = x;
		Py_INCREF(self->pysec_);
	}
	return (PyObject*)self;
}

static PyObject* NPyMechObj_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
	NPySegObj* pyseg;
	if (!PyArg_ParseTuple(args, "O!", psegment_type, &pyseg)) {
		return NULL;
	}
	NPyMechObj* self;
	self = (NPyMechObj*)type->tp_alloc(type, 0);
//printf("NPyMechObj_new %lx %s\n", (long)self, self->ob_type->tp_name);
	if (self != NULL) {
		self->pyseg_ = pyseg;
		Py_INCREF(self->pyseg_);
	}
	return (PyObject*)self;
}

static PyObject* NPyRangeVar_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
	NPyRangeVar* self;
	self = (NPyRangeVar*)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->pyseg_ = NULL;
		self->sym_ = NULL;
	}
	return (PyObject*)self;
}

static int NPySecObj_init(NPySecObj* self, PyObject* args, PyObject* kwds) {
//printf("NPySecObj_init %lx %lx\n", (long)self, (long)self->sec_);
	if (self != NULL) {
		self->sec_ = nrnpy_newsection(self);
	}
	return 0;
}

static int NPySegObj_init(NPySegObj* self, PyObject* args, PyObject* kwds) {
//printf("NPySegObj_init %lx %lx\n", (long)self, (long)self->pysec_);
	NPySecObj* pysec;
	double x;
	if (!PyArg_ParseTuple(args, "O!d", psection_type, &pysec, &x)) {
		return -1;
	}
	if (x > 1.0 && x < 1.0001) { x = 1.0; }
	if (x < 0. || x > 1.0) {
		PyErr_SetString(PyExc_ValueError,
			"segment position range is 0 <= x <= 1");
		return -1;
	}
	Py_INCREF(pysec);
	if (self->pysec_) {
		Py_DECREF(self->pysec_);
	}
	self->pysec_ = pysec;
	self->x_ = x;
	return 0;
}

static void o2loc(Object* o, Section** psec, double* px) {
	if (o->ctemplate->sym != nrnpy_pyobj_sym_) {
		hoc_execerror("not a Python nrn.Segment", 0);
	}
	PyObject* po = nrnpy_hoc2pyobject(o);
	if (!PyObject_TypeCheck(po, psegment_type)) {
		hoc_execerror("not a Python nrn.Segment", 0);
	}
	NPySegObj* pyseg = (NPySegObj*)po;
	*psec = pyseg->pysec_->sec_;
	*px =  pyseg->x_;
}

static int NPyMechObj_init(NPyMechObj* self, PyObject* args, PyObject* kwds) {
//printf("NPyMechObj_init %lx %lx %s\n", (long)self, (long)self->pyseg_, self->ob_type->tp_name);
	NPySegObj* pyseg;
	if (!PyArg_ParseTuple(args, "O!", psegment_type, &pyseg)) {
		return -1;
	}
	Py_INCREF(pyseg);
	Py_XDECREF(self->pyseg_);
	self->pyseg_ = pyseg;
	return 0;
}

static int NPyRangeVar_init(NPyRangeVar* self, PyObject* args, PyObject* kwds) {
	return 0;
}

static PyObject* NPySecObj_name(NPySecObj* self) {
	PyObject* result;
	result = PyString_FromString(secname(self->sec_));
	return result;
}

static PyObject* NPyMechObj_name(NPyMechObj* self) {
	PyObject* result = NULL;
	if (self->prop_) {
		result = PyString_FromString(memb_func[self->prop_->type].sym->name);
	}
	return result;
}

static PyObject* NPyRangeVar_name(NPyRangeVar* self) {
	PyObject* result = NULL;
	if (self->sym_) {
		result = PyString_FromString(self->sym_->name);
	}
	return result;
}

static PyObject* NPySecObj_connect(NPySecObj* self, PyObject*  args) {
	NPySecObj* parent;
	double parentx, childend;
	parentx = 1.; childend = 0.;
	if (!PyArg_ParseTuple(args, "O!|dd", psection_type, &parent, &parentx, &childend)) {
		return NULL;
	}
//printf("NPySecObj_connect %s %g %g\n", parent, parentx, childend);
	if (parentx > 1. || parentx < 0.) {
		PyErr_SetString(PyExc_ValueError, "out of range 0 <= parentx <= 1.");
		return NULL;
	}
	if (childend != 0. && childend != 1.) {
		PyErr_SetString(PyExc_ValueError, "child connection end must be  0 or 1");
		return NULL;
	}
	Py_INCREF(self);
	hoc_pushx(childend);
	hoc_pushx(parentx);
	nrn_pushsec(self->sec_);
	nrn_pushsec(parent->sec_);
	simpleconnectsection();
	return (PyObject*)self;
}

static PyObject* NPySecObj_insert(NPySecObj* self, PyObject*  args) {
	char* tname;
	if (!PyArg_ParseTuple(args, "s", &tname)) {
		return NULL;
	}
	PyObject* otype = PyDict_GetItemString(pmech_types, tname);
	if (!otype) {
		PyErr_SetString(PyExc_ValueError, "argument not a density mechanism name.");
		return NULL;
	}
	int type = PyInt_AsLong(otype);
//printf("NPySecObj_insert %s %d\n", tname, type);
	mech_insert1(self->sec_, type);
	Py_INCREF(self);
	return (PyObject*)self;
}

PyObject* nrnpy_pushsec(PyObject* sec) {
	if (PyObject_TypeCheck(sec, psection_type)) {
		nrn_pushsec(((NPySecObj*)sec)->sec_);
		return sec;
	}
	return NULL;
}

static PyObject* NPySecObj_push(NPySecObj* self, PyObject*  args) {
	nrn_pushsec(self->sec_);
	Py_INCREF(self);
	return (PyObject*)self;
}

static PyObject* section_iter(NPySecObj* self) {
	//printf("section_iter %d\n", self->allseg_iter_);
	NPySegObj* seg;
	seg = PyObject_New(NPySegObj, psegment_type);
	if (seg == NULL) {
		return NULL;
	}
	seg->x_ = -1.;
	seg->alliter_ = self->allseg_iter_;
	self->allseg_iter_ = 0;
	seg->pysec_ = self;
	Py_INCREF(self);
	return (PyObject*)seg;
}

static PyObject* allseg(NPySecObj* self, PyObject* args) {
	//printf("allseg\n");
	Py_INCREF(self);
	self->allseg_iter_ = 1;
	return (PyObject*)self;
}

static PyObject* segment_iter(NPySegObj* self) {
	NPyMechObj* m = NULL;
	Node* nd = node_exact(self->pysec_->sec_, self->x_);
	Prop* p = nd->prop;
	m = PyObject_New(NPyMechObj, pmech_generic_type);
	for(;;) {
		if (!p) {
			break;
		}
//printf("segment_iter %d %s\n", p->type, memb_func[p->type].sym->name);
		if (PyDict_GetItemString(pmech_types, memb_func[p->type].sym->name)){
//printf("segment_iter found\n");
			break;
		}
		p = p->next;
	}
	m->pyseg_ = self;
	Py_INCREF(m->pyseg_);
	m->prop_ = p;
	m->first_iter_ = 1;
	return (PyObject*)m;
}

static PyObject* section_getattro(NPySecObj* self, PyObject* name) {
	Py_INCREF(name);
	char* n = PyString_AsString(name);
//printf("section_getattr %s\n", n);
	PyObject* result = 0;
	if (strcmp(n, "L") == 0) {
		result = Py_BuildValue("d", section_length(self->sec_));
	}else if (strcmp(n, "Ra") == 0) {
		result = Py_BuildValue("d", nrn_ra(self->sec_));
	}else if (strcmp(n, "nseg") == 0) {
		result = Py_BuildValue("i", self->sec_->nnode - 1);
	}else{
		result = PyObject_GenericGetAttr((PyObject*)self, name);
	}
	Py_DECREF(name);
	return result;
}

static int section_setattro(NPySecObj* self, PyObject* name, PyObject* value) {
	int err = 0;
	Py_INCREF(name);
	char* n = PyString_AsString(name);
//printf("section_setattro %s\n", n);
	if (strcmp(n, "L") == 0) {
		double x;
		if (PyArg_Parse(value, "d", &x) == 1 &&  x > 0.) {
			if (can_change_morph(self->sec_)) {
				self->sec_->prop->dparam[2].val = x;
				nrn_length_change(self->sec_, x);
				diam_changed = 1;
			}
		}else{
			PyErr_SetString(PyExc_ValueError,
				"L must be > 0.");
			err = -1;
		}
	}else if (strcmp(n, "Ra") == 0) {
		double x;
		if (PyArg_Parse(value, "d", &x) == 1 &&  x > 0.) {
			self->sec_->prop->dparam[7].val = x;
			diam_changed = 1;
		}else{
			PyErr_SetString(PyExc_ValueError,
				"Ra must be > 0.");
			err = -1;
		}
	}else if (strcmp(n, "nseg") == 0) {
		int nseg;
		if (PyArg_Parse(value, "i", &nseg) == 1 &&  nseg > 0 && nseg <= 32767) {
			nrn_change_nseg(self->sec_, nseg);
		}else{
			PyErr_SetString(PyExc_ValueError,
				"nseg must be in range 1 to 32767");
			err = -1;
		}
//printf("section_setattro err=%d nseg=%d nnode\n", err, nseg, self->sec_->nnode);
	}else{
		err = PyObject_GenericSetAttr((PyObject*)self, name, value);
	}
	Py_DECREF(name);
	return err;
}

static PyObject* segment_next(NPySegObj* self) {
//printf("segment_next enter x=%g\n", self->x_);
	double dx = 1./((double)self->pysec_->sec_->nnode-1);
	double x = self->x_;
	if (x < -1e-9) {
		if (self->alliter_) {
			x = 0.;
		}else{
			x = dx/2.;
		}
	}else if (x < 1e-9) {
		x = dx/2.;
	}else{
		x += dx;
	}
	if (x > 1. + dx - 1e-9) {
		return NULL;
	}
	if (x > 1. + 1e-9) {
		if (self->alliter_) {
			x = 1.0;
		}else{
			return NULL;
		}
	}
	self->x_ = x;
	Py_INCREF(self);
	return (PyObject*)self;
}

static PyObject* mech_next(NPyMechObj* self) {
	Prop* p = self->prop_;
	if (self->first_iter_) {
		self->first_iter_ = 0;
	}else{
		p = p->next;
	}
	NPyMechObj* m = NULL;
	for(;;) {
		if (!p) { return NULL; }
//printf("segment_iter %d %s\n", p->type, memb_func[p->type].sym->name);
		if (PyDict_GetItemString(pmech_types, memb_func[p->type].sym->name)) {
//printf("segment_iter found\n");
			m = PyObject_New(NPyMechObj, pmech_generic_type);
			break;
		}
		p = p->next;
	}
	if (m == NULL) { return NULL; }
	self->prop_  = p;
	m->pyseg_ = self->pyseg_;
	Py_INCREF(m->pyseg_);
	m->prop_ = p;
	return (PyObject*)m;
}

static void rv_noexist(Section* sec, const char* n, double x, int err) {
	char buf[200];
	if (err == 2) {
		sprintf(buf, "%s was not made to point to anything at %s(%g)", n, secname(sec), x);
	}else if (err == 1) {
		sprintf(buf, "%s, the mechanism does not exist at %s(%g)", n, secname(sec), x);
	}else{
		sprintf(buf, "%s does not exist at %s(%g)", n, secname(sec), x);
	}
	PyErr_SetString(PyExc_NameError, buf);
}

static PyObject* segment_getattro(NPySegObj* self, PyObject* name) {
	Symbol* sym;
	Py_INCREF(name);
	char* n = PyString_AsString(name);
//printf("segment_getattr %s\n", n);
	PyObject* result = 0;
	PyObject* otype;
	if (strcmp(n, "v") == 0) {
		Node* nd = node_exact(self->pysec_->sec_, self->x_);
		result = Py_BuildValue("d", NODEV(nd));
	}else if ((otype = PyDict_GetItemString(pmech_types, n)) != NULL) {
		int type = PyInt_AsLong(otype);
//printf("segment_getattr type=%d\n", type);
		Node* nd = node_exact(self->pysec_->sec_, self->x_);
		Prop* p = nrn_mechanism(type, nd);
		if (!p) {
			rv_noexist(self->pysec_->sec_, n, self->x_, 1);
			Py_DECREF(name);
			return NULL;
		}	
		NPyMechObj* m = PyObject_New(NPyMechObj, pmech_generic_type);
		if (m == NULL) {
			Py_DECREF(name);
			return NULL;
		}
		m->pyseg_ = self;
		m->prop_ = p;
		Py_INCREF(m->pyseg_);
		result = (PyObject*)m;
	}else if ((sym = hoc_table_lookup(n, hoc_built_in_symlist)) != 0 && sym->type == RANGEVAR) {
		if (ISARRAY(sym)) {
			NPyRangeVar* r = PyObject_New(NPyRangeVar, range_type);
			r->pyseg_ = self;
			Py_INCREF(r->pyseg_);
			r->sym_ = sym;
			result = (PyObject*)r;
		}else{
			int err;
			double* d = nrnpy_rangepointer(self->pysec_->sec_, sym, self->x_, &err);
			if (!d) {
				rv_noexist(self->pysec_->sec_, n, self->x_, err);
				result = NULL;
			}else{
				result = Py_BuildValue("d", *d);
			}
		}
	}else if (strncmp(n, "_ref_", 5) == 0) {
		if (strcmp(n+5, "v") == 0) {
			Node* nd = node_exact(self->pysec_->sec_, self->x_);
			result = nrn_hocobj_ptr(&(NODEV(nd)));
		}else if ((sym = hoc_table_lookup(n+5, hoc_built_in_symlist)) != 0 && sym->type == RANGEVAR) {
			if (ISARRAY(sym)) {
				NPyRangeVar* r = PyObject_New(NPyRangeVar, range_type);
				r->pyseg_ = self;
				Py_INCREF(r->pyseg_);
				r->sym_ = sym;
				result = (PyObject*)r;
			}else{
				int err;
				double* d = nrnpy_rangepointer(self->pysec_->sec_, sym, self->x_, &err);
				if (!d) {
					rv_noexist(self->pysec_->sec_, n+5, self->x_, err);
					result = NULL;
				}else{
					result = nrn_hocobj_ptr(d);
				}
			}
		}else{
			rv_noexist(self->pysec_->sec_, n, self->x_, 2);
			result = NULL;
		}
	}else{
		result = PyObject_GenericGetAttr((PyObject*)self, name);
	}
	Py_DECREF(name);
	return result;
}

static int segment_setattro(NPySegObj* self, PyObject* name, PyObject* value) {
	Symbol* sym;
	int err = 0;
	Py_INCREF(name);
	char* n = PyString_AsString(name);
//printf("segment_setattro %s\n", n);
	if (strcmp(n, "x") == 0) {
		int nseg;
		double x;
		if (PyArg_Parse(value, "d", &x) == 1 &&  x > 0. && x <= 1.) {
			if (x < 1e-9) {
				self->x_ = 0.;
			}else if (x > 1. - 1e-9) {
				self->x_ = 1.;
			}else{
				self->x_ = x;
			}
		}else{
			PyErr_SetString(PyExc_ValueError,
				"x must be in range 0. to 1.");
			err = -1;
		}
	}else if ((sym = hoc_table_lookup(n, hoc_built_in_symlist)) != 0 && sym->type == RANGEVAR) {
		if (ISARRAY(sym)) {
			assert(0);
		}else{
			int errp;
			double* d = nrnpy_rangepointer(self->pysec_->sec_, sym, self->x_, &errp);
			if (!d) {
				rv_noexist(self->pysec_->sec_, n, self->x_, errp);
				return -1;
			}
			if (!PyArg_Parse(value, "d", d)) {
				PyErr_SetString(PyExc_ValueError, "bad value");
				err = -1;
			}
		}
	}else{
		err = PyObject_GenericSetAttr((PyObject*)self, name, value);
	}
	Py_DECREF(name);
	return err;
}

static PyObject* mech_getattro(NPyMechObj* self, PyObject* name) {
	Py_INCREF(name);
	char* n = PyString_AsString(name);
//printf("mech_getattro %s\n", n);
	PyObject* result = 0;
	NrnProperty np(self->prop_);	
	char buf[200];
	int isptr = (strncmp(n, "_ref_", 5) == 0);
	sprintf(buf, "%s_%s", isptr?n+5:n, memb_func[self->prop_->type].sym->name);
	Symbol* sym = np.find(buf);
	if (sym) {
//printf("mech_getattro sym %s\n", sym->name);
		double* px = np.prop_pval(sym, 0);
		if (isptr) {
			result = nrn_hocobj_ptr(px);
		}else{
			result = Py_BuildValue("d", *px);
		}
	}else{
		result = PyObject_GenericGetAttr((PyObject*)self, name);
	}
	Py_DECREF(name);
	return result;
}

static int mech_setattro(NPyMechObj* self, PyObject* name, PyObject* value) {
	int err = 0;
	Py_INCREF(name);
	char* n = PyString_AsString(name);
//printf("mech_setattro %s\n", n);
	NrnProperty np(self->prop_);	
	char buf[200];
	sprintf(buf, "%s_%s", n, memb_func[self->prop_->type].sym->name);
	Symbol* sym = np.find(buf);
	if (sym) {
		double x;
		if (PyArg_Parse(value, "d", &x) == 1) {
			*np.prop_pval(sym, 0) = x;
		}else{
			PyErr_SetString(PyExc_ValueError,
				"must be a double");
			err = -1;
		}
	}else{
		err = PyObject_GenericSetAttr((PyObject*)self, name, value);
	}
	Py_DECREF(name);
	return err;
}

static PyObject* NPySecObj_call(NPySecObj* self, PyObject* args) {
	double x = 0.5;
	PyArg_ParseTuple(args, "|d", &x);
	PyObject* segargs = Py_BuildValue("(O,d)", self, x);
	PyObject* seg = NPySegObj_new(psegment_type, segargs, 0);
	return seg;
}

static Py_ssize_t rv_len(PyObject* self) {
	NPyRangeVar* r = (NPyRangeVar*)self;
	assert(r->sym_ && r->sym_->arayinfo);
	assert(r->sym_->arayinfo->nsub == 1);
	return r->sym_->arayinfo->sub[0];
}
static PyObject* rv_getitem(PyObject* self, Py_ssize_t ix) {
	NPyRangeVar* r = (NPyRangeVar*)self;
	PyObject* result = NULL;
	if (ix < 0 || ix >= rv_len(self)) {
		PyErr_SetString(PyExc_IndexError, r->sym_->name);
		return NULL;
	}
	int err;
	double* d = nrnpy_rangepointer(r->pyseg_->pysec_->sec_, r->sym_, r->pyseg_->x_, &err);
	if (!d) {
		rv_noexist(r->pyseg_->pysec_->sec_, r->sym_->name, r->pyseg_->x_, err);
		return NULL;
	}
	d += ix;
	result = Py_BuildValue("d", *d);
	return result;
}
static int rv_setitem(PyObject* self, Py_ssize_t ix, PyObject* value) {
	NPyRangeVar* r = (NPyRangeVar*)self;
	if (ix < 0 || ix >= rv_len(self)) {
		PyErr_SetString(PyExc_IndexError, r->sym_->name);
		return -1;
	}
	int err;
	double* d = nrnpy_rangepointer(r->pyseg_->pysec_->sec_, r->sym_, r->pyseg_->x_, &err);
	if (!d) {
		rv_noexist(r->pyseg_->pysec_->sec_, r->sym_->name, r->pyseg_->x_, err);
		return -1;
	}
	d += ix;
	if (!PyArg_Parse(value, "d", d)) {
		PyErr_SetString(PyExc_ValueError, "bad value");
		return -1;
	}
	return 0;
}

static PyMethodDef NPySecObj_methods[] = {
	{"name", (PyCFunction)NPySecObj_name, METH_NOARGS,
	 "Section name (same as hoc secname())"
	},
	{"connect", (PyCFunction)NPySecObj_connect, METH_VARARGS,
	 "childSection.connect(parentSection, [parentX], [childEnd])"
	},
	{"insert", (PyCFunction)NPySecObj_insert, METH_VARARGS,
	 "section.insert(densityMechanismType) e.g. soma.insert(hh)"
	},
	{"push", (PyCFunction)NPySecObj_push, METH_VARARGS,
	 "section.push() makes it the currently accessed section. Should end with a corresponding hoc.pop_section()"
	},
	{"allseg", (PyCFunction)allseg, METH_VARARGS,
	 "iterate over segments. Includes x=0 and x=1 zero-area nodes in the iteration."
	},
	{NULL}
};

static PyMethodDef NPySegObj_methods[] = {
	{NULL}
};

static PyMemberDef NPySegObj_members[] = {
	{"x", T_DOUBLE, offsetof(NPySegObj, x_), 0, 
	 "location in the section (segment containing x)"},
	{"sec", T_OBJECT_EX, offsetof(NPySegObj, pysec_), 0, "Section"},
	{NULL}
};

static PyMethodDef NPyMechObj_methods[] = {
	{"name", (PyCFunction)NPyMechObj_name, METH_NOARGS,
	 "Mechanism name (same as hoc suffix for density mechanism)"
	},
	{NULL}
};

static PyMethodDef NPyRangeVar_methods[] = {
	{"name", (PyCFunction)NPyRangeVar_name, METH_NOARGS,
	 "Range variable array name"
	},
	{NULL}
};

static PyMemberDef NPyMechObj_members[] = {
	{NULL}
};

static PySequenceMethods rv_seqmeth = {
	rv_len, NULL, NULL, rv_getitem,
	NULL, rv_setitem, NULL, NULL,
	NULL, NULL
};

static PyTypeObject nrnpy_SectionType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "nrn.Section",         /*tp_name*/
    sizeof(NPySecObj), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)NPySecObj_dealloc,                        /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    (ternaryfunc)NPySecObj_call,                         /*tp_call*/
    0,                         /*tp_str*/
    (getattrofunc)section_getattro,                         /*tp_getattro*/
    (setattrofunc)section_setattro,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Section objects",         /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    (getiterfunc)section_iter,		               /* tp_iter */
    0,		               /* tp_iternext */
    NPySecObj_methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)NPySecObj_init,      /* tp_init */
    0,                         /* tp_alloc */
    NPySecObj_new,                 /* tp_new */
};

static PyTypeObject nrnpy_SegmentType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "nrn.Segment",         /*tp_name*/
    sizeof(NPySegObj), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)NPySegObj_dealloc,                        /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    (getattrofunc)segment_getattro,                         /*tp_getattro*/
    (setattrofunc)segment_setattro,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Segment objects",         /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    (getiterfunc)segment_iter,		               /* tp_iter */
    (iternextfunc)segment_next,		               /* tp_iternext */
    NPySegObj_methods,             /* tp_methods */
    NPySegObj_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)NPySegObj_init,      /* tp_init */
    0,                         /* tp_alloc */
    NPySegObj_new,                 /* tp_new */
};

static PyTypeObject nrnpy_RangeType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "nrn.RangeVar",         /*tp_name*/
    sizeof(NPyRangeVar), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)NPyRangeVar_dealloc,                        /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    &rv_seqmeth,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Range Variable Array objects",         /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    NPyRangeVar_methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)NPyRangeVar_init,      /* tp_init */
    0,                         /* tp_alloc */
    NPyRangeVar_new,                 /* tp_new */
};

static PyTypeObject nrnpy_MechanismType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "nrn.Mechanism",         /*tp_name*/
    sizeof(NPyMechObj), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)NPyMechObj_dealloc,                        /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    (getattrofunc)mech_getattro,                         /*tp_getattro*/
    (setattrofunc)mech_setattro,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Mechanism objects",         /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    (iternextfunc)mech_next,		               /* tp_iternext */
    NPyMechObj_methods,             /* tp_methods */
    NPyMechObj_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)NPyMechObj_init,      /* tp_init */
    0,                         /* tp_alloc */
    NPyMechObj_new,                 /* tp_new */
};

PyObject* nrnpy_cas(PyObject* self, PyObject* args) {
	Section* sec = chk_access();
	section_ref(sec);
	NPySecObj* pysec = NULL;
	if (sec->prop->dparam[9]._pvoid) {
		pysec = (NPySecObj*)sec->prop->dparam[9]._pvoid;
		Py_INCREF(pysec);
		assert(pysec->sec_ == sec);
	}else{
		pysec = (NPySecObj*)psection_type->tp_alloc(psection_type, 0);
		pysec->sec_ = sec;
		sec->prop->dparam[9]._pvoid;
	}
	return (PyObject*)pysec;
}

static PyMethodDef nrnpy_methods[] = {
	{"cas", nrnpy_cas, METH_VARARGS, "Return the currently accessed section." },
	{"allsec", nrnpy_forall, METH_VARARGS, "Return iterator over all sections." },
	{NULL}
};
	
static PyObject* nrnmodule_;

myPyMODINIT_FUNC nrnpy_nrn(void) 
{
    int i;
    PyObject* m;

    psection_type = &nrnpy_SectionType;
    nrnpy_SectionType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&nrnpy_SectionType) < 0)
        return;
    Py_INCREF(&nrnpy_SectionType);

    psegment_type = &nrnpy_SegmentType;
    nrnpy_SegmentType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&nrnpy_SegmentType) < 0)
        return;
    Py_INCREF(&nrnpy_SegmentType);

    range_type = &nrnpy_RangeType;
    nrnpy_RangeType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&nrnpy_RangeType) < 0)
        return;
    Py_INCREF(&nrnpy_RangeType);

    m = Py_InitModule3("nrn", nrnpy_methods,
                       "NEURON interaction with Python");
    nrnmodule_ = m;	
    PyModule_AddObject(m, "Section", (PyObject *)psection_type);
    PyModule_AddObject(m, "Segment", (PyObject *)psegment_type);

#if 1
    pmech_generic_type = &nrnpy_MechanismType;
    nrnpy_MechanismType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&nrnpy_MechanismType) < 0)
        return;
    Py_INCREF(&nrnpy_MechanismType);
    PyModule_AddObject(m, "Mechanism", (PyObject *)pmech_generic_type);
    pmech_types = PyDict_New();
    for (i=4; i < n_memb_func; ++i) { // start at pas
	nrnpy_reg_mech(i);
    }
    nrnpy_reg_mech_p_ = nrnpy_reg_mech;
    nrnpy_o2loc_p_ = o2loc;
#endif
}

void nrnpy_reg_mech(int type) {
	int i;
	char* s;
	Memb_func* mf = memb_func + type;
	if (mf->is_point) { return; }
	if (!nrnmodule_) { return; }
	s = mf->sym->name;
//printf("nrnpy_reg_mech %s %d\n", s, type); 
	if (PyDict_GetItemString(pmech_types, s)) {
		hoc_execerror(s, "mechanism already exists");
	}
	Py_INCREF(&nrnpy_MechanismType);
	PyModule_AddObject(nrnmodule_, s, (PyObject *)pmech_generic_type);
	PyDict_SetItemString(pmech_types, s, Py_BuildValue("i", type));
}

void nrnpy_unreg_mech(int type) {
	// not implemented but needed when KSChan name changed.
}

} // end of extern c

