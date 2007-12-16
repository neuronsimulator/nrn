#include <nrnpython.h>
#include <structmember.h>
#include <InterViews/resource.h>
#include <nrnoc2iv.h>
#include <ocjump.h>
#include "ivocvect.h"

#ifdef WITH_NUMPY

// needed for C extension using NumPy
#define PY_ARRAY_UNIQUE_SYMBOL _nrnpy_arrayu


// Numeric Python header
#include "numpy/arrayobject.h"

#endif // WITH_NUMPY


extern "C" {

#include "parse.h"
extern void hoc_pushs(Symbol*);
extern double* hoc_evalpointer();
extern Symlist* hoc_top_level_symlist;
extern Symlist* hoc_built_in_symlist;
extern Inst* hoc_pc;
extern void hoc_push_string();
extern char** hoc_strpop();
extern void hoc_objectvar();
extern Object** hoc_objpop();
extern Object* hoc_obj_look_inside_stack(int);
extern void hoc_object_component();
extern int hoc_stack_type();
extern void hoc_call();
extern Objectdata* hoc_top_level_data;
extern void hoc_tobj_unref(Object**);
extern void sec_access_push();
extern boolean hoc_valid_stmt(const char*, Object*);
myPyMODINIT_FUNC nrnpy_nrn();
extern PyObject* nrnpy_cas(PyObject*, PyObject*);
extern int section_object_seen;
extern Symbol* nrnpy_pyobj_sym_;
extern PyObject* nrnpy_hoc2pyobject(Object*);
PyObject* nrnpy_ho2po(Object*);
Object* nrnpy_po2ho(PyObject*);
extern Object* nrnpy_pyobject_in_obj(PyObject*);
static void pyobject_in_objptr(Object**, PyObject*);
/*
Because python types have so many methods, attempt to do all set and get
using a PyHocObject which has different amounts filled in as the information
is gathered with a view toward ultimately making a call to hoc_object_component.
That requires that array indices or function arguments are on the stack.
The major variant is that we may be at the top level. The function arg
case is easy since they all come as a tuple in the call method. The
array indices come sequentially with a series of calls to the
sequence method. A nice side effect of intermediate objects is the extra
efficiency of reuse that avoids symbol lookup. Sadly, the scalar case does
not give this since the value is set/get instead of  returning the
intermediate.
*/
typedef struct {
	PyObject_HEAD
	Object* ho_;
	union {
		double x_;
		char* s_;
		Object* ho_;
	}u;
	Symbol* sym_; // for functions and arrays
	int nindex_; // number indices seen so far (or narg)
	int* indices_; // one fewer than nindex_
	int type_; // 0 HocTopLevelInterpreter, 1 HocObject, 2 function, 3 array
		// 4 reference to number
		// 5 reference to string
		// 6 reference to hoc object
} PyHocObject;

static PyTypeObject* hocobject_type;

static PyObject* nrnexec(PyObject* self, PyObject* args) {
	const char* cmd;
	if (!PyArg_ParseTuple(args, "s", &cmd)) {
		return NULL;
	}
	boolean b = hoc_valid_stmt(cmd, 0);
	return Py_BuildValue("i", b?1:0);
}

static  PyObject* hoc_ac(PyObject* self, PyObject* args) {
	PyArg_ParseTuple(args, "|d", &hoc_ac_);
	return Py_BuildValue("d", hoc_ac_);
}


static PyObject* test_numpy(PyObject* self, PyObject* args) {

	if (!PyArg_ParseTuple(args, "")) {
		return NULL;
	}

	#ifdef WITH_NUMPY

	PyArrayObject *po = NULL;
	int dims = 1;

	po = (PyArrayObject*)PyArray_FromDims(1,&dims,PyArray_DOUBLE);
	*(double*)(po->data) = 0.0;
	return Py_BuildValue("N",po);

	#else

	// return None
	return Py_BuildValue("");

	#endif // WITH_NUMPY



}



static PyMethodDef HocMethods[] = {
	{"execute", nrnexec, METH_VARARGS, "Execute a hoc command, return 1 on success, 0 on failure." },
	{"hoc_ac", hoc_ac, METH_VARARGS, "Get (or set) the scalar hoc_ac_." },

	{"test_numpy", test_numpy, METH_VARARGS, "If numpy support is available, returns an array containing 1 zero, otherwize None." },


	{NULL, NULL, 0, NULL}
};

static void hocobj_dealloc(PyHocObject* self) {
//printf("hocobj_dealloc %lx\n", (long)self);
	if (self->ho_) {
		hoc_obj_unref(self->ho_);
	}
	if (self->type_ == 5 && self->u.s_) {
		//delete [] self->u.s_;
		free(self->u.s_);
	}
	if (self->type_ == 6 && self->u.ho_) {
		hoc_obj_unref(self->u.ho_);
	}
	if (self->indices_) { delete [] self->indices_; }
	self->ob_type->tp_free((PyObject*)self);
}

static PyObject* hocobj_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
	PyHocObject* self;
	self = (PyHocObject*)type->tp_alloc(type, 0);
//printf("hocobj_new %lx\n", (long)self);
	if (self != NULL) {
		self->ho_ = NULL;
		self->u.x_ = 0.;
		self->sym_ = NULL;
		self->indices_ = NULL;
		self->nindex_ = 0;
		self->type_ = 0;
	}
	return (PyObject*)self;
}

