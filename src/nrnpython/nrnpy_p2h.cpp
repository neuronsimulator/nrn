#include <../../nrnconf.h>

#include <stdio.h>
#include <InterViews/resource.h>
#include <nrnoc2iv.h>
#include <classreg.h>
#include <nrnpython.h>
#include <hoccontext.h>

extern "C" {
#include "parse.h"
extern void hoc_nopop();
extern void hoc_pop_defer();
extern Object* hoc_new_object(Symbol*, void*);
extern int hoc_stack_type();
extern char** hoc_strpop();
extern Object** hoc_objpop();
extern Object* hoc_pop_object();
extern void hoc_stkobj_unref(Object*);
extern void hoc_tobj_unref(Object**);
extern int hoc_ipop();
PyObject* nrnpy_hoc2pyobject(Object*);
Object* nrnpy_pyobject_in_obj(PyObject*);
int nrnpy_ho_eq_po(Object*, PyObject*);
extern Symbol* nrnpy_pyobj_sym_;
extern void (*nrnpy_py2n_component)(Object*, Symbol*, int, int);
extern void (*nrnpy_hpoasgn)(Object*, int);
extern double (*nrnpy_praxis_efun)(Object*, Object*);
extern int (*nrnpy_hoccommand_exec)(Object*);
extern int (*nrnpy_hoccommand_exec_strret)(Object*, char*, int);
extern void (*nrnpy_cmdtool)(Object*, int, double, double, int);
extern double (*nrnpy_guigetval)(Object*);
extern void (*nrnpy_guisetval)(Object*, double);
extern int (*nrnpy_guigetstr)(Object*, char**);
extern char* (*nrnpy_po2pickle)(Object*, size_t* size);
extern Object* (*nrnpy_pickle2po)(char*, size_t size);
extern char* (*nrnpy_callpicklef)(char*, size_t size, int narg, size_t* retsize);
extern int (*nrnpy_pysame)(Object*, Object*); // contain same Python object
extern Object* (*nrnpympi_alltoall)(Object*, int);

void nrnpython_reg_real();
PyObject* nrnpy_ho2po(Object*);
void nrnpy_decref_defer(PyObject*);
void nrnpy_decref_clear();
PyObject* nrnpy_pyCallObject(PyObject*, PyObject*);

Object* nrnpy_po2ho(PyObject*);
static void py2n_component(Object*, Symbol*, int, int);
static void hpoasgn(Object*, int);
static double praxis_efun(Object*, Object*);
static int hoccommand_exec(Object*);
static int hoccommand_exec_strret(Object*, char*, int);
static void grphcmdtool(Object*, int, double, double, int);
static double guigetval(Object*);
static void guisetval(Object*, double);
static int guigetstr(Object*, char**);
static char* po2pickle(Object*, size_t* size);
static Object* pickle2po(char*, size_t size);
static char* call_picklef(char*, size_t size, int narg, size_t* retsize);
static Object* py_alltoall(Object*, int);
static int pysame(Object*, Object*);
static PyObject* main_module;
static PyObject* main_namespace;
static hoc_List* dlist;
#if NRNPYTHON_DYNAMICLOAD
extern int nrnpy_site_problem;
extern int* nrnpy_site_problem_p;
#endif
}

class Py2Nrn {
public:
	Py2Nrn();
	virtual ~Py2Nrn();
	int type_; //0 toplevel
	PyObject* po_;
};

static void* p_cons(Object*) {
	Py2Nrn* p = new Py2Nrn();
	return p;
}

static void p_destruct(void* v) {
	delete (Py2Nrn*)v;
}

Member_func p_members[] = {0,0};

void nrnpython_reg_real() {
	//printf("nrnpython_reg_real()\n");
	class2oc("PythonObject", p_cons, p_destruct, p_members, NULL, NULL, NULL);
	Symbol* s = hoc_lookup("PythonObject");
	nrnpy_pyobj_sym_ = s;
	nrnpy_py2n_component = py2n_component;
	nrnpy_hpoasgn = hpoasgn;
	nrnpy_praxis_efun = praxis_efun;
	nrnpy_hoccommand_exec = hoccommand_exec;
	nrnpy_hoccommand_exec_strret = hoccommand_exec_strret;
	nrnpy_cmdtool = grphcmdtool;
	nrnpy_guigetval = guigetval;
	nrnpy_guisetval = guisetval;
	nrnpy_guigetstr = guigetstr;
	nrnpy_po2pickle = po2pickle;
	nrnpy_pickle2po = pickle2po;
	nrnpy_callpicklef = call_picklef;
	nrnpympi_alltoall = py_alltoall;
	nrnpy_pysame = pysame;
	dlist = hoc_l_newlist();
#if NRNPYTHON_DYNAMICLOAD
	nrnpy_site_problem_p = &nrnpy_site_problem;
#endif
}

