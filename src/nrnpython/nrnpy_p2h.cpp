#include <../../nrnconf.h>

#include <stdio.h>
#include <InterViews/resource.h>
#include <nrnoc2iv.h>
#include <classreg.h>
#include <nrnpython.h>
#include <hoccontext.h>
#include "nrnpy_utils.h"

#include "parse.hpp"
extern void hoc_nopop();
extern void hoc_pop_defer();
extern Object* hoc_new_object(Symbol*, void*);
extern int hoc_stack_type();
extern char** hoc_strpop();
extern Object** hoc_objpop();
extern Object* hoc_pop_object();
extern void hoc_tobj_unref(Object**);
extern int hoc_ipop();
PyObject* nrnpy_hoc2pyobject(Object*);
PyObject* hocobj_call_arg(int);
Object* nrnpy_pyobject_in_obj(PyObject*);
int nrnpy_ho_eq_po(Object*, PyObject*);
extern void* (*nrnpy_save_thread)();
extern void (*nrnpy_restore_thread)(void*);
static void* save_thread() { return PyEval_SaveThread(); }
static void restore_thread(void* g) {PyEval_RestoreThread((PyThreadState*)g); }
extern Symbol* nrnpy_pyobj_sym_;
extern void (*nrnpy_py2n_component)(Object*, Symbol*, int, int);
extern void (*nrnpy_hpoasgn)(Object*, int);
extern double (*nrnpy_praxis_efun)(Object*, Object*);
extern int (*nrnpy_hoccommand_exec)(Object*);
extern int (*nrnpy_hoccommand_exec_strret)(Object*, char*, int);
extern void (*nrnpy_cmdtool)(Object*, int, double, double, int);
extern double (*nrnpy_func_call)(Object*, int, int*);
extern Object* (*nrnpy_callable_with_args)(Object*, int);
extern double (*nrnpy_guigetval)(Object*);
extern void (*nrnpy_guisetval)(Object*, double);
extern int (*nrnpy_guigetstr)(Object*, char**);
extern char* (*nrnpy_po2pickle)(Object*, size_t* size);
extern Object* (*nrnpy_pickle2po)(char*, size_t size);
extern char* (*nrnpy_callpicklef)(char*, size_t size, int narg,
                                  size_t* retsize);
extern int (*nrnpy_pysame)(Object*, Object*);  // contain same Python object
extern Object* (*nrnpympi_alltoall_type)(int, int);
typedef struct {
  PyObject_HEAD Section* sec_;
  char* name_;
  PyObject* cell_;
} NPySecObj;
extern NPySecObj* newpysechelp(Section* sec);
extern void (*nrnpy_call_python_with_section)(Object*, Section*);
extern "C" void nrnpython_reg_real();
PyObject* nrnpy_ho2po(Object*);
void nrnpy_decref_defer(PyObject*);
PyObject* nrnpy_pyCallObject(PyObject*, PyObject*);

Object* nrnpy_po2ho(PyObject*);
static void py2n_component(Object*, Symbol*, int, int);
static void hpoasgn(Object*, int);
static double praxis_efun(Object*, Object*);
static int hoccommand_exec(Object*);
static int hoccommand_exec_strret(Object*, char*, int);
static void grphcmdtool(Object*, int, double, double, int);
static double func_call(Object*, int, int*);
static Object* callable_with_args(Object*, int);
static double guigetval(Object*);
static void guisetval(Object*, double);
static int guigetstr(Object*, char**);
static char* po2pickle(Object*, size_t* size);
static Object* pickle2po(char*, size_t size);
static char* call_picklef(char*, size_t size, int narg, size_t* retsize);
static Object* py_alltoall_type(int, int);
static int pysame(Object*, Object*);
static PyObject* main_module;
static PyObject* main_namespace;
static hoc_List* dlist;
#if NRNPYTHON_DYNAMICLOAD
extern int nrnpy_site_problem;
extern int* nrnpy_site_problem_p;
#endif