static int hocobj_init(PyHocObject* self, PyObject* args, PyObject* kwds) {
//printf("hocobj_init %lx %lx\n", (long)self, (long)self->ho_);
	if (self != NULL) {
		if (self->ho_) { hoc_obj_unref(self->ho_); }
		self->ho_ = NULL;
		self->u.x_ = 0.;
		self->sym_ = NULL;
		self->indices_ = NULL;
		self->nindex_ = 0;
		self->type_ = 0;
	}
	return 0;
}

static void pyobject_in_objptr(Object** op, PyObject* po) {
	Object* o = nrnpy_pyobject_in_obj(po);
	if(*op) {
		hoc_obj_unref(*op);
	}
	*op = o;
}

#if defined(__MINGW32__)
#define ALT_fprintf sprintf
#define ALT_f (buf+strlen(buf))
#define ALT_ws PyFile_WriteString(buf, po)
#else
#define ALT_fprintf fprintf
#define ALT_f f
#define ALT_ws /**/
#endif

static int hocobj_print(PyHocObject* self, FILE* f, int i) {
#if defined(__MINGW32__)
	char buf[256];
	buf[0] = '\0';
	PyObject* po = PyFile_FromFile(f, "what",  "w", 0);
#endif
	// ALT_fprintf(ALT_f, "hocobj_print f=%lx i=%d\n", (long)f, i);
	if (f) {
		if (self->type_ == 1) {
			ALT_fprintf(ALT_f, "%s", hoc_object_name(self->ho_));
			ALT_ws;
		}else if (self->type_ == 2 || self->type_ == 3) {
			ALT_fprintf(ALT_f, "%s%s%s",
				self->ho_ ? hoc_object_name(self->ho_) : "",
				self->ho_ ? "." : "",
				self->sym_->name);
			ALT_ws;
			if (self->type_ == 3) {
				for (int i = 0; i < self->nindex_; ++i) {
					ALT_fprintf(ALT_f, "[%d]", self->indices_[i]);
					ALT_ws;
				}
				ALT_fprintf(ALT_f, "[?]");
				ALT_ws;
			}else{
				ALT_fprintf(ALT_f, "()");
				ALT_ws;
			}
		}else if (self->type_ == 4) {
			ALT_fprintf(ALT_f, "hoc ref value %g", self->u.x_);
			ALT_ws;
		}else if (self->type_ == 5) {
			ALT_fprintf(ALT_f, "hoc ref value \"%s\"", self->u.s_);
			ALT_ws;
		}else if (self->type_ == 6) {
			ALT_fprintf(ALT_f, "hoc ref value \"%s\"", hoc_object_name(self->u.ho_));
			ALT_ws;
		}else{
			ALT_fprintf(ALT_f, "TopLevelHocInterpreter");
			ALT_ws;
		}
	}
#if defined(__MINGW32__)
	Py_DECREF(po);
#endif
	return 0;
}

static Inst* save_pc(Inst* newpc) {
	Inst* savpc = hoc_pc;
	hoc_pc = newpc;
	return savpc;
}

static int hocobj_pushargs(PyObject* args) {
	int i, narg = PyTuple_Size(args);
	for (i=0; i < narg; ++i) {
		PyObject* po = PyTuple_GetItem(args, i);
//PyObject_Print(po, stdout, 0);
//printf("  pushargs %d\n", i);
		if (nrnpy_numbercheck(po)) {
			PyObject* pn = PyNumber_Float(po);
			hoc_pushx(PyFloat_AsDouble(pn));
			Py_XDECREF(pn);
		}else if (PyString_Check(po)) {
			char** ts = hoc_temp_charptr();
			*ts = PyString_AsString(po);
			hoc_pushstr(ts);
		}else if (PyObject_IsInstance(po, (PyObject*)hocobject_type)) {
			PyHocObject* pho = (PyHocObject*)po;
			int tp = pho->type_;
			if (tp == 1) {
				hoc_push_object(pho->ho_);
			}else if (tp == 4) {
				hoc_pushpx(&pho->u.x_);
			}else if (tp == 5) {
				hoc_pushstr(&pho->u.s_);
			}else if (tp == 6) {
				hoc_pushobj(&pho->u.ho_);
			}else{
				// make a hoc python object and push that
				Object* ob;
				ob = 0;
				pyobject_in_objptr(&ob, po);
				hoc_push_object(ob);
				hoc_obj_unref(ob);
			}
		}else{ // make a hoc PythonObject and push that?
			Object* ob;
			ob = 0;
			pyobject_in_objptr(&ob, po);
			hoc_push_object(ob);
			hoc_obj_unref(ob);
		}
	}
	return narg;
}