Py2Nrn::Py2Nrn() {
	po_ = NULL;
	type_ = 0;	
//	printf("Py2Nrn() %p\n", this);
}
Py2Nrn::~Py2Nrn() {
	PyGILState_STATE gilsav = PyGILState_Ensure();
	Py_XDECREF(po_);
	PyGILState_Release(gilsav);	
//	printf("~Py2Nrn() %p\n", this);
}

int nrnpy_ho_eq_po(Object* ho, PyObject* po) {
	if (ho->ctemplate->sym == nrnpy_pyobj_sym_) {
		return ((Py2Nrn*)ho->u.this_pointer)->po_ == po;
	}
	return 0;
}

int pysame(Object* o1, Object* o2) {
	if (o2->ctemplate->sym == nrnpy_pyobj_sym_) {
		return nrnpy_ho_eq_po(o1, ((Py2Nrn*)o2->u.this_pointer)->po_);
	}
	return 0;
}

PyObject* nrnpy_hoc2pyobject(Object* ho) {
	PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
	if (!po) {
		if (!main_module) {
			main_module = PyImport_AddModule("__main__");
			main_namespace = PyModule_GetDict(main_module);
			Py_INCREF(main_module);
			Py_INCREF(main_namespace);
		}
		po = main_module;
	}
	return po;
}

Object* nrnpy_pyobject_in_obj(PyObject* po) {
	Py2Nrn* pn = new Py2Nrn();
	pn->po_ = po;
	Py_INCREF(po);
	pn->type_ = 1;
	Object* on = hoc_new_object(nrnpy_pyobj_sym_, (void*)pn);
	hoc_obj_ref(on);
	return on;
}

PyObject* nrnpy_pyCallObject(PyObject* callable, PyObject* args) {
	// When hoc calls a PythonObject method, then in case python
	// calls something back in hoc, the hoc interpreter must be
	// at the top level
	HocTopContextSet
	PyObject* p = PyObject_CallObject(callable, args);
#if 0
printf("PyObject_CallObject callable\n");
PyObject_Print(callable, stdout, 0);
printf("\nargs\n");
PyObject_Print(args, stdout, 0);
printf("\nreturn %p\n", p);
if (p) { PyObject_Print(p, stdout, 0); printf("\n");}
#endif
	HocContextRestore
	return p;
}

