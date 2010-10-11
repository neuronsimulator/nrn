#include <../../nrnconf.h>

#include <stdio.h>
#include <InterViews/resource.h>
#include <nrnoc2iv.h>
#include <classreg.h>
#include <nrnpython.h>

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
extern void nrnpython_ensure_threadstate();

extern Object* hoc_thisobject;
extern Symlist* hoc_symlist;
extern Objectdata* hoc_top_level_data;
extern Symlist* hoc_top_level_symlist;

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
static PyObject* main_module;
static PyObject* main_namespace;
static hoc_List* dlist;
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
	class2oc("PythonObject", p_cons, p_destruct, p_members);
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
	dlist = hoc_l_newlist();
}

Py2Nrn::Py2Nrn() {
	po_ = NULL;
	type_ = 0;	
//	printf("Py2Nrn() %lx\n", (long)this);
}
Py2Nrn::~Py2Nrn() {
	nrnpython_ensure_threadstate();
	Py_XDECREF(po_);
//	printf("~Py2Nrn() %lx\n", (long)this);
}

int nrnpy_ho_eq_po(Object* ho, PyObject* po) {
	if (ho->ctemplate->sym == nrnpy_pyobj_sym_) {
		return ((Py2Nrn*)ho->u.this_pointer)->po_ == po;
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
	Object* objsave = 0;
	Objectdata* obdsave;
	Symlist* slsave;
	if (hoc_thisobject) {
		objsave = hoc_thisobject;
		obdsave = hoc_objectdata_save();
		slsave = hoc_symlist;
		hoc_thisobject = 0;
		hoc_objectdata = hoc_top_level_data;
		hoc_symlist = hoc_top_level_symlist;
	}

	nrnpython_ensure_threadstate();
	PyObject* p = PyObject_CallObject(callable, args);
#if 0
printf("PyObject_CallObject callable\n");
PyObject_Print(callable, stdout, 0);
printf("\nargs\n");
PyObject_Print(args, stdout, 0);
printf("\nreturn %lx\n", (long)p);
if (p) { PyObject_Print(p, stdout, 0); printf("\n");}
#endif

	if (objsave) {
		hoc_thisobject = objsave;
		hoc_objectdata = hoc_objectdata_restore(obdsave);
		hoc_symlist = slsave;
	}
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
//printf("PyObject_CallObject %s %lx\n", sym->name, (long)tail);
		result = nrnpy_pyCallObject(tail, args);
		Py_DECREF(args);
//PyObject_Print(result, stdout, 0);
//printf("  result of call\n");
		if (!result) {
			PyErr_Print();
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
#if 1
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
	nrnpython_ensure_threadstate();
	// should we use this instead?
	//PyGILState_STATE s = PyGILState_Ensure();
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
//printf("threadstate %lx\n", PyThreadState_GET());
		r = nrnpy_pyCallObject(PyTuple_GetItem(po, 0), args);
	}else{
		r = nrnpy_pyCallObject(po, PyTuple_New(0));
	}
	//PyGILState_Release(s);
	if (r == NULL) {
		PyErr_Print();
		hoc_execerror("Python Callback failed", 0);
	}
	return r;
}

static PyObject* hoccommand_exec_help(Object* ho) {
	PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
//printf("%s\n", hoc_object_name(ho));
	return hoccommand_exec_help1(po);
}

static double praxis_efun(Object* ho, Object* v) {
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
	return x;
}

static int hoccommand_exec(Object* ho) {
	PyObject* r = hoccommand_exec_help(ho);
	Py_XDECREF(r);
	return (r != NULL);
}

static int hoccommand_exec_strret(Object* ho, char* buf, int size) {
	PyObject* r = hoccommand_exec_help(ho);
	if (r) {
		PyObject* pn = PyObject_Str(r);
		const char* cp = PyString_AsString(pn);
		strncpy(buf, cp, size);
		buf[size-1] = '\0';
		Py_XDECREF(pn);
		Py_XDECREF(r);
	}
	return (r != NULL);
}

static void grphcmdtool(Object* ho, int type, double x, double y, int key) {
	PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
	PyObject* r;
	nrnpython_ensure_threadstate();
	// should we use this instead?
	//PyGILState_STATE s = PyGILState_Ensure();
	PyObject* args = PyTuple_Pack(4, PyInt_FromLong(type),
		PyFloat_FromDouble(x), PyFloat_FromDouble(y), PyInt_FromLong(key));
	r = nrnpy_pyCallObject(po, args);
	//PyGILState_Release(s);
	Py_XDECREF(args);
	Py_XDECREF(r);
}

static double guigetval(Object* ho){
	PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
	PyObject* r = PyObject_GetAttr(PyTuple_GetItem(po, 0), PyTuple_GetItem(po, 1));
	PyObject* pn = PyNumber_Float(r);
	double x = PyFloat_AsDouble(pn);
	Py_XDECREF(pn);
	return x;
}

static void guisetval(Object* ho, double x) {
	PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
	PyObject* pn = PyFloat_FromDouble(x);
	if (PyObject_SetAttr(PyTuple_GetItem(po, 0), PyTuple_GetItem(po, 1), pn)){
		;
	}
	Py_XDECREF(pn);
}

static int guigetstr(Object* ho, char** cpp){
	PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
	PyObject* r = PyObject_GetAttr(PyTuple_GetItem(po, 0), PyTuple_GetItem(po, 1));
	PyObject* pn = PyObject_Str(r);
	const char* cp = PyString_AsString(pn);
	if (*cpp && strcmp(*cpp, cp) == 0) {
		Py_XDECREF(pn);
		return 0;
	}
	if (*cpp) { delete [] *cpp; }
	*cpp = new char[strlen(cp) + 1];
	strcpy(*cpp, cp);
	Py_XDECREF(pn);
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

static Object* pickle2po(char* s, size_t size) {
	setpickle();
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