static Symbol* getsym(char* name, Object* ho, int fail) {
	Symbol* sym = 0;
	if (ho) {
		sym = hoc_table_lookup(name, ho->ctemplate->symtable);
	}else{
		sym = hoc_table_lookup(name, hoc_top_level_symlist);
		if (!sym) {
			sym = hoc_table_lookup(name, hoc_built_in_symlist);
		}
	}
	if (!sym && fail) {
		char e[200];
		sprintf(e, "'%s' is not a hoc variable name.", name);
		PyErr_SetString(PyExc_LookupError, e);
	}
	return sym;
}

// on entry the stack order is indices, args, object
// on exit all that is popped and the result is on the stack
static void component(PyHocObject* po) {
	Inst fc[6];
	fc[0].sym = po->sym_;
	fc[1].i = 0;
	fc[2].i = 0;
	fc[5].i = 0;
	if (po->type_ == 2) {
		fc[2].i = po->nindex_;
		fc[5].i = 1;
	}else if (po->type_ == 3) {
		fc[1].i = po->nindex_;
	}
	assert(hoc_obj_look_inside_stack(po->nindex_) == po->ho_);
	fc[3].i = po->ho_->ctemplate->id;
	fc[4].sym = po->sym_;
	Inst* pcsav = save_pc(fc);
	hoc_object_component();
	hoc_pc = pcsav;
}

int nrnpy_numbercheck(PyObject* po) {
	// PyNumber_Check can return 1 for things that are not numbers
	return (PyInt_CheckExact(po) || PyFloat_CheckExact(po)
		  || PyLong_CheckExact(po) || PyBool_Check(po));
}

PyObject* nrnpy_ho2po(Object* o) {
	// o may be NULLobject, or encapsulate a Python object (via
	// the PythonObject class in hoc (see Py2Nrn in nrnpy_p2h.cpp),
	// or be a native hoc class instance such as Graph.
	// The return value is None, or the encapsulated PyObject or
	// an encapsulating PyHocObject
	PyObject* po;
	if (!o) {
		po = Py_BuildValue("");
	}else if (o->ctemplate->sym == nrnpy_pyobj_sym_) {
		po = nrnpy_hoc2pyobject(o);
		Py_INCREF(po);
	}else{
		po = hocobj_new(hocobject_type, 0, 0);
		((PyHocObject*)po)->ho_ = o;
		((PyHocObject*)po)->type_ = 1;
		hoc_obj_ref(o);
	}
	return po;
}

Object* nrnpy_po2ho(PyObject* po) {
	// po may be None, or encapsulate a hoc object (via the
	// PyHocObject, or be a native Python instance such as [1,2,3]
	// The return value is None, or the encapsulated hoc object,
	// or a hoc object of type PythonObject that encapsulates the
	// po.
	Object* o;
	if (po == Py_None) {
		o = 0;
	}else if (PyObject_TypeCheck(po, hocobject_type)) {
		PyHocObject* pho = (PyHocObject*)po;
		// cases: 1 just a regular hoc object, 6 reference to hoc
		// object, all the rest encapsulate
		if (pho->type_ == 1) {
			o = pho->ho_;
			hoc_obj_ref(o);
		}else if (pho->type_ == 6) {
			o = pho->u.ho_;
			hoc_obj_ref(o);
		}else{
			o = nrnpy_pyobject_in_obj(po);
		}
	}else{ // even if Python string or number
		o = nrnpy_pyobject_in_obj(po);
	}
	return o;
}

PyObject* nrnpy_hoc_pop() {
	PyObject* result = 0;
	Object* ho;
	Object** d;
	switch (hoc_stack_type()) {
	case STRING:
		result = Py_BuildValue("s", *hoc_strpop());
		break;
	case VAR:
		result = Py_BuildValue("d", *hoc_pxpop());
		break;
	case NUMBER:
		result = Py_BuildValue("d", hoc_xpop());
		break;
	case OBJECTVAR:
	case OBJECTTMP:
		d = hoc_objpop();
		ho = *d;
//printf("Py2Nrn %lx %lx\n", (long)ho->ctemplate->sym, (long)nrnpy_pyobj_sym_);
		if (!ho) {
			result = Py_BuildValue("");
		}else if (ho->ctemplate->sym == nrnpy_pyobj_sym_) {
			result = nrnpy_hoc2pyobject(ho);
		}else{
			result = hocobj_new(hocobject_type, 0, 0);
			((PyHocObject*)result)->ho_ = ho;
			((PyHocObject*)result)->type_ = 1;
			hoc_obj_ref(ho);
		}
		hoc_tobj_unref(d);
		break;
	default:
		printf("nrnpy_hoc_pop error: stack type = %d\n", hoc_stack_type());
	}
	return result;
}

static int set_final_from_stk(PyObject* po) {
	int err = 0;
	switch (hoc_stack_type()) {
	case STRING:
		char* s;
		if (PyArg_Parse(po, "s", &s) == 1) {
			hoc_assign_str(hoc_strpop(), s);
		}else{
			err = 1;
		}
		break;
	case VAR:
		double x;
		if (PyArg_Parse(po, "d", &x) == 1) {
			*(hoc_pxpop()) = x;
		}else{
			err = 1;
		}
		break;
	case OBJECTVAR:
		PyHocObject* pho;
		if (PyArg_Parse(po, "O!", hocobject_type, &pho) == 1) {
			Object** pobj = hoc_objpop();
			if (pho->sym_) {
 PyErr_SetString(PyExc_TypeError, "argument cannot be a hoc object intermediate");
				return -1;
			}
			Object* ob = *pobj;
			hoc_obj_ref(pho->ho_);
			hoc_obj_unref(ob);
			*pobj = pho->ho_;
		}else{
			err = 1;
		}
		break;
	default:
		printf("set_final_from_stk() error: stack type = %d\n", hoc_stack_type());
		err = 1;
		break;
	}
	return err;
}