class Py2Nrn {
 public:
  Py2Nrn();
  virtual ~Py2Nrn();
  int type_;  // 0 toplevel
  PyObject* po_;
};

static void* p_cons(Object*) {
  Py2Nrn* p = new Py2Nrn();
  return p;
}

static void p_destruct(void* v) { delete (Py2Nrn*)v; }

Member_func p_members[] = {0,0};

static void call_python_with_section(Object* pyact, Section* sec) {
  PyObject* po = ((Py2Nrn*)pyact->u.this_pointer)->po_;
  PyObject* r;
  PyLockGIL lock;

  PyObject* args = PyTuple_Pack(1, (PyObject*) newpysechelp(sec));
  r = nrnpy_pyCallObject(po, args);
  Py_XDECREF(args);
  Py_XDECREF(r);
}


extern "C" void nrnpython_reg_real() {
  //printf("nrnpython_reg_real()\n");
  class2oc("PythonObject", p_cons, p_destruct, p_members, NULL, NULL, NULL);
  Symbol* s = hoc_lookup("PythonObject");
  assert(s);
  nrnpy_pyobj_sym_ = s;
  nrnpy_py2n_component = py2n_component;
  nrnpy_call_python_with_section = call_python_with_section;
  nrnpy_hpoasgn = hpoasgn;
  nrnpy_praxis_efun = praxis_efun;
  nrnpy_hoccommand_exec = hoccommand_exec;
  nrnpy_hoccommand_exec_strret = hoccommand_exec_strret;
  nrnpy_cmdtool = grphcmdtool;
  nrnpy_func_call = func_call;
  nrnpy_callable_with_args = callable_with_args;
  nrnpy_guigetval = guigetval;
  nrnpy_guisetval = guisetval;
  nrnpy_guigetstr = guigetstr;
  nrnpy_po2pickle = po2pickle;
  nrnpy_pickle2po = pickle2po;
  nrnpy_callpicklef = call_picklef;
  nrnpympi_alltoall_type = py_alltoall_type;
  nrnpy_pysame = pysame;
  nrnpy_save_thread = save_thread;
  nrnpy_restore_thread = restore_thread;
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
  PyLockGIL lock;
  Py_XDECREF(po_);
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
  HocTopContextSet PyObject* p = PyObject_CallObject(callable, args);
#if 0
printf("PyObject_CallObject callable\n");
PyObject_Print(callable, stdout, 0);
printf("\nargs\n");
PyObject_Print(args, stdout, 0);
printf("\nreturn %p\n", p);
if (p) { PyObject_Print(p, stdout, 0); printf("\n");}
#endif
  HocContextRestore return p;
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
  PyLockGIL lock;
  if (pn->type_ == 0) {  // top level
    if (!main_module) {
      main_module = PyImport_AddModule("__main__");
      main_namespace = PyModule_GetDict(main_module);
      Py_INCREF(main_module);
      Py_INCREF(main_namespace);
    }
    tail =
        PyRun_String(sym->name, Py_eval_input, main_namespace, main_namespace);
  } else {
    Py_INCREF(head);
    if (strcmp(sym->name, "_") == 0) {
      tail = head;
      Py_INCREF(tail);
    } else {
      tail = PyObject_GetAttrString(head, sym->name);
    }
  }
  if (!tail) {
    PyErr_Print();
    lock.release();
    hoc_execerror("No attribute:", sym->name);
  }
  PyObject* args = 0;
  Object* on;
  PyObject* result = 0;
  if (isfunc) {
    args = PyTuple_New(nindex);
    for (i = 0; i < nindex; ++i) {
      PyObject* arg = nrnpy_hoc_pop();
      // PyObject_Print(arg, stdout, 0);
      // printf(" %d   arg %d\n", arg->ob_refcnt,  i);
      if (PyTuple_SetItem(args, nindex - 1 - i, arg)) {
        assert(0);
      }
    }
    // printf("PyObject_CallObject %s %p\n", sym->name, tail);
    result = nrnpy_pyCallObject(tail, args);
    Py_DECREF(args);
    // PyObject_Print(result, stdout, 0);
    // printf("  result of call\n");
    if (!result) {
      PyErr_Print();
      lock.release();
      hoc_execerror("PyObject method call failed:", sym->name);
    }
  } else if (nindex) {
    PyObject* arg;
    if (hoc_stack_type() == NUMBER) {
      arg = Py_BuildValue("l", (long)hoc_xpop());
    } else {
      arg = nrnpy_hoc_pop();
    }
    result = PyObject_GetItem(tail, arg);
    if (!result) {
      PyErr_Print();
      lock.release();
      hoc_execerror("Python get item failed:", hoc_object_name(ob));
    }
  } else {
    result = tail;
    Py_INCREF(result);
  }
  // printf("py2n_component %s %d %s result refcount=%d\n", hoc_object_name(ob),
  // ob->refcount, sym->name, result->ob_refcnt);
  // if numeric, string, or python HocObject return those
  if (nrnpy_numbercheck(result)) {
    hoc_pop_defer();
    PyObject* pn = PyNumber_Float(result);
    hoc_pushx(PyFloat_AsDouble(pn));
    Py_XDECREF(pn);
    Py_XDECREF(result);
  } else if (is_python_string(result)) {
    char** ts = hoc_temp_charptr();
    Py2NRNString str(result, true);
    *ts = str.c_str();
    hoc_pop_defer();
    hoc_pushstr(ts);
    // how can we defer the result unref til the string is popped
    nrnpy_decref_defer(result);
  } else {
    // PyObject_Print(result, stdout, 0);
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
  int err = 0;
  int nindex;
  Symbol* sym;
  PyObject* poleft;
  PyObject* poright;
  if (type == NUMBER) {
    poright = PyFloat_FromDouble(hoc_xpop());
  } else if (type == STRING) {
    poright = Py_BuildValue("s", *hoc_strpop());
  } else if (type == OBJECTVAR || type == OBJECTTMP) {
    Object** po2 = hoc_objpop();
    poright = nrnpy_ho2po(*po2);
    hoc_tobj_unref(po2);
  } else {
    hoc_execerror("Cannot assign that type to PythonObject", (char*)0);
  }
  Object* stack_value = hoc_pop_object();
  assert(o == stack_value);
  poleft = nrnpy_hoc2pyobject(o);
  sym = hoc_spop();
  nindex = hoc_ipop();
  // printf("hpoasgn %s %s %d\n", hoc_object_name(o), sym->name, nindex);
  if (nindex == 0) {
    err = PyObject_SetAttrString(poleft, sym->name, poright);
  } else if (nindex == 1) {
    PyObject* key = PyLong_FromDouble(hoc_xpop());
    PyObject* a = PyObject_GetAttrString(poleft, sym->name);
    if (a) {
      err = PyObject_SetItem(a, key, poright);
      Py_DECREF(a);
    }else{
      err = -1;
    }
    Py_DECREF(key);
  } else {
    char buf[512];
    sprintf(buf, "%s.%s[][]...=...:", hoc_object_name(o), sym->name);
    hoc_execerror(
        buf,
        "HOC cannot handle PythonObject assignment with more than one index.");
  }
  Py_DECREF(poright);
  if (err) {
    PyErr_Print();
    hoc_execerror("Assignment to PythonObject failed", NULL);
  }
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

#if (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION == 3)
// copied from /Modules/_ctypes/_ctypes.c
static PyObject* PyTuple_Pack(int n, ...) {
  int i;
  PyObject* o;
  PyObject* result;
  PyObject** items;
  va_list vargs;

  va_start(vargs, n);
  result = PyTuple_New(n);
  if (result == NULL) return NULL;
  items = ((PyTupleObject*)result)->ob_item;
  for (i = 0; i < n; i++) {
    o = va_arg(vargs, PyObject*);
    Py_INCREF(o);
    items[i] = o;
  }
  va_end(vargs);
  return result;
}
#endif

static PyObject* hoccommand_exec_help1(PyObject* po) {
  PyObject* r;
  // PyObject_Print(po, stdout, 0);
  // printf("\n");
  if (PyTuple_Check(po)) {
    PyObject* args = PyTuple_GetItem(po, 1);
    if (!PyTuple_Check(args)) {
      args = PyTuple_Pack(1, args);
    }else{
      Py_INCREF(args);
    }
    // PyObject_Print(PyTuple_GetItem(po, 0), stdout, 0);
    // printf("\n");
    // PyObject_Print(args, stdout, 0);
    // printf("\n");
    // printf("threadstate %p\n", PyThreadState_GET());
    r = nrnpy_pyCallObject(PyTuple_GetItem(po, 0), args);
    Py_DECREF(args);
  } else {
    PyObject* args = PyTuple_New(0);
    r = nrnpy_pyCallObject(po, args);
    Py_DECREF(args);
  }
  if (r == NULL) {
    PyErr_Print();
  }
  return r;
}

static PyObject* hoccommand_exec_help(Object* ho) {
  PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
  // printf("%s\n", hoc_object_name(ho));
  return hoccommand_exec_help1(po);
}

static double praxis_efun(Object* ho, Object* v) {
  PyLockGIL lock;

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
  PyLockGIL lock;

  PyObject* r = hoccommand_exec_help(ho);
  if (r == NULL) {
    lock.release();
    hoc_execerror("Python Callback failed", 0);
  }
  Py_XDECREF(r);
  return (r != NULL);
}

static int hoccommand_exec_strret(Object* ho, char* buf, int size) {
  PyLockGIL lock;

  PyObject* r = hoccommand_exec_help(ho);
  if (r) {
    PyObject* pn = PyObject_Str(r);
    Py2NRNString str(pn);
    Py_XDECREF(pn);
    strncpy(buf, str.c_str(), size);
    buf[size - 1] = '\0';
    Py_XDECREF(r);
  } else {
    lock.release();
    hoc_execerror("Python Callback failed", 0);
  }
  return (r != NULL);
}

static void grphcmdtool(Object* ho, int type, double x, double y, int key) {
  PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
  PyObject* r;
  PyLockGIL lock;

  PyObject* args = PyTuple_Pack(4, PyInt_FromLong(type), PyFloat_FromDouble(x),
                                PyFloat_FromDouble(y), PyInt_FromLong(key));
  r = nrnpy_pyCallObject(po, args);
  Py_XDECREF(args);
  Py_XDECREF(r);
}

static Object* callable_with_args(Object* ho, int narg) {
  PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
  PyLockGIL lock;

  PyObject* args = PyTuple_New((Py_ssize_t)narg);
  if (args == NULL) {
    lock.release();
    hoc_execerror("PyTuple_New failed", 0);
  }
  for (int i = 0; i < narg; ++i) {
    PyObject* item = nrnpy_hoc_pop();
    if (item == NULL) {
      Py_XDECREF(args);
      lock.release();
      hoc_execerror("nrnpy_hoc_pop failed", 0);
    }
    if (PyTuple_SetItem(args, (Py_ssize_t)(narg - i - 1), item) != 0) {
      Py_XDECREF(args);
      lock.release();
      hoc_execerror("PyTuple_SetItem failed", 0);
    }
  }

  PyObject* r = PyTuple_New(2);
  PyTuple_SetItem(r, 1, args);
  Py_INCREF(po); // when r is destroyed, do not want po refcnt to go to 0
  PyTuple_SetItem(r, 0, po);

  Object* hr = nrnpy_po2ho(r);
  Py_XDECREF(r);

  return hr;
}

static double func_call(Object* ho, int narg, int* err) {
  PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
  PyObject* r;
  PyLockGIL lock;

  PyObject* args = PyTuple_New((Py_ssize_t)narg);
  if (args == NULL) {
    lock.release();
    hoc_execerror("PyTuple_New failed", 0);
  }
  for (int i = 0; i < narg; ++i) {
    PyObject* item = nrnpy_hoc_pop();
    if (item == NULL) {
      Py_XDECREF(args);
      lock.release();
      hoc_execerror("nrnpy_hoc_pop failed", 0);
    }
    if (PyTuple_SetItem(args, (Py_ssize_t)(narg - i - 1), item) != 0) {
      Py_XDECREF(args);
      lock.release();
      hoc_execerror("PyTuple_SetItem failed", 0);
    }
  }

  r = nrnpy_pyCallObject(po, args);
  Py_XDECREF(args);
  double rval = 0.0;
  if (r == NULL) {
    if (!err || *err) {
      PyErr_Print();
    }else{
      PyErr_Clear();
    }
    if (!err || *err) {
      lock.release();
      hoc_execerror("func_call failed", 0);
    }
    if (err) { *err = 1; }
  }else{
    if (nrnpy_numbercheck(r)) {
      PyObject* pn = PyNumber_Float(r);
      rval = PyFloat_AsDouble(pn);
      Py_XDECREF(pn);
    }
    Py_XDECREF(r);
    if (err) { *err = 0; } // success
  }
  return rval;
}

static double guigetval(Object* ho) {
  PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
  PyLockGIL lock;
  PyObject* r = NULL;
  PyObject* p = PyTuple_GetItem(po, 0);
  if (PySequence_Check(p) || PyMapping_Check(p)) {
    r = PyObject_GetItem(p, PyTuple_GetItem(po, 1));
  }else{
    r = PyObject_GetAttr(p, PyTuple_GetItem(po, 1));
  }
  PyObject* pn = PyNumber_Float(r);
  double x = PyFloat_AsDouble(pn);
  Py_XDECREF(pn);
  return x;
}

static void guisetval(Object* ho, double x) {
  PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
  PyLockGIL lock;
  PyObject* pn = PyFloat_FromDouble(x);
  PyObject* p = PyTuple_GetItem(po, 0);
  if (PySequence_Check(p) || PyMapping_Check(p)) {
    PyObject_SetItem(p, PyTuple_GetItem(po, 1), pn);
  }else{
    PyObject_SetAttr(p, PyTuple_GetItem(po, 1), pn);
  }
  Py_XDECREF(pn);
}

static int guigetstr(Object* ho, char** cpp) {
  PyObject* po = ((Py2Nrn*)ho->u.this_pointer)->po_;
  PyLockGIL lock;

  PyObject* r = PyObject_GetAttr(PyTuple_GetItem(po, 0),
                                 PyTuple_GetItem(po, 1));
  PyObject* pn = PyObject_Str(r);
  Py2NRNString name(pn);
  Py_DECREF(pn);
  char* cp = name.c_str();
  if (*cpp && strcmp(*cpp, cp) == 0) {
    return 0;
  }
  if (*cpp) {
    delete[] * cpp;
  }
  *cpp = new char[strlen(cp) + 1];
  strcpy(*cpp, cp);
  return 1;
}

static PyObject* loads;
static PyObject* dumps;

static void setpickle() {
    PyObject* pickle;
    if (!dumps) {
        #if PY_MAJOR_VERSION >= 3
            pickle = PyImport_ImportModule("pickle");
        #else
            pickle = PyImport_ImportModule("cPickle");
        #endif

        if (pickle) {
            Py_INCREF(pickle);
            dumps = PyObject_GetAttrString(pickle, "dumps");
            loads = PyObject_GetAttrString(pickle, "loads");
            if (dumps) {
                Py_INCREF(dumps);
                Py_INCREF(loads);
            }
        }
        if (!dumps || !loads) {
            hoc_execerror("Neither Python cPickle nor pickle are available", 0);
        }
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
  *size = PyString_Size(r) + 1;
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
  if (ho && ho->ctemplate->sym == nrnpy_pyobj_sym_) {
    PyObject* po = nrnpy_hoc2pyobject(ho);
    char* buf = pickle(po, size);
    return buf;
  } else {
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

/** Full python traceback error message returned as string.
 *  free the return value.
**/
char* nrnpyerr_str() {
  static char** pmes;
  if (PyErr_Occurred()) {
    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
    // try for full backtrace
    PyObject* module_name = PyString_FromString("traceback");
    if (!module_name) { return NULL; }
    PyObject* pyth_module = PyImport_Import(module_name);
    if (!pyth_module) { return NULL; }
    Py_DECREF(module_name);   
    PyObject* pyth_func = PyObject_GetAttrString(pyth_module, "format_exception");
    if (!pyth_func) { return NULL; }
    PyObject* pyth_val = PyObject_CallFunctionObjArgs(pyth_func, ptype, pvalue, ptraceback);
    if (!pyth_val) { return NULL; }
    Py_DECREF(pyth_func);
    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);
    PyObject* py_str = PyObject_Str(pyth_val);
    Py_DECREF(pyth_func);
    Py2NRNString mes(py_str);
    Py_DECREF(py_str);
    char* cmes = strdup(mes.c_str());
    return cmes;
  }
  return NULL;
}

char* call_picklef(char* fname, size_t size, int narg, size_t* retsize) {
  // fname is a pickled callable, narg is the number of args on the
  // hoc stack with types double, char*, hoc Vector, and PythonObject
  // callable return must be pickleable.
  PyObject* args = 0;
  PyObject* result = 0;
  PyObject* callable;

  setpickle();
#if PY_MAJOR_VERSION >= 3
  PyObject* ps = PyBytes_FromStringAndSize(fname, size);
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
    // Py_XDECREF(arg);
  }
  result = nrnpy_pyCallObject(callable, args);
  Py_DECREF(callable);
  Py_DECREF(args);
  if (!result) {
    char* mes = nrnpyerr_str();
    Fprintf(stderr, "%s\n", mes);
    free(mes);
    hoc_execerror("PyObject method call failed:", NULL);
  }
  char* rs = pickle(result, retsize);
  Py_XDECREF(result);
  return rs;
}

#include "nrnmpi.h"

int* mk_displ(int* cnts) {
  int* displ = new int[nrnmpi_numprocs + 1];
  displ[0] = 0;
  for (int i = 0; i < nrnmpi_numprocs; ++i) {
    displ[i + 1] = displ[i] + cnts[i];
  }
  return displ;
}

static PyObject* char2pylist(char* buf, int np, int* cnt, int* displ) {
  PyObject* plist = PyList_New(np);
  assert(plist != NULL);
  for (int i = 0; i < np; ++i) {
    if (cnt[i] == 0) {
      Py_INCREF(Py_None); // 'Fatal Python error: deallocating None' eventually
      PyList_SetItem(plist, i, Py_None);
    } else {
      PyObject* p = unpickle(buf + displ[i], cnt[i]);
      PyList_SetItem(plist, i, p);
    }
  }
  return plist;
}

#if NRNMPI
static PyObject* py_allgather(PyObject* psrc){
  int np = nrnmpi_numprocs;
  size_t sz;
  char* sbuf = pickle(psrc, &sz);
  // what are the counts from each rank
  int* rcnt = new int[np];
  rcnt[nrnmpi_myid] = int(sz);
  nrnmpi_int_allgather_inplace(rcnt, 1);
  int* rdispl = mk_displ(rcnt);
  char* rbuf = new char[rdispl[np]];

  nrnmpi_char_allgatherv(sbuf, rbuf, rcnt, rdispl);
  delete [] sbuf;
  
  PyObject* pdest = char2pylist(rbuf, np, rcnt, rdispl);
  delete [] rbuf;
  delete [] rcnt;
  delete [] rdispl;
  return pdest;
}

static PyObject* py_gather(PyObject* psrc, int root){
  int np = nrnmpi_numprocs;
  size_t sz;
  char* sbuf = pickle(psrc, &sz);
  // what are the counts from each rank
  int scnt = int(sz);
  int* rcnt = NULL;
  if (root == nrnmpi_myid) {
    rcnt = new int[np];
  }
  nrnmpi_int_gather(&scnt, rcnt, 1, root);
  int* rdispl = NULL;
  char* rbuf = NULL;
  if (root == nrnmpi_myid) {
    rdispl = mk_displ(rcnt);
    rbuf = new char[rdispl[np]];
  }

  nrnmpi_char_gatherv(sbuf, scnt, rbuf, rcnt, rdispl, root);
  delete [] sbuf;
  
  PyObject* pdest = Py_None;
  if (root == nrnmpi_myid) {
    pdest = char2pylist(rbuf, np, rcnt, rdispl);
    delete [] rbuf;
    delete [] rcnt;
    delete [] rdispl;
  }else{
    Py_INCREF(pdest);
  }
  return pdest;
}

static PyObject* py_broadcast(PyObject* psrc, int root){
  // Note: root returns reffed psrc.
  int np = nrnmpi_numprocs;
  char* buf = NULL;
  int cnt = 0;
  if (root == nrnmpi_myid) {
    size_t sz;
    buf = pickle(psrc, &sz);
    cnt = int(sz);
  }
  nrnmpi_int_broadcast(&cnt, 1, root);
  if (root != nrnmpi_myid) {
    buf = new char[cnt];
  }
  nrnmpi_char_broadcast(buf, cnt, root);
  PyObject* pdest = psrc;
  if (root != nrnmpi_myid) {
    pdest = unpickle(buf, size_t(cnt));
  }else{
    Py_INCREF(pdest);
  }
  delete [] buf;
  return pdest;
}
#endif

//type 1-alltoall, 2-allgather, 3-gather, 4-broadcast, 5-scatter
// size for 3, 4, 5 refer to rootrank.
Object* py_alltoall_type(int size, int type) {
  int np = nrnmpi_numprocs; // of subworld communicator
  PyObject* psrc = NULL;
  PyObject* pdest = NULL;

  if (type == 1 || type == 5) { // alltoall, scatter
    Object* o = *hoc_objgetarg(1);
   if (type == 1 || nrnmpi_myid == size) {// if scatter only root must be a list
    psrc = nrnpy_hoc2pyobject(o);
    if (!PyList_Check(psrc)) {
      hoc_execerror("Argument must be a Python list", 0);
    }
    if (PyList_Size(psrc) != np) {
      if (type == 1) {
        hoc_execerror("py_alltoall list size must be nhost", 0);
      }else{
        hoc_execerror("py_scatter list size must be nhost", 0);
      }
    }
   }
    if (np == 1) {
      if (type == 1) {
        return o;
      }else{ // return psrc[0]
        pdest = PyList_GetItem(psrc, 0);
	Py_INCREF(pdest);
        Object* ho = nrnpy_po2ho(pdest);
        if (ho) {
          --ho->refcount;
        }
        Py_XDECREF(pdest);
        return ho;
      }
    }
  }else{
    // Get the raw PyObject* arg. So things like None, int, bool are preserved.
    psrc = hocobj_call_arg(0);
    Py_INCREF(psrc);

    if (np == 1) {
      if (type == 4) { // broadcast is just the PyObject
        pdest = psrc;
      }else{ // allgather and gather must wrap psrc in list
        pdest = PyList_New(1);
        PyList_SetItem(pdest, 0, psrc);
      }
      Object* ho = nrnpy_po2ho(pdest);
      if (ho) {
        --ho->refcount;
      }
      Py_XDECREF(pdest);
      return ho;
    }
  }

#if NRNMPI
  setpickle();
  int root;

 if (type == 2) {
    pdest = py_allgather(psrc);
    Py_DECREF(psrc);
 }else if (type != 1 && type != 5) {
    root = size;
    if (root < 0 || root >= np) {
      hoc_execerror("root rank must be >= 0 and < nhost", 0);
    }
    if (type == 3) {
      pdest = py_gather(psrc, root);
    }else if (type == 4) {
      pdest = py_broadcast(psrc, root);
    }
    Py_DECREF(psrc);
 }else{

  if (type == 5) { // scatter
    root = size;
    size = 0; // calculate dest size (cannot be -1 so cannot return it)
  }

  char* s = NULL;
  int* scnt = NULL;
  int* sdispl = NULL;
  char* r = NULL;
  int* rcnt = NULL;
  int* rdispl = NULL;

        // setup source buffer for transfer s, scnt, sdispl
        // for alltoall, each rank handled identically
        // for scatter, root handled as list all, other ranks handled as None
        if (type == 1 || nrnmpi_myid == root) { // psrc is list of nhost items

  scnt = new int[np];
  for (int i = 0; i < np; ++i) {
    scnt[i] = 0;
  }

  PyObject* iterator = PyObject_GetIter(psrc);
  PyObject* p;

  size_t bufsz = 100000;  // 100k buffer to start with
  if (size > 0) {         // or else the positive number specified
    bufsz = size;
  }
  if (size >= 0) {  // otherwise count only
    s = new char[bufsz];
  }
  int curpos = 0;
  for (int i = 0; (p = PyIter_Next(iterator)) != NULL; ++i) {
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
        delete[] s;
        s = s2;
      }
      for (int j = 0; j < sz; ++j) {
        s[curpos + j] = b[j];
      }
    }
    curpos += sz;
    scnt[i] = sz;
    delete[] b;
    Py_DECREF(p);
  }
  Py_DECREF(iterator);

        // scatter equivalent to alltoall NONE list for not root ranks.
        }else if (type == 5 && nrnmpi_myid != root){
  // nothing to setup, s, scnt, sdispl already NULL
	}

        // destinations need to know receive counts. Then transfer data.
        if (type == 1) { // alltoall

  // what are destination counts
  int* ones = new int[np];
  for (int i = 0; i < np; ++i) {
    ones[i] = 1;
  }
  sdispl = mk_displ(ones);
  rcnt = new int[np];
  nrnmpi_int_alltoallv(scnt, ones, sdispl, rcnt, ones, sdispl);
  delete[] ones;
  delete[] sdispl;

  // exchange
  sdispl = mk_displ(scnt);
  rdispl = mk_displ(rcnt);
  char* r = 0;
  if (size < 0) {
    pdest = PyTuple_New(2);
    PyTuple_SetItem(pdest, 0, Py_BuildValue("l", (long)sdispl[np]));
    PyTuple_SetItem(pdest, 1, Py_BuildValue("l", (long)rdispl[np]));
    delete[] scnt;
    delete[] sdispl;
    delete[] rcnt;
    delete[] rdispl;
  } else {
    r = new char[rdispl[np] + 1]; //force > 0 for all None case
    nrnmpi_char_alltoallv(s, scnt, sdispl, r, rcnt, rdispl);
    delete[] s;
    delete[] scnt;
    delete[] sdispl;

    pdest = char2pylist(r, np, rcnt, rdispl);

    delete[] r;
    delete[] rcnt;
    delete[] rdispl;
  }

        }else{ // scatter

  // destination counts
  rcnt = new int[1];
  nrnmpi_int_scatter(scnt, rcnt, 1, root);
  r = new char[rcnt[0] + 1]; // rcnt[0] can be 0

  // exchange
  if (nrnmpi_myid == root) {sdispl = mk_displ(scnt);}
  nrnmpi_char_scatterv(s, scnt, sdispl, r, rcnt[0], root);
  if (s) delete[] s;
  if (scnt) delete[] scnt;
  if (sdispl) delete[] sdispl;

  if (rcnt[0]) {
    pdest = unpickle(r, size_t(rcnt[0]));
  }else{
    pdest = Py_None;
    Py_INCREF(pdest);
  }

  delete[] r;
  delete[] rcnt;
  assert (rdispl == NULL);

        }

 }
  Object* ho = nrnpy_po2ho(pdest);
  Py_XDECREF(pdest);
  if (ho) {
    --ho->refcount;
  }
  return ho;
#else
  assert(0);
  return NULL;
#endif
}