void py2n_component(Object* ob, Symbol* sym, int nindex, int isfunc) {
#if 0
	if (isfunc) {
	printf("py2n_component %s.%s(%d)\n", hoc_object_name(ob), sym->name, nindex);
	}else{
	printf("py2n_component %s.%s[%d]\n", hoc_object_name(ob), sym->name, nindex);
	}
#endif
	int i;
	Py2Nrn* pn = (Py2Nrn*)ob->u.this_pointer;
	PyObject* head = pn->po_;
	PyObject* tail;
	PyGILState_STATE gilsav = PyGILState_Ensure();
	if (pn->type_ == 0) { // top level
		if (!main_module) {
			main_module = PyImport_AddModule("__main__");
			main_namespace = PyModule_GetDict(main_module);
			Py_INCREF(main_module);
			Py_INCREF(main_namespace);
		}
		tail = PyRun_String(sym->name, Py_eval_input, main_namespace, main_namespace) ;
	}else{
		Py_INCREF(head);
		if (strcmp(sym->name, "_") == 0) {
			tail = head;
			Py_INCREF(tail);
		}else{
			tail = PyObject_GetAttrString(head, sym->name);
		}
	}
	if (!tail) {
		PyErr_Print();
		PyGILState_Release(gilsav);
		hoc_execerror("No attribute:", sym->name);
	}
	PyObject* args = 0;
	Object* on;
	PyObject* result = 0;
	if (isfunc) {
		args = PyTuple_New(nindex);
		for (i = 0; i < nindex; ++i) {
			PyObject* arg = nrnpy_hoc_pop();
//PyObject_Print(arg, stdout, 0);
//printf(" %d   arg %d\n", arg->ob_refcnt,  i);
			if (PyTuple_SetItem(args, nindex - 1 - i, arg)) {
				assert(0);
			}
		}
//printf("PyObject_CallObject %s %p\n", sym->name, tail);
		result = nrnpy_pyCallObject(tail, args);
		Py_DECREF(args);
//PyObject_Print(result, stdout, 0);
//printf("  result of call\n");
		if (!result) {
			PyErr_Print();
			PyGILState_Release(gilsav);
			hoc_execerror("PyObject method call failed:", sym->name);
		}
	}else if (nindex) {
		PyObject* arg;
		if (hoc_stack_type() == NUMBER) {
			arg = Py_BuildValue("l", (long)hoc_xpop());
		}else{
			arg = nrnpy_hoc_pop();
		}
		result = PyObject_GetItem(tail, arg);
		if (!result) {
			PyErr_Print();
			PyGILState_Release(gilsav);
			hoc_execerror("Python get item failed:", hoc_object_name(ob)); 
		}
	}else{
		result = tail;
		Py_INCREF(result);
	}
//printf("py2n_component %s %d %s result refcount=%d\n", hoc_object_name(ob), ob->refcount, sym->name, result->ob_refcnt);
	// if numeric, string, or python HocObject return those
	if (nrnpy_numbercheck(result)) {
		hoc_pop_defer();
		PyObject* pn = PyNumber_Float(result);
		hoc_pushx(PyFloat_AsDouble(pn));
		Py_XDECREF(pn);
		Py_XDECREF(result);
	}else if (PyString_Check(result)) {
		char** ts = hoc_temp_charptr();
		*ts = PyString_AsString(result);
		hoc_pop_defer();
		hoc_pushstr(ts);
		// how can we defer the result unref til the string is popped
		nrnpy_decref_defer(result);
	}else{
//PyObject_Print(result, stdout, 0);
		on = nrnpy_po2ho(result);
		hoc_pop_defer();
		hoc_push_object(on);
		if (on) { 
			on->refcount--;
		}
		Py_XDECREF(result);
	}
	Py_XDECREF(head);
	Py_DECREF(tail);
	PyGILState_Release(gilsav);
}

static void hpoasgn(Object* o, int type) {
	int nindex; Symbol* sym;
	PyObject* poleft;
	PyObject* poright;
	if (type == NUMBER) {
		poright = PyFloat_FromDouble(hoc_xpop());
	}else if (type == STRING) {
		poright = Py_BuildValue("s", *hoc_strpop());
	}else if (type == OBJECTVAR || type == OBJECTTMP) {
		Object** po2 = hoc_objpop();
		poright = nrnpy_ho2po(*po2);
		hoc_tobj_unref(po2);
	}else{
		hoc_execerror("Cannot assign that type to PythonObject", (char*)0);
	}
	assert(o == hoc_pop_object());
	poleft = nrnpy_hoc2pyobject(o);
	sym = hoc_spop();
	nindex = hoc_ipop();
//printf("hpoasgn %s %s %d\n", hoc_object_name(o), sym->name, nindex);
	if (nindex == 0) {
		PyObject_SetAttrString(poleft, sym->name, poright);
	}else if (nindex == 1) {
		PyObject* key = PyLong_FromDouble(hoc_xpop());
		PyObject* a = PyObject_GetAttrString(poleft, sym->name);
		PyObject_SetItem(a, key, poright);
		Py_DECREF(a);
		Py_DECREF(key);
	}else{
		char buf[512];
		sprintf(buf, "%s.%s[][]...=...:", hoc_object_name(o), sym->name);
hoc_execerror(buf, "HOC cannot handle PythonObject assignment with more than one index.");
	}
	Py_DECREF(poright);
//	hoc_push_object(o);
	hoc_stkobj_unref(o);
}

void nrnpy_decref_defer(PyObject* po) {
	if (po) {
#if 0
		PyObject* ps = PyObject_Str(po);
		printf("defer %s\n", PyString_AsString(ps));
		Py_DECREF(ps);
#endif
		hoc_l_lappendvoid(dlist, (void*)po);
	}
}
void nrnpy_decref_clear() {
	while(dlist->next != dlist) {
		PyObject* po = (PyObject*)VOIDITM(dlist->next);
#if 1
		PyObject* ps = PyObject_Str(po);
		printf("decref %s\n", PyString_AsString(ps));
		Py_DECREF(ps);
#endif
		Py_DECREF(po);
		hoc_l_delete(dlist->next);
	}
}