static void* fcall(void* vself, void* vargs) {
	PyHocObject* self = (PyHocObject*)vself;
	if (self->ho_) {
		hoc_push_object(self->ho_);
	}
	int narg = hocobj_pushargs((PyObject*)vargs);
	if (self->ho_) {
		self->nindex_ = narg;
		component(self);
		return (void*)nrnpy_hoc_pop();
	}
	if (self->sym_->type == BLTIN) {
		if (narg != 1) {
			hoc_execerror("must be one argument for", self->sym_->name);
		}
		double d = hoc_call_func(self->sym_, 1);
		hoc_pushx(d);
	}else{
		Inst fc[3];
		fc[0].sym = self->sym_;
		fc[1].i = narg;
		fc[2].in = STOP;
		Inst* pcsav = save_pc(fc);
		hoc_call();
		hoc_pc = pcsav;
	}
	return (void*)nrnpy_hoc_pop();
}

static PyObject* hocobj_call(PyHocObject* self, PyObject* args) {
	if (self->type_ == 0) {
		return nrnexec((PyObject*)self, args);
	}else if (self->type_ == 2) {
		OcJump* oj;
		oj = new OcJump();
		if (oj) {
  PyObject* result = (PyObject*)oj->fpycall(fcall, (void*)self, (void*)args);
			delete oj;
			if (result == NULL) {
  PyErr_SetString(PyExc_RuntimeError, "hoc error");
			}
			return result;
		}else{		
			return (PyObject*)fcall((void*)self, (void*)args);
		}
	}else{
		return NULL;
	}
}

static Arrayinfo* hocobj_aray(Symbol* sym, Object* ho) {
	if (!sym->arayinfo) { return 0; }
	if (ho) { // objectdata or not?
		int cplus = (ho->ctemplate->sym->subtype & (CPLUSOBJECT | JAVAOBJECT));
		if (cplus) {
			return sym->arayinfo;
		}else{
			return	ho->u.dataspace[sym->u.oboff + 1].arayinfo;
		}
	}else{
		if (sym->type == VAR && (sym->subtype == USERDOUBLE
		  || sym->subtype == USERINT || sym->subtype == USERFLOAT)) {
			return sym->arayinfo;
		}else{
			return hoc_top_level_data[sym->u.oboff + 1].arayinfo;
		}
	}
}

static PyHocObject* intermediate(PyHocObject* po, Symbol* sym , int ix) {
	PyHocObject* ponew = (PyHocObject*)hocobj_new(hocobject_type, 0, 0);
	if (po->ho_) {
		ponew->ho_ = po->ho_;
		hoc_obj_ref(po->ho_);
	}
	if (ix > -1) { // array, increase dimension by one
		int j;
		assert(po->sym_ == sym);
		assert(po->type_ == 3);
		ponew->sym_ = sym;
		ponew->nindex_ = po->nindex_ + 1;
		ponew->type_ = po->type_;
		ponew->indices_ = new int[ponew->nindex_];
		for (j = 0; j < po->nindex_; ++j) {
			ponew->indices_[j] = po->indices_[j];
		}
		ponew->indices_[po->nindex_] = ix;
	}else{ // make it an array (no indices yet)
		ponew->sym_ = sym;
		ponew->type_ = 3;
	}
	return ponew;
}

// when called, nindex is 1 less than reality
static void hocobj_pushtop(PyHocObject* po, Symbol* sym, int ix) {
	int i;
	int n = po->nindex_++;
//printf("hocobj_pushtop n=%d", po->nindex_);
	for (i=0; i < n; ++i) {
		hoc_pushx((double)po->indices_[i]);
//printf(" %d", po->indices_[i]);
	}
	hoc_pushx((double)ix);
//printf(" %d\n", ix);
	if (sym) {
		hoc_pushs(sym);
	}
}

static void hocobj_objectvar(Symbol* sym) {
	Inst fc;
	fc.sym = sym;
	Inst* pcsav = save_pc(&fc);
	hoc_objectvar();
	hoc_pc = pcsav;
}

static PyObject* hocobj_getsec(Symbol* sym) {
	Inst fc;
	fc.sym = sym;
	Inst* pcsav = save_pc(&fc);
	sec_access_push();
	hoc_pc = pcsav;
	PyObject* result = nrnpy_cas(0, 0);
	nrn_popsec();			
	return result;
}

// leave pointer on stack ready for get/set final
static void eval_component(PyHocObject* po, int ix) {
	int j;
	hoc_push_object(po->ho_);
	hocobj_pushtop(po, 0, ix);
	component(po);
	--po->nindex_;
}

