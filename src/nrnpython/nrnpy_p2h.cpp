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
PyObject* nrnpy_hoc2pyobject(Object*);
Object* nrnpy_pyobject_in_obj(PyObject*);
extern Symbol* nrnpy_pyobj_sym_;
extern void (*nrnpy_py2n_component)(Object*, Symbol*, int, int);
PyObject* nrnpy_ho2po(Object*);
Object* nrnpy_po2ho(PyObject*);
static void py2n_component(Object*, Symbol*, int, int);
static PyObject* main_module;
static PyObject* main_namespace;
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

void nrnpython_reg() {
//	printf("nrnpython_reg()\n");
	class2oc("PythonObject", p_cons, p_destruct, p_members);
	Symbol* s = hoc_lookup("PythonObject");
	nrnpy_pyobj_sym_ = s;
	nrnpy_py2n_component = py2n_component;
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
	if (!po) { po = main_module; }
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