#if (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION == 3)
// copied from /Modules/_ctypes/_ctypes.c
static PyObject* PyTuple_Pack(int n, ...) {
        int i;
        PyObject *o;
        PyObject *result;
        PyObject **items;
        va_list vargs;

        va_start(vargs, n);
        result = PyTuple_New(n);
        if (result == NULL)
                return NULL;
        items = ((PyTupleObject *)result)->ob_item;
        for (i = 0; i < n; i++) {
                o = va_arg(vargs, PyObject *);
                Py_INCREF(o);
                items[i] = o;
        }
        va_end(vargs);
        return result;
}
#endif

static PyObject* hoccommand_exec_help1(PyObject* po) {
	PyObject* r;
//PyObject_Print(po, stdout, 0);
//printf("\n");
	if (PyTuple_Check(po)) {
		PyObject* args = PyTuple_GetItem(po, 1);
		if (!PyTuple_Check(args)) {
			args = PyTuple_Pack(1, args);
		}
//PyObject_Print(PyTuple_GetItem(po, 0), stdout, 0);
//printf("\n");
//PyObject_Print(args, stdout, 0);
//printf("\n");
//printf("threadstate %p\n", PyThreadState_GET());
		r = nrnpy_pyCallObject(PyTuple_GetItem(po, 0), args);
	}else{
		r = nrnpy_pyCallObject(po, PyTuple_New(0));
	}
	if (r == NULL) {
		PyErr_Print();
	}
	return r;
}

static PyObject* hoccommand_exec_help(Object* ho) {
	PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
//printf("%s\n", hoc_object_name(ho));
	return hoccommand_exec_help1(po);
}

static double praxis_efun(Object* ho, Object* v) {
	PyGILState_STATE s = PyGILState_Ensure();
	PyObject* pc = nrnpy_ho2po(ho);
	PyObject* pv = nrnpy_ho2po(v);
	PyObject* po = Py_BuildValue("(OO)", pc, pv);
	Py_XDECREF(pc);
	Py_XDECREF(pv);
	PyObject* r = hoccommand_exec_help1(po);
	PyObject* pn = PyNumber_Float(r);
	double x = PyFloat_AsDouble(pn);
	Py_XDECREF(pn);
	Py_XDECREF(r);
	Py_XDECREF(po);
	PyGILState_Release(s);
	return x;
}

static int hoccommand_exec(Object* ho) {
	PyGILState_STATE s = PyGILState_Ensure();
	PyObject* r = hoccommand_exec_help(ho);
	if (r == NULL) {
		PyGILState_Release(s);
		hoc_execerror("Python Callback failed", 0);
	}
	Py_XDECREF(r);
	PyGILState_Release(s);
	return (r != NULL);
}

static int hoccommand_exec_strret(Object* ho, char* buf, int size) {
	PyGILState_STATE s = PyGILState_Ensure();
	PyObject* r = hoccommand_exec_help(ho);
	if (r) {
		PyObject* pn = PyObject_Str(r);
		const char* cp = PyString_AsString(pn);
		strncpy(buf, cp, size);
		nrnpy_pystring_asstring_free(cp);
		buf[size-1] = '\0';
		Py_XDECREF(pn);
		Py_XDECREF(r);
		PyGILState_Release(s);
	}else{
		PyGILState_Release(s);
		hoc_execerror("Python Callback failed", 0);
	}
	return (r != NULL);
}

static void grphcmdtool(Object* ho, int type, double x, double y, int key) {
	PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
	PyObject* r;
	PyGILState_STATE s = PyGILState_Ensure();
	PyObject* args = PyTuple_Pack(4, PyInt_FromLong(type),
		PyFloat_FromDouble(x), PyFloat_FromDouble(y), PyInt_FromLong(key));
	r = nrnpy_pyCallObject(po, args);
	Py_XDECREF(args);
	Py_XDECREF(r);
	PyGILState_Release(s);
}

static double guigetval(Object* ho){
	PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
	PyGILState_STATE s = PyGILState_Ensure();
	PyObject* r = PyObject_GetAttr(PyTuple_GetItem(po, 0), PyTuple_GetItem(po, 1));
	PyObject* pn = PyNumber_Float(r);
	double x = PyFloat_AsDouble(pn);
	Py_XDECREF(pn);
	PyGILState_Release(s);
	return x;
}