static PyObject* hocobj_getattro(PyHocObject* self, PyObject* name) {
	Inst fc, *pcsav;
	Py_INCREF(name);
	char* n = PyString_AsString(name);
//printf("hocobj_getattro %s\n", n);

	PyObject* result = 0;

	// use below for casting
	PyHocObject* po;

	if (self->type_ == 1 && !self->ho_) {
		Py_DECREF(name);
		PyErr_SetString(PyExc_TypeError, "not a compound type");
		return NULL;
	}
	Symbol* sym = getsym(n, self->ho_, 0);
	Py_DECREF(name);
	if (!sym) {
	  // ipython wants to know if there is a __getitem__
	  // even though it does not use it.
	  return PyObject_GenericGetAttr((PyObject*)self, name);
	}
	if (self->ho_) { // use the component fork.
		result = hocobj_new(hocobject_type, 0, 0);
		PyHocObject* po = (PyHocObject*)result;
		po->ho_ = self->ho_;
		hoc_obj_ref(po->ho_);
		po->sym_ = sym;
		// evaluation deferred unless VAR,STRING,OBJECTVAR and not
		// an array
		int t = sym->type;
		if (t == VAR || t == STRING || t == OBJECTVAR || t == RANGEVAR || t == SECTION) {
			if (!ISARRAY(sym)) {
				hoc_push_object(po->ho_);
				component(po);
				hocobj_dealloc(po);
				if (t == SECTION) {
					section_object_seen = 0;
					result = nrnpy_cas(0,0);
					nrn_popsec();
					return result;
				}else{
					return nrnpy_hoc_pop();
				}
			}else{
				po->type_ = 3;
				return result;
			}
		}else{
			po->type_ = 2;
			return result;
		}
	}
	// top level interpreter fork
	switch (sym->type) {
	case VAR: // double*
		if (!ISARRAY(sym)) {
			hoc_pushs(sym);
			hoc_evalpointer();
			result = Py_BuildValue("d", *hoc_pxpop());
		}else{
			result = (PyObject*)intermediate(self, sym, -1);
		}
		break;
	case STRING: // char*
		fc.sym = sym;
		pcsav = save_pc(&fc);
		hoc_push_string();
		hoc_pc = pcsav;
		result = Py_BuildValue("s", *hoc_strpop());
		break;
	case OBJECTVAR: // Object*
		if (!ISARRAY(sym)) {
			fc.sym = sym;
			pcsav = save_pc(&fc);
			hoc_objectvar();
			hoc_pc = pcsav;
			Object* ho = *hoc_objpop();
			result = nrnpy_ho2po(ho);
		}else{
			result = (PyObject*)intermediate(self, sym, -1);
		}
		break;
	case SECTION:
		if (!ISARRAY(sym)) {
			result = hocobj_getsec(sym);
		}else{
			result = (PyObject*)intermediate(self, sym, -1);
		}
		break;
	case PROCEDURE:
	case FUNCTION:
	case FUN_BLTIN:
	case BLTIN:
	case HOCOBJFUNCTION:
	case STRINGFUNC:
	    {
		result = hocobj_new(hocobject_type, 0, 0);
		po = (PyHocObject*)result;
		if (self->ho_) {
			po->ho_ = self->ho_;
			hoc_obj_ref(po->ho_);
		}
		po->type_ = 2;
		po->sym_ = sym;
//printf("function %s\n", po->sym_->name);
		break;
	    }
	default: // otherwise
	    {
		char e[200];
		sprintf(e, "no HocObject interface for %s (hoc type %d)", n, sym->type);
		PyErr_SetString(PyExc_TypeError, e);
		break;
	    }
	}
	return result;
}

static int hocobj_setattro(PyHocObject* self, PyObject* name, PyObject* value) {
	int err = 0;
	Inst* pcsav;
	Inst fc;

	PyHocObject* po;

	if (self->type_ == 1 && !self->ho_) {
		return 1;
	}
	Py_INCREF(name);
	char* n = PyString_AsString(name);
//printf("hocobj_setattro %s\n", n);
	Symbol* sym = getsym(n, self->ho_, 1);
	Py_DECREF(name);
	if (!sym) {
		return -1;
	}
	if (self->ho_) { // use the component fork.
		PyObject* result = hocobj_new(hocobject_type, 0, 0);
		po = (PyHocObject*)result;
		po->ho_ = self->ho_;
		hoc_obj_ref(po->ho_);
		po->sym_ = sym;
		// evaluation deferred unless VAR,STRING,OBJECTVAR and not
		// an array
		int t = sym->type;
		if (t == VAR || t == STRING || t == OBJECTVAR || t == RANGEVAR) {
			if (!ISARRAY(sym)) {
				hoc_push_object(po->ho_);
				component(po);
				hocobj_dealloc(po);
				return set_final_from_stk(value);
			}else{
				char e[200];
				sprintf(e, "'%s' requires subscript for assignment", n);
				PyErr_SetString(PyExc_TypeError, e);
				return -1;
			}
		}else{
			PyErr_SetString(PyExc_TypeError, "not assignable");
			return -1;
		}
	}
	switch (sym->type) {
	case VAR: // double*
		if (ISARRAY(sym)) {
			PyErr_SetString(PyExc_TypeError, "wrong number of subscripts");
			return -1;
		}
		hoc_pushs(sym);
		hoc_evalpointer();
		err = PyArg_Parse(value, "d", hoc_pxpop()) != 1;
		break;
	case STRING: // char*
		fc.sym = sym;
		pcsav = save_pc(&fc);
		hoc_push_string();
		hoc_pc = pcsav;
		char* s;
		if (PyArg_Parse(value, "s", &s) == 1) {
			hoc_assign_str(hoc_strpop(), s);
		}else{
			err = 1;
		}
		break;
	case OBJECTVAR: // Object*
	    {
		hocobj_objectvar(sym);
		Object** op;
		op = hoc_objpop();
		PyObject* po;
		PyHocObject* pho;
		if (PyArg_Parse(value, "O", &po) == 1) {
			if (po == Py_None) {
				hoc_obj_unref(*op);
				*op = 0;
			}else if (PyObject_TypeCheck(po, hocobject_type)) {
				pho = (PyHocObject*)po;
				if (pho->sym_) {
 PyErr_SetString(PyExc_TypeError, "argument cannot be a hoc object intermediate");
					return -1;
				}
				hoc_obj_ref(pho->ho_);
				hoc_obj_unref(*op);
				*op = pho->ho_;
			}else{ // it is a PythonObject in hoc
				pyobject_in_objptr(op, po);
			}
		}else{
			err = 1;
		}
		break;
	    }
	default:
		PyErr_SetString(PyExc_TypeError, "not assignable");
		err = -1;
		break;
	}

	return err;
}

