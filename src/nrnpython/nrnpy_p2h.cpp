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
extern Symbol* nrnpy_pyobj_sym_;
extern void (*nrnpy_py2n_component)(Object*, Symbol*, int, int);
extern void (*nrnpy_hpoasgn)(Object*, int);
extern int (*nrnpy_hoccommand_exec)(Object*);
extern void (*nrnpy_cmdtool)(Object*, int, double, double, int);
extern double (*nrnpy_guigetval)(Object*);
extern void (*nrnpy_guisetval)(Object*, double);
extern int (*nrnpy_guigetstr)(Object*, char**);
extern void nrnpython_ensure_threadstate();

void nrnpython_reg_real();
PyObject* nrnpy_ho2po(Object*);
void nrnpy_decref_defer(PyObject*);
void nrnpy_decref_clear();

Object* nrnpy_po2ho(PyObject*);
static void py2n_component(Object*, Symbol*, int, int);
static void hpoasgn(Object*, int);
static int hoccommand_exec(Object*);
static void grphcmdtool(Object*, int, double, double, int);
static double guigetval(Object*);
static void guisetval(Object*, double);
static int guigetstr(Object*, char**);
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
	nrnpy_hoccommand_exec = hoccommand_exec;
	nrnpy_cmdtool = grphcmdtool;
	nrnpy_guigetval = guigetval;
	nrnpy_guisetval = guisetval;
	nrnpy_guigetstr = guigetstr;
	dlist = hoc_l_newlist();
}

Py2Nrn::Py2Nrn() {
	po_ = NULL;
	type_ = 0;	
//	printf("Py2Nrn() %lx\n", (long)this);
}
Py2Nrn::~Py2Nrn() {
	Py_XDECREF(po_);
//	printf("~Py2Nrn() %lx\n", (long)this);
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
		result = PyObject_CallObject(tail, args);
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
//printf("\n");
		on = nrnpy_po2ho(result);
		hoc_pop_defer();
		hoc_push_object(on);
		if (on) { 
			on->refcount--;
		}else{
			Py_XDECREF(result);
		}
//printf("%s refcount = %d\n", hoc_object_name(on), on->refcount);
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
	}
	Py_DECREF(poright);
	hoc_push_object(o);
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

static int hoccommand_exec(Object* ho) {
	PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
//printf("%s\n", hoc_object_name(ho));
	PyObject* r;
	nrnpython_ensure_threadstate();
	// should we use this instead?
	//PyGILState_STATE s = PyGILState_Ensure();
	if (PyTuple_Check(po)) {
//PyObject_Print(po, stdout, 0);
//printf("\n");
		PyObject* args = PyTuple_GetItem(po, 1);
		if (!PyTuple_Check(args)) {
			args = PyTuple_Pack(1, args);
		}
//PyObject_Print(PyTuple_GetItem(po, 0), stdout, 0);
//printf("\n");
//PyObject_Print(args, stdout, 0);
//printf("\n");
//printf("threadstate %lx\n", PyThreadState_GET());
		r = PyObject_CallObject(PyTuple_GetItem(po, 0), args);
	}else{
		r = PyObject_CallObject(po, PyTuple_New(0));
	}
	//PyGILState_Release(s);
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
	r = PyObject_CallObject(po, args);
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