static void guisetval(Object* ho, double x) {
	PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
	PyGILState_STATE s = PyGILState_Ensure();
	PyObject* pn = PyFloat_FromDouble(x);
	if (PyObject_SetAttr(PyTuple_GetItem(po, 0), PyTuple_GetItem(po, 1), pn)){
		;
	}
	Py_XDECREF(pn);
	PyGILState_Release(s);
}

static int guigetstr(Object* ho, char** cpp){
	PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
	PyGILState_STATE s = PyGILState_Ensure();
	PyObject* r = PyObject_GetAttr(PyTuple_GetItem(po, 0), PyTuple_GetItem(po, 1));
	PyObject* pn = PyObject_Str(r);
	const char* cp = PyString_AsString(pn);
	if (*cpp && strcmp(*cpp, cp) == 0) {
		Py_XDECREF(pn);
		nrnpy_pystring_asstring_free(cp);
		PyGILState_Release(s);
		return 0;
	}
	if (*cpp) { delete [] *cpp; }
	*cpp = new char[strlen(cp) + 1];
	strcpy(*cpp, cp);
	Py_XDECREF(pn);
	nrnpy_pystring_asstring_free(cp);
	PyGILState_Release(s);
	return 1;
}

static PyObject* loads;
static PyObject* dumps;

static void setpickle() {
	if (!dumps) {
		PyObject* pickle = PyImport_ImportModule("cPickle");
		if (pickle == NULL) {
			pickle = PyImport_ImportModule("pickle");
		}
		if (pickle) {
			Py_INCREF(pickle);
			dumps = PyObject_GetAttrString(pickle, "dumps");
			loads = PyObject_GetAttrString(pickle, "loads");
			if (dumps) {
				Py_INCREF(dumps);
				Py_INCREF(loads);
			}
		}
	}
	if (!dumps || !loads) {
		hoc_execerror("Neither Python cPickle nor pickle are available", 0);
	}
}

// note that *size includes the null terminating character if it exists
static char* pickle(PyObject* p, size_t* size) {
	PyObject* arg = PyTuple_Pack(1, p);
	PyObject* r = nrnpy_pyCallObject(dumps, arg);
	Py_XDECREF(arg);
	assert(r);
#if PY_MAJOR_VERSION >= 3
	assert(PyBytes_Check(r));
	*size = PyBytes_Size(r);
	char* buf1 = PyBytes_AsString(r);
#else
	char* buf1 = PyString_AsString(r);
	*size  = PyString_Size(r) + 1;
#endif
	char* buf = new char[*size];
	for (int i = 0; i < *size; ++i) {
		buf[i] = buf1[i];
	}
	Py_XDECREF(r);
	return buf;
}

static char* po2pickle(Object* ho, size_t* size) {
	setpickle();
	if (ho && ho->ctemplate->sym ==	nrnpy_pyobj_sym_) {
		PyObject* po = nrnpy_hoc2pyobject(ho);
		char* buf = pickle(po, size);
		return buf;
	}else{
		return 0;
	}
}

static PyObject* unpickle(char* s, size_t size) {
#if PY_MAJOR_VERSION >= 3
	PyObject* ps = PyBytes_FromStringAndSize(s, size);
#else
	PyObject* ps = PyString_FromString(s);
#endif
	PyObject* arg = PyTuple_Pack(1, ps);
	PyObject* po = nrnpy_pyCallObject(loads, arg);
	assert(po);
	Py_XDECREF(arg);
	Py_XDECREF(ps);
	return po;
}

static Object* pickle2po(char* s, size_t size) {
	setpickle();
	PyObject* po = unpickle(s, size);
	Object* ho = nrnpy_pyobject_in_obj(po);
	Py_XDECREF(po);
	return ho;
}