static Py_ssize_t hocobj_len(PyObject* self) {
	return 0;
}

static Symbol* sym_vec_x;
static Symbol* sym_mat_x;

static int araychk(Arrayinfo* a, PyHocObject* po, int ix) {
	assert(a->nsub > po->nindex_);
	int n;
	// Hoc Vector and Matrix are special cases because the sub[]
	// do not get filled in til just before hoc_araypt is called.
	// at least check the vector
	if (po->sym_ == sym_vec_x) {
		n = vector_capacity((IvocVect*)po->ho_->u.this_pointer);
	}else if (po->sym_ == sym_mat_x) {
		if (ix >= 0) return 0;
	}else{
		n = a->sub[po->nindex_];
	}
	if (ix < 0 || n <= ix) {
//printf("ix=%d nsub=%d nindex=%d sub[nindex]=%d\n", ix, a->nsub, po->nindex_, a->sub[po->nindex_]);
		char e[200];
		sprintf(e, "%s%s%s", po->ho_?hoc_object_name(po->ho_):"",
			(po->ho_ && po->sym_) ? "." : "",
			po->sym_ ? po->sym_->name : "");
		PyErr_SetString(PyExc_IndexError, e);
		return -1;
	}
	return 0;
}

/*
Had better be an array. But the same ambiguity as with getattro
in that we may return the final value or an intermediate (in the
case where there is more than one dimension.) At least for now we
only have to handle the OBJECTVAR and VAR case as a component and
at the top level.
*/
static PyObject* hocobj_getitem(PyObject* self, Py_ssize_t ix) {
	PyObject* result = NULL;
	PyHocObject* po = (PyHocObject*)self;
	if (po->type_ > 3) {
		if (ix != 0) {
			PyErr_SetString(PyExc_IndexError, "index for hoc ref must be 0" );
			return NULL;
		}
		if (po->type_ == 4) {
			result = Py_BuildValue("d", po->u.x_);
		}else if (po->type_ == 5) {
			result = Py_BuildValue("s", po->u.s_);
		}else{
			result = nrnpy_ho2po(po->u.ho_);
		}
		return result;
	}
	if (!po->sym_) {
		PyErr_SetString(PyExc_TypeError, "unsubscriptable object");
		return NULL;
	}
	assert(po->type_ == 3);
	Arrayinfo* a = hocobj_aray(po->sym_, po->ho_);
	if (araychk(a, po, ix)) {
		return NULL;
	}
	if (a->nsub - 1 >  po->nindex_) { // another intermediate
		PyHocObject* ponew = intermediate(po, po->sym_, ix);
		result = (PyObject*)ponew;
	}else{ // ready to evaluate
		if (po->ho_) {
			eval_component(po, ix);
			if (po->sym_->type == SECTION) {
				section_object_seen = 0;
				result = nrnpy_cas(0,0);
				nrn_popsec();
				return result;
			}else{
				result = nrnpy_hoc_pop();
			}
		}else{ // must be a top level intermediate
			switch (po->sym_->type) {
			case VAR:
				hocobj_pushtop(po, po->sym_, ix);
				hoc_evalpointer();
				--po->nindex_;
				result = Py_BuildValue("d", *hoc_pxpop());
				break;
			case OBJECTVAR:
				hocobj_pushtop(po, 0, ix);
				hocobj_objectvar(po->sym_);
				--po->nindex_;
				result = hocobj_new(hocobject_type, 0, 0);
				((PyHocObject*)result)->ho_ = *hoc_objpop();
				((PyHocObject*)result)->type_ = 1;
				hoc_obj_ref(((PyHocObject*)result)->ho_);
				break;
			case SECTION:
				hocobj_pushtop(po, 0, ix);
				result = hocobj_getsec(po->sym_);
				--po->nindex_;
				break;
			}
		}
	}
	return result;
}