char* call_picklef(char *fname, size_t size, int narg, size_t* retsize) {
	// fname is a pickled callable, narg is the number of args on the
	// hoc stack with types double, char*, hoc Vector, and PythonObject
	// callable return must be pickleable.
	PyObject* args = 0;
	PyObject* result = 0;
	PyObject* callable;

	setpickle();
#if PY_MAJOR_VERSION >= 3
	PyObject* ps= PyBytes_FromStringAndSize(fname, size);
#else
	PyObject* ps = PyString_FromString(fname);
#endif
	args = PyTuple_Pack(1, ps);
	callable = nrnpy_pyCallObject(loads, args);
	assert(callable);
	Py_XDECREF(args);
	Py_XDECREF(ps);

	args = PyTuple_New(narg);
	for (int i = 0; i < narg; ++i) {
		PyObject* arg = nrnpy_hoc_pop();
		if (PyTuple_SetItem(args, narg - 1 - i, arg)) {
			assert(0);
		}
		//Py_XDECREF(arg);
	}
	result = nrnpy_pyCallObject(callable, args);
	Py_DECREF(callable);
	Py_DECREF(args);
#if PY_MAJOR_VERSION >= 3
	if (PyBytes_Check(result)) {
#else
	if (!result) {
#endif
		PyErr_Print();
		hoc_execerror("PyObject method call failed:", 0);
	}
	char* rs = pickle(result, retsize);
	Py_XDECREF(result);
	return rs;
}

#include "nrnmpi.h"

int* mk_displ(int* cnts) {
	int* displ = new int[nrnmpi_numprocs + 1];
	displ[0] = 0;
	for (int i=0; i < nrnmpi_numprocs; ++i) {
		displ[i+1] = displ[i] + cnts[i];
	}
	return displ;
}

Object* py_alltoall(Object* o, int size) {
	int np = nrnmpi_numprocs;
	PyObject* psrc = nrnpy_hoc2pyobject(o);
	if (!PyList_Check(psrc)) {
		hoc_execerror("Argument must be a Python list", 0);
	}
	if (PyList_Size(psrc) != np) {
		hoc_execerror("py_alltoall list size must be nhost",0);
	}
	if (np == 1) {
		return o;
	}
#if NRNMPI
	setpickle();
	int* scnt = new int[np];
	for (int i=0; i < np; ++i) { scnt[i] = 0; }

	PyObject* pdest;
	PyObject* iterator = PyObject_GetIter(psrc);
	PyObject* p;
	
	size_t bufsz = 100000; // 100k buffer to start with
	if (size > 0) { // or else the positive number specified
		bufsz = size;
	}
	char* s = 0;
	if (size >= 0) { // otherwise count only
		s = new char[bufsz];
	}
	int curpos = 0;
	for (int i=0 ; (p = PyIter_Next(iterator)) != NULL; ++i) {
		if (p == Py_None) {
			scnt[i] = 0;
			Py_DECREF(p);
			continue;
		}
		size_t sz;
		char* b = pickle(p, &sz);
	    if (size >= 0) {
		if (curpos + sz >= bufsz) {
			bufsz = bufsz * 2 + sz;
			char* s2 = new char[bufsz];
			for (int i = 0; i < curpos; ++i) {
				s2[i] = s[i];
			}
			delete [] s;
			s = s2;
		}
		for (int j=0; j < sz; ++j) {
			s[curpos+j] = b[j];
		}
	    }
		curpos += sz;
		scnt[i] = sz;
		delete [] b;
		Py_DECREF(p);
	}
	Py_DECREF(iterator);

	// what are destination counts
	int* ones = new int[np];
	for (int i=0; i < np; ++i) { ones[i] = 1; }
	int* sdispl = mk_displ(ones);
	int* rcnt = new int[np];
	nrnmpi_int_alltoallv(scnt, ones, sdispl, rcnt, ones, sdispl);
	delete [] ones;
	delete [] sdispl;

	// exchange
	sdispl = mk_displ(scnt);
	int* rdispl = mk_displ(rcnt);
	char* r = 0;
    if (size < 0) {
	pdest = PyTuple_New(2);
	PyTuple_SetItem(pdest, 0, Py_BuildValue("l", (long)sdispl[np]));
	PyTuple_SetItem(pdest, 1, Py_BuildValue("l", (long)rdispl[np]));
	delete [] scnt;
	delete [] sdispl;
	delete [] rcnt;
	delete [] rdispl;
    } else {
	r = new char[rdispl[np]];
	nrnmpi_char_alltoallv(s, scnt, sdispl, r, rcnt, rdispl);
	delete [] s;
	delete [] scnt;
	delete [] sdispl;

	assert((pdest = PyList_New(np)) != NULL);
	for (int i=0; i < np; ++i) {
		if (rcnt[i] == 0) {
			PyList_SetItem(pdest, i, Py_None);
		}else{
			PyObject* p = unpickle(r + rdispl[i], rcnt[i]);
			PyList_SetItem(pdest, i, p);
		}
	}

	delete [] r;
	delete [] rcnt;
	delete [] rdispl;
    }
	Object* ho = nrnpy_po2ho(pdest);
	--ho->refcount;
	return ho;
#endif
}