static int hocobj_setitem(PyObject* self, Py_ssize_t i, PyObject* arg) {
//printf("hocobj_setitem %d\n", i);
	int err = -1;
	PyHocObject* po = (PyHocObject*)self;
	if (po->type_ > 3) {
		if (i != 0) {
			PyErr_SetString(PyExc_IndexError, "index for hoc ref must be 0" );
			return -1;
		}
		if (po->type_ == 4) {
			PyArg_Parse(arg, "d", &po->u.x_);
		}else if (po->type_ == 5) {
			char* ts;
			PyArg_Parse(arg, "s", &ts);
			hoc_assign_str(&po->u.s_, ts);
		}else{
			PyObject* tp;
			PyArg_Parse(arg, "O", &tp);
			po->u.ho_ = nrnpy_po2ho(tp);
		}
		return 0;
	}
	if (!po->sym_) {
		PyErr_SetString(PyExc_TypeError, "unsubscriptable object");
		return -1;
	}
	assert(po->type_ == 3);
	Arrayinfo* a = hocobj_aray(po->sym_, po->ho_);
	if (a->nsub - 1 !=  po->nindex_) {
		PyErr_SetString(PyExc_TypeError, "wrong number of subscripts");
		return -1;
	}
	if (araychk(a, po, i)) {
		return -1;
	}
	if (po->ho_) {
		if (po->sym_->type == SECTION) {
			PyErr_SetString(PyExc_TypeError, "not assignable");
		}else{
			eval_component(po, i);
			err = set_final_from_stk(arg);
		}
	}else{ // must be a top level intermediate
		switch (po->sym_->type) {
		case VAR:
			hocobj_pushtop(po, po->sym_, i);
			hoc_evalpointer();
			--po->nindex_;
			err = PyArg_Parse(arg, "d", hoc_pxpop()) != 1;
			break;
		case OBJECTVAR:
		    {
			hocobj_pushtop(po, 0, i);
			hocobj_objectvar(po->sym_);
			--po->nindex_;
			Object** op;
			op = hoc_objpop();
			if (PyArg_Parse(arg, "O!", hocobject_type, &po) == 1) {
				if (po->sym_) {
 PyErr_SetString(PyExc_TypeError, "argument cannot be a hoc object intermediate");
					return -1;
				}
				hoc_obj_ref(po->ho_);
				hoc_obj_unref(*op);
				*op = po->ho_;
			}else{
				err = 1;
			}
			break;
		    }
		default:
			PyErr_SetString(PyExc_TypeError, "not assignable");
			break;
		}
	}
	return err;
}

static PyObject* mkref(PyObject* self, PyObject* args) {
	PyObject* pa;
	PyHocObject* result;
	if (PyArg_ParseTuple(args, "O", &pa) == 1) {
		result = (PyHocObject*)hocobj_new(hocobject_type, 0, 0);
		if (nrnpy_numbercheck(pa)) {
			result->type_ = 4;
			PyObject* pn = PyNumber_Float(pa);
			result->u.x_ = PyFloat_AsDouble(pn);
			Py_XDECREF(pn);
		}else if (PyString_Check(pa)) {
			result->type_ = 5;
			result->u.s_ = 0;
			hoc_assign_str(&result->u.s_, PyString_AsString(pa));
		}else{
			result->type_ = 6;
			result->u.ho_ = nrnpy_po2ho(pa);
		}
		return (PyObject*)result;	
	}
	PyErr_SetString(PyExc_TypeError, "single arg must be number, string, or Object");
	return NULL;
}

static PySequenceMethods hocobj_seqmeth = {
	hocobj_len, NULL, NULL, hocobj_getitem,
	NULL, hocobj_setitem, NULL, NULL,
	NULL, NULL
};

#ifdef WITH_NUMPY


static PyObject* PyObj_FromNrnObj(Object* obj) {

  if (obj==NULL) {
    printf("Warning: PyObj_FromNrnObj obj NULL\n");
    //return null;
    Py_INCREF(Py_None);
    return Py_None;
  }
  

  if (is_obj_type(obj,"Vector")) {
    // we can deal with vectors
    Vect* v = (Vect*)obj->u.this_pointer;
    PyArrayObject*array = NULL;
    int i,n = v->capacity();

    array = (PyArrayObject*)PyArray_FromDims(1,&n,PyArray_DOUBLE);
    for (i=0;i<n;i++) {
      *(double*)(array->data + i*array->strides[0]) = (*v)[i];
    }

    return (PyObject*)array;

  }

  printf("Warning: PyObj_FromNrnObj cannot handle obj type, returning NULL\n");
  //return NULL;
  Py_INCREF(Py_None);
  return Py_None;
  
}





static int hocobj_tonumpy(PyObject* self, PyObject* args) {


  if (!PyArg_ParseTuple(args, "")) {
    return NULL;
  }

  PyArrayObject* array = NULL;


  if (self->type_ == 2 || self->type_ == 3) {

    Symbol* sym = self->sym_;

    if (!sym) {
      PyErr_SetString(PyExc_RuntimeError, "sym==NULL");
      return NULL;
    }

    if (sym->type == VAR) {

      if (ISARRAY(sym)) {

	// Make a numpy array from an ndim NEURON array

	int total = hoc_total_array(sym);
	PyArrayObject* array = NULL;
	int i;

	Arrayinfo* a = sym->arayinfo;
	double* p = OPVAL(sym);
	  
	if (a) {
	  PyObject* pDims = PyList_New(a->nsub);

	  for (i= a->nsub-1;i>=0;--i) {
	    PyList_SetItem(pDims,i,PyInt_FromLong(a->sub[i]));
	  }

	  // first contiguous, then reshape
	  array = (PyArrayObject*)PyArray_FromDims(1,&total,PyArray_DOUBLE);
	  if (array==NULL) {
	    PyErr_SetString(PyExc_RuntimeError,"hoc.get hoc error allocating array.");
	    return NULL;
	  }

	  // fill array memory

	  for (i=0;i<total;i++) {
	    *(double*)(array->data + i*array->strides[0]) = p[i];
	  }

	  // return shaped array

	  pObj = (PyObject*)PyArray_Reshape(array,pDims);
	  // throw away pDims
	  Py_DECREF(pDims);

	}

      }
      else {
	// VAR is not an array

	pObj = PyFloat_FromDouble(*OPVAL(sym));
      }

    }
    else if (sym->type == OBJECTVAR) {

      if (ISARRAY(sym)) {

	// create numpy array of type 'O' (PyObjects) from NEURON OBJREF array

	int total = hoc_total_array(sym);
	PyArrayObject* array = NULL;
	int i;

	Arrayinfo* a = sym->arayinfo;

	if (a) {
	  PyObject* pDims = PyList_New(a->nsub);

	  for (i= a->nsub-1;i>=0;--i) {
	    PyList_SetItem(pDims,i,PyInt_FromLong(a->sub[i]));
	  }


	  // first contiguous, then reshape

	  array = (PyArrayObject*)PyArray_FromDims(1,&total,PyArray_OBJECT);
	  if (array==NULL) {
	    PyErr_SetString(PyExc_RuntimeError,"hoc.get hoc error allocating array.");
	    return NULL;
	  }

	  // fill array memory

	  for (i=0;i<total;i++) {
	    *(PyObject**)(array->data + i*array->strides[0]) = PyObj_FromNrnObj(OPOBJ(sym)[i]);
	  }

	  // return shaped array

	  pObj = (PyObject*)PyArray_Reshape(array,pDims);
	  // throw away pDims
	  Py_DECREF(pDims);
	      
	  if (pObj==NULL) {
	    PyErr_SetString(PyExc_RuntimeError,"hoc.get hoc error reshaping array.");
	    return NULL;
	  }


	}

      }
      else {
	// return single object
	pObj = PyObj_FromNrnObj(OPOBJ(sym)[0]);
      }


  }
  else {
    PyErr_SetString(PyExc_TypeError, "not a compound type");
    return NULL;
  }

}




static PyMethodDef hocobj_methods[] = {
  {"toarray",hocobj_tonumpy,METH_VARARGS,"toarray(self) returns a numpy array of self."},
  {NULL, NULL, 0, NULL}
};


#else

static PyMethodDef hocobj_methods[] = {
	{"ref", mkref, METH_VARARGS, "Wrap to allow call by reference in a hoc function"},
	{NULL, NULL, 0, NULL}
};


#endif //WITH_NUMPY


static PyMemberDef hocobj_members[] = {
	{NULL, 0, 0, 0, NULL}
};

static PyTypeObject nrnpy_HocObjectType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "hoc.HocObject",         /*tp_name*/
    sizeof(PyHocObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)hocobj_dealloc,                        /*tp_dealloc*/
    (printfunc)hocobj_print,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    &hocobj_seqmeth,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    (ternaryfunc)hocobj_call,                         /*tp_call*/
    0,                         /*tp_str*/
    (getattrofunc)hocobj_getattro,                         /*tp_getattro*/
    (setattrofunc)hocobj_setattro,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Hoc Object wrapper",         /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
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

myPyMODINIT_FUNC nrnpy_hoc() {
	PyObject* m;
	m = Py_InitModule3("hoc", HocMethods,
		"HOC interaction with Python");

	hocobject_type = &nrnpy_HocObjectType;
	if (PyType_Ready(hocobject_type) < 0)
		return;
	Py_INCREF(hocobject_type);
//printf("AddObject HocObject\n");
	PyModule_AddObject(m, "HocObject", (PyObject*)hocobject_type);

	Symbol* s;
	s = hoc_lookup("Vector"); assert(s);
	sym_vec_x = hoc_table_lookup("x", s->u.ctemplate->symtable); assert(sym_vec_x);
	s = hoc_lookup("Matrix"); assert(s);
	sym_mat_x = hoc_table_lookup("x", s->u.ctemplate->symtable); assert(sym_mat_x);

#ifdef WITH_NUMPY

	// setup for numpy
	import_array();

#endif // WITH_NUMPY


	nrnpy_nrn();
}

}//end of extern c

