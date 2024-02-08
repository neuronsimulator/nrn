#include "ivocvect.h"
#include "neuron/container/data_handle.hpp"
#include "nrniv_mf.h"
#include "nrn_pyhocobject.h"
#include "nrnoc2iv.h"
#include "nrnpy.h"
#include "nrnpy_utils.h"
#include "nrnpython.h"
#include <unordered_map>

#include "nrnwrap_dlfcn.h"
#include "ocfile.h"
#include "ocjump.h"
#include "oclist.h"
#include "shapeplt.h"

#include <InterViews/resource.h>
#include <structmember.h>  // for PyMemberDef

#include <cstdint>
#include <vector>
#include <sstream>

extern PyTypeObject* psection_type;
extern std::vector<const char*> py_exposed_classes;

#include "parse.hpp"
extern void (*nrnpy_sectionlist_helper_)(void*, Object*);
extern void* (*nrnpy_get_pyobj)(Object* obj);
extern void (*nrnpy_restore_savestate)(int64_t, char*);
extern void (*nrnpy_store_savestate)(char** save_data, uint64_t* save_data_size);
extern void (*nrnpy_decref)(void* pyobj);
extern void lvappendsec_and_ref(void* sl, Section* sec);
extern Section* nrn_noerr_access();
extern void hoc_pushs(Symbol*);
extern double* hoc_evalpointer();
extern double cable_prop_eval(Symbol* sym);
extern void nrn_change_nseg(Section*, int);
extern Symlist* hoc_top_level_symlist;
extern Symlist* hoc_built_in_symlist;
extern Inst* hoc_pc;
extern void hoc_push_string();
extern char** hoc_strpop();
extern void hoc_objectvar();
extern Object* hoc_newobj1(Symbol*, int);
extern int ivoc_list_count(Object*);
extern Object** hoc_objpop();
extern Object* hoc_obj_look_inside_stack(int);
extern void hoc_object_component();
extern int nrn_inpython_;
extern int hoc_stack_type();
extern void hoc_call();
extern Objectdata* hoc_top_level_data;
extern void hoc_tobj_unref(Object**);
extern void hoc_unref_defer();
extern void sec_access_push();
extern PyObject* nrnpy_pushsec(PyObject*);
extern bool hoc_valid_stmt(const char*, Object*);
PyObject* nrnpy_nrn();
extern PyObject* nrnpy_cas(PyObject*, PyObject*);
extern PyObject* nrnpy_forall(PyObject*, PyObject*);
extern PyObject* nrnpy_newsecobj(PyObject*, PyObject*, PyObject*);
extern int section_object_seen;
extern Symbol* nrn_child_sym;
extern int nrn_secref_nchild(Section*);
static void pyobject_in_objptr(Object**, PyObject*);
extern IvocVect* (*nrnpy_vec_from_python_p_)(void*);
extern Object** (*nrnpy_vec_to_python_p_)(void*);
extern Object** (*nrnpy_vec_as_numpy_helper_)(int, double*);
extern Object* (*nrnpy_rvp_rxd_to_callable)(Object*);
extern "C" int nrnpy_set_vec_as_numpy(PyObject* (*p)(int, double*) );  // called by ctypes.
extern "C" int nrnpy_set_gui_callback(PyObject*);
extern Symbol* ivoc_alias_lookup(const char* name, Object* ob);
class NetCon;
extern int nrn_netcon_weight(NetCon*, double**);
extern int nrn_matrix_dim(void*, int);

extern PyObject* pmech_types;  // Python map for name to Mechanism
extern PyObject* rangevars_;   // Python map for name to Symbol

extern int hoc_max_builtin_class_id;
extern int hoc_return_type_code;

static cTemplate* hoc_vec_template_;
static cTemplate* hoc_list_template_;
static cTemplate* hoc_sectionlist_template_;

static std::unordered_map<Symbol*, PyTypeObject*> sym_to_type_map;
static std::unordered_map<PyTypeObject*, Symbol*> type_to_sym_map;
static std::vector<std::string> exposed_py_type_names;

// typestr returned by Vector.__array_interface__
// byteorder (first element) is modified at import time
// to reflect the system byteorder
// Allocate one extra character space in case we have a two character integer of
// bytes per double
// i.e. 16
static char array_interface_typestr[5] = "|f8";

// static pointer to neurons.doc.get_docstring function initialized at import
// time
static PyObject* pfunc_get_docstring = NULL;

// Methods unique to the HocTopLevelInterpreter type of HocObject
// follow the add_methods implementation of python3.6.2 in typeobject.c
// and the GenericGetAttr implementation in object.c
static PyObject* topmethdict = NULL;
static void add2topdict(PyObject*);

static const char* hocobj_docstring = "class neuron.hoc.HocObject - Hoc Object wrapper";

#if 1
#include <hoccontext.h>
#else
extern Object* hoc_thisobject;
#define HocTopContextSet  \
    if (hoc_thisobject) { \
        abort();          \
    }                     \
    assert(hoc_thisobject == 0);
#define HocContextRestore /**/
#endif

static PyObject* rvp_plot = NULL;
static PyObject* plotshape_plot = NULL;
static PyObject* cpp2refstr(char** cpp);
static PyObject* get_mech_object_ = NULL;
static PyObject* nrnpy_rvp_pyobj_callback = NULL;

PyTypeObject* hocobject_type;

static PyObject* hocobj_call(PyHocObject* self, PyObject* args, PyObject* kwrds);

bool nrn_chk_data_handle(const neuron::container::data_handle<double>& pd) {
    if (pd) {
        return true;
    }
    PyErr_SetString(PyExc_ValueError, "Invalid data_handle");
    return false;
}

/** @brief if hoc_evalpointer calls hoc_execerror, return 1
 **/
static int hoc_evalpointer_err() {
    try {
        hoc_evalpointer();
    } catch (std::exception const& e) {
        std::ostringstream oss;
        oss << "subscript out of range (array size or number of dimensions changed?)";
        PyErr_SetString(PyExc_IndexError, oss.str().c_str());
        return 1;
    }
    return 0;
}

static PyObject* nrnexec(PyObject* self, PyObject* args) {
    const char* cmd;
    if (!PyArg_ParseTuple(args, "s", &cmd)) {
        return NULL;
    }
    bool b = hoc_valid_stmt(cmd, 0);
    return Py_BuildValue("i", b ? 1 : 0);
}

static PyObject* hoc_ac(PyObject* self, PyObject* args) {
    PyArg_ParseTuple(args, "|d", &hoc_ac_);
    return Py_BuildValue("d", hoc_ac_);
}

static PyMethodDef HocMethods[] = {
    {"execute", nrnexec, METH_VARARGS, "Execute a hoc command, return 1 on success, 0 on failure."},
    {"hoc_ac", hoc_ac, METH_VARARGS, "Get (or set) the scalar hoc_ac_."},
    {NULL, NULL, 0, NULL}};

static void hocobj_dealloc(PyHocObject* self) {
    // printf("hocobj_dealloc %p\n", self);
    if (self->ho_) {
        hoc_obj_unref(self->ho_);
    }
    if (self->type_ == PyHoc::HocRefStr && self->u.s_) {
        // delete [] self->u.s_;
        free(self->u.s_);
    }
    if (self->type_ == PyHoc::HocRefObj && self->u.ho_) {
        hoc_obj_unref(self->u.ho_);
    }
    if (self->indices_) {
        delete[] self->indices_;
    }
    if (self->type_ == PyHoc::HocRefPStr && self->u.pstr_) {
        // nothing deleted
    }
    ((PyObject*) self)->ob_type->tp_free((PyObject*) self);

    // Deferred deletion of HOC Objects is unnecessary when a HocObject is
    // destroyed. And we would like to have prompt deletion if this HocObject
    // wrapped a HOC Object whose refcount was 1.
    hoc_unref_defer();
}

static PyObject* hocobj_new(PyTypeObject* subtype, PyObject* args, PyObject* kwds) {
    PyObject* subself;
    PyObject* base;
    PyHocObject* hbase = nullptr;

    subself = subtype->tp_alloc(subtype, 0);
    // printf("hocobj_new %s %p %p\n", subtype->tp_name, subtype, subself);
    if (subself == NULL) {
        return NULL;
    }
    PyHocObject* self = (PyHocObject*) subself;
    self->ho_ = NULL;
    self->u.x_ = 0.;
    self->sym_ = NULL;
    self->indices_ = NULL;
    self->nindex_ = 0;
    self->type_ = PyHoc::HocTopLevelInterpreter;
    self->iteritem_ = 0;

    // if subtype is a subclass of some NEURON class, then one of its
    // tp_mro's is in sym_to_type_map
    for (Py_ssize_t i = 0; i < PyTuple_Size(subtype->tp_mro); i++) {
        PyObject* item = PyTuple_GetItem(subtype->tp_mro, i);
        auto symbol_result = type_to_sym_map.find((PyTypeObject*) item);
        if (symbol_result != type_to_sym_map.end()) {
            hbase = (PyHocObject*) hocobj_new(hocobject_type, 0, 0);
            hbase->type_ = PyHoc::HocFunction;
            hbase->sym_ = symbol_result->second;
            break;
        }
    }

    if (kwds && PyDict_Check(kwds) && (base = PyDict_GetItemString(kwds, "hocbase"))) {
        if (PyObject_TypeCheck(base, hocobject_type)) {
            hbase = (PyHocObject*) base;
        } else {
            PyErr_SetString(PyExc_TypeError, "HOC base class not valid");
            Py_DECREF(subself);
            return NULL;
        }
        PyDict_DelItemString(kwds, "hocbase");
    }

    if (hbase and hbase->type_ == PyHoc::HocFunction && hbase->sym_->type == TEMPLATE) {
        // printf("hocobj_new base %s\n", hbase->sym_->name);
        // remove the hocbase keyword since hocobj_call only allows
        // the "sec" keyword argument
        PyObject* r = hocobj_call(hbase, args, kwds);
        if (!r) {
            Py_DECREF(subself);
            return NULL;
        }
        PyHocObject* rh = (PyHocObject*) r;
        self->type_ = rh->type_;
        self->ho_ = rh->ho_;
        hoc_obj_ref(self->ho_);
        Py_DECREF(r);
    }

    return subself;
}

static int hocobj_init(PyObject* subself, PyObject* args, PyObject* kwds) {
// printf("hocobj_init %s %p\n",
// ((PyTypeObject*)PyObject_Type(subself))->tp_name, subself);
#if 0
	if (subself != NULL) {
		PyHocObject* self = (PyHocObject*)subself;
		if (self->ho_) { hoc_obj_unref(self->ho_); }
		self->ho_ = NULL;
		self->u.x_ = 0.;
		self->sym_ = NULL;
		self->indices_ = NULL;
		self->nindex_ = 0;
		self->type_ = 0;
	}
#endif
    return 0;
}

static void pyobject_in_objptr(Object** op, PyObject* po) {
    Object* o = nrnpy_pyobject_in_obj(po);
    if (*op) {
        hoc_obj_unref(*op);
    }
    *op = o;
}

static PyObject* hocobj_name(PyObject* pself, PyObject* args) {
    auto* const self = reinterpret_cast<PyHocObject*>(pself);
    std::string cp;
    if (self->type_ == PyHoc::HocObject) {
        cp = hoc_object_name(self->ho_);
    } else if (self->type_ == PyHoc::HocFunction || self->type_ == PyHoc::HocArray) {
        if (self->ho_) {
            cp.append(hoc_object_name(self->ho_));
            cp.append(1, '.');
        }
        cp.append(self->sym_->name);
        if (self->type_ == PyHoc::HocArray) {
            for (int i = 0; i < self->nindex_; ++i) {
                cp.append(1, '[');
                cp.append(std::to_string(self->indices_[i]));
                cp.append(1, ']');
            }
            cp.append("[?]");
        } else {
            cp.append("()");
        }
    } else if (self->type_ == PyHoc::HocRefNum) {
        cp.append("<hoc ref value ");
        cp.append(std::to_string(self->u.x_));
        cp.append(1, '>');
    } else if (self->type_ == PyHoc::HocRefStr) {
        cp.append("<hoc ref str \"");
        cp.append(self->u.s_);
        cp.append("\">");
    } else if (self->type_ == PyHoc::HocRefPStr) {
        cp.append("<hoc ref pstr \"");
        cp.append(*self->u.pstr_);
        cp.append("\">");
    } else if (self->type_ == PyHoc::HocRefObj) {
        cp.append("<hoc ref value \"");
        cp.append(hoc_object_name(self->u.ho_));
        cp.append("\">");
    } else if (self->type_ == PyHoc::HocForallSectionIterator) {
        cp.append("<all section iterator next>");
    } else if (self->type_ == PyHoc::HocSectionListIterator) {
        cp.append("<SectionList iterator>");
    } else if (self->type_ == PyHoc::HocScalarPtr) {
        std::ostringstream oss;
        oss << self->u.px_;
        cp = std::move(oss).str();
    } else if (self->type_ == PyHoc::HocArrayIncomplete) {
        cp.append("<incomplete pointer to hoc array ");
        cp.append(self->sym_->name);
        cp.append(1, '>');
    } else {
        cp.append("<TopLevelHocInterpreter>");
    }
    return Py_BuildValue("s", cp.c_str());
}

static PyObject* hocobj_repr(PyObject* p) {
    return hocobj_name(p, NULL);
}

static Inst* save_pc(Inst* newpc) {
    Inst* savpc = hoc_pc;
    hoc_pc = newpc;
    return savpc;
}

// also called from nrnpy_nrn.cpp
int hocobj_pushargs(PyObject* args, std::vector<char*>& s2free) {
    int i, narg = PyTuple_Size(args);
    for (i = 0; i < narg; ++i) {
        PyObject* po = PyTuple_GetItem(args, i);
        // PyObject_Print(po, stdout, 0);
        // printf("  pushargs %d\n", i);
        if (nrnpy_numbercheck(po)) {
            PyObject* pn = PyNumber_Float(po);
            hoc_pushx(PyFloat_AsDouble(pn));
            Py_XDECREF(pn);
        } else if (is_python_string(po)) {
            char** ts = hoc_temp_charptr();
            Py2NRNString str(po, /* disable_release */ true);
            if (str.err()) {
                // Since Python error has been set, need to clear, or hoc_execerror
                // printing with nrnpy_pr will generate a
                // Exception ignored on calling ctypes callback function.
                // So get the message, clear, and make the message
                // part of the execerror.
                *ts = str.get_pyerr();
                s2free.push_back(*ts);
                hoc_execerr_ext("python string arg cannot decode into c_str. Pyerr message: %s",
                                *ts);
            }
            *ts = str.c_str();
            s2free.push_back(*ts);
            hoc_pushstr(ts);
        } else if (PyObject_TypeCheck(po, hocobject_type)) {
            // The PyObject_TypeCheck above used to be PyObject_IsInstance. The
            // problem with the latter is that it calls the __class__ method of
            // the object which can raise an error for nrn.Section, nrn.Segment,
            // etc. if the internal Section is invalid (Section.prop == NULL).
            // That, in consequence, will generate an
            // Exception ignored on calling ctypes callback function: <function nrnpy_pr
            // thus obscuring the actual error, such as
            // nrn.Segment associated with deleted internal Section.
            PyHocObject* pho = (PyHocObject*) po;
            PyHoc::ObjectType tp = pho->type_;
            if (tp == PyHoc::HocObject) {
                hoc_push_object(pho->ho_);
            } else if (tp == PyHoc::HocRefNum) {
                hoc_pushpx(&pho->u.x_);
            } else if (tp == PyHoc::HocRefStr) {
                hoc_pushstr(&pho->u.s_);
            } else if (tp == PyHoc::HocRefObj) {
                hoc_pushobj(&pho->u.ho_);
            } else if (tp == PyHoc::HocScalarPtr) {
                if (!pho->u.px_) {
                    hoc_execerr_ext("Invalid pointer (arg %d)", i);
                }
                hoc_push(pho->u.px_);
            } else if (tp == PyHoc::HocRefPStr) {
                hoc_pushstr(pho->u.pstr_);
            } else {
                // make a hoc python object and push that
                Object* ob = NULL;
                pyobject_in_objptr(&ob, po);
                hoc_push_object(ob);
                hoc_obj_unref(ob);
            }
        } else {  // make a hoc PythonObject and push that?
            Object* ob = NULL;
            if (po != Py_None) {
                pyobject_in_objptr(&ob, po);
            }
            hoc_push_object(ob);
            hoc_obj_unref(ob);
        }
    }
    return narg;
}

void hocobj_pushargs_free_strings(std::vector<char*>& s2free) {
    std::vector<char*>::iterator it = s2free.begin();
    for (; it != s2free.end(); ++it) {
        if (*it) {
            free(*it);
        }
    }
    s2free.clear();
}

static Symbol* getsym(char* name, Object* ho, int fail) {
    Symbol* sym = 0;
    if (ho) {
        sym = hoc_table_lookup(name, ho->ctemplate->symtable);
        if (!sym && strcmp(name, "delay") == 0) {
            sym = hoc_table_lookup("del", ho->ctemplate->symtable);
        } else if (!sym && ho->aliases) {
            sym = ivoc_alias_lookup(name, ho);
        }
    } else {
        sym = hoc_table_lookup(name, hoc_top_level_symlist);
        if (!sym) {
            sym = hoc_table_lookup(name, hoc_built_in_symlist);
        }
    }
    if (sym && sym->type == UNDEF) {
        sym = 0;
    }
    if (!sym && fail) {
        char e[200];
        Sprintf(e, "'%s' is not a defined hoc variable name.", name);
        PyErr_SetString(PyExc_LookupError, e);
    }
    return sym;
}

// on entry the stack order is indices, args, object
// on exit all that is popped and the result is on the stack
// returns hoc_return_is_int if called on a builtin (i.e. 2 if bool, 1 if int, 0 otherwise)
static int component(PyHocObject* po) {
    Inst fc[6];
    int var_type = 0;
    hoc_return_type_code = 0;
    fc[0].sym = po->sym_;
    fc[1].i = 0;
    fc[2].i = 0;
    fc[5].i = 0;
    int stk_offset = 0;  // scalar
    if (po->type_ == PyHoc::HocFunction) {
        fc[2].i = po->nindex_;
        fc[5].i = 1;
        stk_offset = po->nindex_;
    } else if (po->type_ == PyHoc::HocArray || po->type_ == PyHoc::HocArrayIncomplete) {
        fc[1].i = po->nindex_;
        stk_offset = po->nindex_ + 1;  // + 1 because of stack_ndim_datum
    }
    Object* stack_value = hoc_obj_look_inside_stack(stk_offset);
    assert(stack_value == po->ho_);
    fc[3].i = po->ho_->ctemplate->id;
    fc[4].sym = po->sym_;
    Inst* pcsav = save_pc(fc);
    hoc_object_component();
    hoc_pc = pcsav;
    // only return a type if we're calling a built-in (these are all registered first)
    if (po->ho_->ctemplate->id <= hoc_max_builtin_class_id) {
        var_type = hoc_return_type_code;
    }
    hoc_return_type_code = 0;
    return (var_type);
}

int nrnpy_numbercheck(PyObject* po) {
    // PyNumber_Check can return 1 for things that should not reasonably
    // be cast to float but should stay as python objects.
    // e.g. numpy.arange(1,5) and 5J
    // The complexity here is partly due to SAGE having its own
    // number system, e.g. type(1) is <type 'sage.rings.integer.Integer'>
    int rval = PyNumber_Check(po);
    // but do not allow sequences
    if (rval == 1 && po->ob_type->tp_as_sequence) {
        rval = 0;
    }
    // or things that fail when float(po) fails. ARGGH! This
    // is a lot more expensive than I would like.
    if (rval == 1) {
        PyObject* tmp = PyNumber_Float(po);
        if (tmp) {
            Py_DECREF(tmp);
        } else {
            PyErr_Clear();
            rval = 0;
        }
    }
    return rval;
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
    } else if (o->ctemplate->sym == nrnpy_pyobj_sym_) {
        po = nrnpy_hoc2pyobject(o);
        Py_INCREF(po);
    } else {
        po = hocobj_new(hocobject_type, 0, 0);
        ((PyHocObject*) po)->ho_ = o;
        ((PyHocObject*) po)->type_ = PyHoc::HocObject;
        auto location = sym_to_type_map.find(o->ctemplate->sym);
        if (location != sym_to_type_map.end()) {
            Py_INCREF(location->second);
            po->ob_type = location->second;
        }
        hoc_obj_ref(o);
    }
    return po;
}

// not static because it's used in nrnpy_nrn.cpp
Object* nrnpy_po2ho(PyObject* po) {
    // po may be None, or encapsulate a hoc object (via the
    // PyHocObject, or be a native Python instance such as [1,2,3]
    // The return value is None, or the encapsulated hoc object,
    // or a hoc object of type PythonObject that encapsulates the
    // po.
    Object* o;
    if (po == Py_None) {
        o = 0;
    } else if (PyObject_TypeCheck(po, hocobject_type)) {
        PyHocObject* pho = (PyHocObject*) po;
        if (pho->type_ == PyHoc::HocObject) {
            o = pho->ho_;
            hoc_obj_ref(o);
        } else if (pho->type_ == PyHoc::HocRefObj) {
            o = pho->u.ho_;
            hoc_obj_ref(o);
        } else {
            // all the rest encapsulate
            o = nrnpy_pyobject_in_obj(po);
        }
    } else {  // even if Python string or number
        o = nrnpy_pyobject_in_obj(po);
    }
    return o;
}

PyObject* nrnpy_hoc_pop(const char* mes) {
    PyObject* result = 0;
    Object* ho;
    Object** d;
    switch (hoc_stack_type()) {
    case STRING:
        result = Py_BuildValue("s", *hoc_strpop());
        break;
    case VAR: {
        // remove mes arg when test coverage development completed
        // printf("VAR nrnpy_hoc_pop %s\n", mes);
        auto const px = hoc_pop_handle<double>();
        if (nrn_chk_data_handle(px)) {
            // unfortunately, this is nonsense if NMODL POINTER is pointing
            // to something other than a double.
            result = Py_BuildValue("d", *px);
        }
    } break;
    case NUMBER:
        result = Py_BuildValue("d", hoc_xpop());
        break;
    case OBJECTVAR:
    case OBJECTTMP:
        d = hoc_objpop();
        ho = *d;
        // printf("Py2Nrn %p %p\n", ho->ctemplate->sym, nrnpy_pyobj_sym_);
        result = nrnpy_ho2po(ho);
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
        } else {
            err = 1;
        }
        break;
    case VAR: {
        if (double x; PyArg_Parse(po, "d", &x) == 1) {
            auto px = hoc_pop_handle<double>();
            if (px) {
                // This is a future crash if NMODL POINTER is pointing
                // to something other than a double.
                *px = x;
            } else {
                PyErr_SetString(PyExc_AttributeError, "POINTER is NULL");
                return -1;
            }
        } else {
            err = 1;
        }
    } break;
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
        } else {
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


static void* nrnpy_hoc_int_pop() {
    return (void*) Py_BuildValue("i", (long) hoc_xpop());
}

static void* nrnpy_hoc_bool_pop() {
    return (void*) PyBool_FromLong((long) hoc_xpop());
}

static void* fcall(void* vself, void* vargs) {
    PyHocObject* self = (PyHocObject*) vself;
    if (self->ho_) {
        hoc_push_object(self->ho_);
    }

    // TODO: this will still have some memory leaks in case of errors.
    //      see discussion in https://github.com/neuronsimulator/nrn/pull/1437
    std::vector<char*> strings_to_free;

    int narg = hocobj_pushargs((PyObject*) vargs, strings_to_free);
    int var_type;
    if (self->ho_) {
        self->nindex_ = narg;
        var_type = component(self);
        hocobj_pushargs_free_strings(strings_to_free);
        switch (var_type) {
        case 2:
            return nrnpy_hoc_bool_pop();
        case 1:
            return nrnpy_hoc_int_pop();
        default:
            // No callable hoc function returns a data handle.
            return nrnpy_hoc_pop("self->ho_ fcall");
        }
    }
    if (self->sym_->type == BLTIN) {
        if (narg != 1) {
            hoc_execerror("must be one argument for", self->sym_->name);
        }
        double d = hoc_call_func(self->sym_, 1);
        hoc_pushx(d);
    } else if (self->sym_->type == TEMPLATE) {
        Object* ho = hoc_newobj1(self->sym_, narg);
        PyHocObject* result = (PyHocObject*) hocobj_new(hocobject_type, 0, 0);
        result->ho_ = ho;
        result->type_ = PyHoc::HocObject;
        // Note: I think the only reason we're not using ho2po here is because we don't have to
        //       hocref ho since it was created by hoc_newobj1... but it would be better if we did
        //       so we could avoid repetitive code
        auto location = sym_to_type_map.find(ho->ctemplate->sym);
        if (location != sym_to_type_map.end()) {
            Py_INCREF(location->second);
            ((PyObject*) result)->ob_type = location->second;
        }

        hocobj_pushargs_free_strings(strings_to_free);
        return result;
    } else {
        HocTopContextSet
        Inst fc[4];
        // ugh. so a potential call of hoc_get_last_pointer_symbol will return nullptr.
        fc[0].in = STOP;
        fc[1].sym = self->sym_;
        fc[2].i = narg;
        fc[3].in = STOP;
        Inst* pcsav = save_pc(fc + 1);
        hoc_call();
        hoc_pc = pcsav;
        HocContextRestore;
    }
    hocobj_pushargs_free_strings(strings_to_free);

    return nrnpy_hoc_pop("laststatement fcall");
}

static PyObject* curargs_;

PyObject* hocobj_call_arg(int i) {
    return PyTuple_GetItem(curargs_, i);
}

static PyObject* hocobj_call(PyHocObject* self, PyObject* args, PyObject* kwrds) {
    // Hack to allow some python only methods to get the python args.
    // without losing info about type bool, int, etc.
    // eg pc.py_broadcast, pc.py_gather, pc.py_allgather
    PyObject* prevargs_ = curargs_;
    curargs_ = args;

    PyObject* section = 0;
    PyObject* result{};
    if (kwrds && PyDict_Check(kwrds)) {
#if 0
		PyObject* keys = PyDict_Keys(kwrds);
		assert(PyList_Check(keys));
		int n = PyList_Size(keys);
		for (int i = 0; i < n; ++i) {
			PyObject* key = PyList_GetItem(keys, i);
			PyObject* value = PyDict_GetItem(kwrds, key);
			printf("%s %s\n", PyUnicode_AsUTF8(key), PyUnicode_AsUTF8(PyObject_Str(value)));
		}
#endif
        section = PyDict_GetItemString(kwrds, "sec");
        int num_kwargs = PyDict_Size(kwrds);
        if (num_kwargs > 1) {
            PyErr_SetString(PyExc_RuntimeError, "invalid keyword argument");
            curargs_ = prevargs_;
            return NULL;
        }
        if (section) {
            if (PyObject_TypeCheck(section, psection_type)) {
                Section* sec = ((NPySecObj*) section)->sec_;
                if (!sec->prop) {
                    nrnpy_sec_referr();
                    curargs_ = prevargs_;
                    return NULL;
                }
                nrn_pushsec(sec);
            } else {
                PyErr_SetString(PyExc_TypeError, "sec is not a Section");
                curargs_ = prevargs_;
                return NULL;
            }
        } else {
            if (num_kwargs) {
                PyErr_SetString(PyExc_RuntimeError, "invalid keyword argument");
                curargs_ = prevargs_;
                return NULL;
            }
        }
    }
    if (self->type_ == PyHoc::HocTopLevelInterpreter) {
        result = nrnexec((PyObject*) self, args);
    } else if (self->type_ == PyHoc::HocFunction) {
        try {
            result = static_cast<PyObject*>(OcJump::fpycall(fcall, self, args));
        } catch (std::exception const& e) {
            std::ostringstream oss;
            oss << "hocobj_call error: " << e.what();
            PyErr_SetString(PyExc_RuntimeError, oss.str().c_str());
        }
        hoc_unref_defer();
    } else {
        PyErr_SetString(PyExc_TypeError, "object is not callable");
        curargs_ = prevargs_;
        return NULL;
    }
    if (section) {
        nrn_popsec();
    }
    curargs_ = prevargs_;
    return result;
}

static Arrayinfo* hocobj_aray(Symbol* sym, Object* ho) {
    if (!sym->arayinfo) {
        return nullptr;
    }
    if (ho) {  // objectdata or not?
        int cplus = (ho->ctemplate->sym->subtype & (CPLUSOBJECT | JAVAOBJECT));
        if (cplus) {
            return sym->arayinfo;
        } else {
            return ho->u.dataspace[sym->u.oboff + 1].arayinfo;
        }
    } else {
        if (sym->type == VAR &&
            (sym->subtype == USERDOUBLE || sym->subtype == USERINT || sym->subtype == USERFLOAT)) {
            return sym->arayinfo;
        } else {
            return hoc_top_level_data[sym->u.oboff + 1].arayinfo;
        }
    }
}

static PyHocObject* intermediate(PyHocObject* po, Symbol* sym, int ix) {
    PyHocObject* ponew = (PyHocObject*) hocobj_new(hocobject_type, 0, 0);
    if (po->ho_) {
        ponew->ho_ = po->ho_;
        hoc_obj_ref(po->ho_);
    }
    if (ix > -1) {  // array, increase dimension by one
        int j;
        assert(po->sym_ == sym);
        assert(po->type_ == PyHoc::HocArray || po->type_ == PyHoc::HocArrayIncomplete);
        ponew->sym_ = sym;
        ponew->nindex_ = po->nindex_ + 1;
        ponew->type_ = po->type_;
        ponew->indices_ = new int[ponew->nindex_];
        for (j = 0; j < po->nindex_; ++j) {
            ponew->indices_[j] = po->indices_[j];
        }
        ponew->indices_[po->nindex_] = ix;
    } else {  // make it an array (no indices yet)
        ponew->sym_ = sym;
        ponew->type_ = PyHoc::HocArray;
    }
    return ponew;
}

// when called, nindex is 1 less than reality
static void hocobj_pushtop(PyHocObject* po, Symbol* sym, int ix) {
    int i;
    int n = po->nindex_++;
    // printf("hocobj_pushtop n=%d", po->nindex_);
    for (i = 0; i < n; ++i) {
        hoc_pushx((double) po->indices_[i]);
        // printf(" %d", po->indices_[i]);
    }
    hoc_pushx((double) ix);
    // printf(" %d\n", ix);
    hoc_push_ndim(n + 1);
    if (sym) {
        hoc_pushs(sym);
    }
}

static int hocobj_objectvar(Symbol* sym) {
    int err{0};
    try {
        Inst fc;
        fc.sym = sym;
        Inst* pcsav = save_pc(&fc);
        hoc_objectvar();
        hoc_pc = pcsav;
    } catch (std::exception const& e) {
        std::ostringstream oss;
        oss << "number of dimensions error:" << e.what();
        PyErr_SetString(PyExc_IndexError, oss.str().c_str());
        err = 1;
    }
    return err;
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

PyObject* nrn_hocobj_handle(neuron::container::data_handle<double> d) {
    PyObject* result = hocobj_new(hocobject_type, 0, 0);
    auto* const po = reinterpret_cast<PyHocObject*>(result);
    po->type_ = PyHoc::HocScalarPtr;
    po->u.px_ = d;
    return result;
}

extern "C" PyObject* nrn_hocobj_ptr(double* pd) {
    return nrn_hocobj_handle(neuron::container::data_handle<double>{pd});
}

int nrn_is_hocobj_ptr(PyObject* po, neuron::container::data_handle<double>& pd) {
    int ret = 0;
    if (PyObject_TypeCheck(po, hocobject_type)) {
        auto* const hpo = reinterpret_cast<PyHocObject*>(po);
        if (hpo->type_ == PyHoc::HocScalarPtr) {
            pd = hpo->u.px_;
            ret = 1;
        }
    }
    return ret;
}

static void symlist2dict(Symlist* sl, PyObject* dict) {
    PyObject* nn = Py_BuildValue("");
    for (Symbol* s = sl->first; s; s = s->next) {
        if (s->type == UNDEF) {
            continue;
        }
        if (s->cpublic == 1 || sl == hoc_built_in_symlist || sl == hoc_top_level_symlist) {
            if (strcmp(s->name, "del") == 0) {
                PyDict_SetItemString(dict, "delay", nn);
            } else {
                PyDict_SetItemString(dict, s->name, nn);
            }
        }
    }
    Py_DECREF(nn);
}

static int setup_doc_system() {
    PyObject* pdoc;
    if (pfunc_get_docstring) {
        return 1;
    }
    pdoc = PyImport_ImportModule("neuron.doc");
    if (pdoc == NULL) {
        PyErr_SetString(PyExc_ImportError, "Failed to import neuron.doc documentation module.");
        return 0;
    }
    pfunc_get_docstring = PyObject_GetAttrString(pdoc, "get_docstring");

    if (pfunc_get_docstring == NULL) {
        PyErr_SetString(PyExc_AttributeError,
                        "neuron.doc module does not have attribute 'get_docstring'!");
        return 0;
    }
    return 1;
}

PyObject* toplevel_get(PyObject* subself, const char* n) {
    PyHocObject* self = (PyHocObject*) subself;
    PyObject* result = NULL;
    if (self->type_ == PyHoc::HocTopLevelInterpreter) {
        PyObject* descr = PyDict_GetItemString(topmethdict, n);
        if (descr) {
            Py_INCREF(descr);
            descrgetfunc f = descr->ob_type->tp_descr_get;
            assert(f);
            result = f(descr, subself, (PyObject*) Py_TYPE(subself));
            Py_DECREF(descr);
        }
    }
    return result;
}

// TODO: This function needs refactoring; there are too many exit points
static PyObject* hocobj_getattr(PyObject* subself, PyObject* pyname) {
    PyHocObject* self = (PyHocObject*) subself;
    if (self->type_ == PyHoc::HocObject && !self->ho_) {
        PyErr_SetString(PyExc_TypeError, "not a compound type");
        return NULL;
    }

    PyObject* result = NULL;
    int isptr = 0;
    Py2NRNString name(pyname);
    char* n = name.c_str();
    if (!n) {
        name.set_pyerr(PyExc_TypeError, "attribute name must be a string");
        return NULL;
    }

    Symbol* sym = getsym(n, self->ho_, 0);
    // Return well known types right away
    auto location = sym_to_type_map.find(sym);
    if (location != sym_to_type_map.end()) {
        Py_INCREF(location->second);
        return (PyObject*) location->second;
    }

    if (!sym) {
        if (self->type_ == PyHoc::HocObject && self->ho_->ctemplate->sym == nrnpy_pyobj_sym_) {
            PyObject* p = nrnpy_hoc2pyobject(self->ho_);
            return PyObject_GenericGetAttr(p, pyname);
        }
        if (self->type_ == PyHoc::HocTopLevelInterpreter) {
            result = toplevel_get(subself, n);
            if (result) {
                return result;
            }
        }
        if (strcmp(n, "__dict__") == 0) {
            // all the public names
            Symlist* sl = 0;
            if (self->ho_) {
                sl = self->ho_->ctemplate->symtable;
            } else if (self->sym_ && self->sym_->type == TEMPLATE) {
                sl = self->sym_->u.ctemplate->symtable;
            }
            PyObject* dict = PyDict_New();
            if (sl) {
                symlist2dict(sl, dict);
            } else {
                symlist2dict(hoc_built_in_symlist, dict);
                symlist2dict(hoc_top_level_symlist, dict);
                add2topdict(dict);
            }

            // Is the self->ho_ a Vector?  If so, add the __array_interface__ symbol

            if (is_obj_type(self->ho_, "Vector")) {
                PyDict_SetItemString(dict, "__array_interface__", Py_None);
            } else if (is_obj_type(self->ho_, "RangeVarPlot") ||
                       is_obj_type(self->ho_, "PlotShape")) {
                PyDict_SetItemString(dict, "plot", Py_None);
            }
            return dict;
        } else if (strncmp(n, "_ref_", 5) == 0) {
            if (self->type_ > PyHoc::HocObject) {
                PyErr_SetString(PyExc_TypeError, "not a HocTopLevelInterpreter or HocObject");
                return NULL;
            }
            sym = getsym(n + 5, self->ho_, 0);
            if (!sym) {
                return PyObject_GenericGetAttr((PyObject*) subself, pyname);
            }
            if (sym->type == STRING) {
                Objectdata* od = hoc_objectdata_save();
                if (self->type_ == PyHoc::HocTopLevelInterpreter) {
                    hoc_objectdata = hoc_top_level_data;
                } else if (self->type_ == PyHoc::HocObject && !self->ho_->ctemplate->constructor) {
                    hoc_objectdata = self->ho_->u.dataspace;
                } else {
                    hoc_objectdata = hoc_objectdata_restore(od);
                    assert(0);
                    return NULL;
                }
                char** cpp = OPSTR(sym);
                hoc_objectdata = hoc_objectdata_restore(od);
                result = cpp2refstr(cpp);
                return result;
            } else if (sym->type != VAR && sym->type != RANGEVAR && sym->type != VARALIAS) {
                char buf[200];
                Sprintf(buf,
                        "Hoc pointer error, %s is not a hoc variable or range variable or strdef",
                        sym->name);
                PyErr_SetString(PyExc_TypeError, buf);
                return NULL;
            } else {
                isptr = 1;
            }
        } else if (is_obj_type(self->ho_, "Vector") && strcmp(n, "__array_interface__") == 0) {
            // return __array_interface__
            // printf("building array interface\n");
            Vect* v = (Vect*) self->ho_->u.this_pointer;
            int size = v->size();
            double* x = vector_vec(v);

            return Py_BuildValue("{s:(i),s:s,s:i,s:(N,O)}",
                                 "shape",
                                 size,
                                 "typestr",
                                 array_interface_typestr,
                                 "version",
                                 3,
                                 "data",
                                 PyLong_FromVoidPtr(x),
                                 Py_True);

        } else if (is_obj_type(self->ho_, "RangeVarPlot") && strcmp(n, "plot") == 0) {
            return PyObject_CallFunctionObjArgs(rvp_plot, (PyObject*) self, NULL);
        } else if (is_obj_type(self->ho_, "PlotShape") && strcmp(n, "plot") == 0) {
            return PyObject_CallFunctionObjArgs(plotshape_plot, (PyObject*) self, NULL);
        } else if (strcmp(n, "__doc__") == 0) {
            if (setup_doc_system()) {
                PyObject* docobj = NULL;
                if (self->ho_) {
                    docobj = Py_BuildValue("s s",
                                           self->ho_->ctemplate->sym->name,
                                           self->sym_ ? self->sym_->name : "");
                } else if (self->sym_) {
                    // Symbol
                    docobj = Py_BuildValue("s s", "", self->sym_->name);
                } else {
                    // Base HocObject

                    docobj = Py_BuildValue("s s", "", "");
                }

                result = PyObject_CallObject(pfunc_get_docstring, docobj);
                Py_DECREF(docobj);
                return result;
            } else {
                return NULL;
            }
        } else if (self->type_ == PyHoc::HocTopLevelInterpreter &&
                   strncmp(n, "__nrnsec_0x", 11) == 0) {
            Section* sec = (Section*) hoc_sec_internal_name2ptr(n, 0);
            if (sec == NULL) {
                PyErr_SetString(PyExc_NameError, n);
            } else if (sec && sec->prop && sec->prop->dparam[PROP_PY_INDEX].get<void*>()) {
                result = static_cast<PyObject*>(sec->prop->dparam[PROP_PY_INDEX].get<void*>());
                Py_INCREF(result);
            } else {
                nrn_pushsec(sec);
                result = nrnpy_cas(NULL, NULL);
                nrn_popsec();
            }
            return result;
        } else if (self->type_ == PyHoc::HocTopLevelInterpreter && strncmp(n, "__pysec_", 8) == 0) {
            Section* sec = (Section*) hoc_pysec_name2ptr(n, 0);
            if (sec == NULL) {
                PyErr_SetString(PyExc_NameError, n);
            } else if (sec && sec->prop && sec->prop->dparam[PROP_PY_INDEX].get<void*>()) {
                result = static_cast<PyObject*>(sec->prop->dparam[PROP_PY_INDEX].get<void*>());
                Py_INCREF(result);
            } else {
                nrn_pushsec(sec);
                result = nrnpy_cas(NULL, NULL);
                nrn_popsec();
            }
            return result;
        } else {
            // ipython wants to know if there is a __getitem__
            // even though it does not use it.
            return PyObject_GenericGetAttr((PyObject*) subself, pyname);
        }
    }
    // printf("%s type=%d nindex=%d %s\n", self->sym_?self->sym_->name:"noname",
    // self->type_, self->nindex_, sym->name);
    // no hoc component for a hoc function
    // ie the sym has to be a component for the object returned by the function
    if (self->type_ == PyHoc::HocFunction) {
        PyErr_SetString(PyExc_TypeError,
                        "No hoc method for a callable. Missing parentheses before the '.'?");
        return NULL;
    }
    if (self->type_ == PyHoc::HocArray) {
        PyErr_SetString(PyExc_TypeError, "Missing array index");
        return NULL;
    }
    if (self->ho_) {  // use the component fork.
        result = hocobj_new(hocobject_type, 0, 0);
        PyHocObject* po = (PyHocObject*) result;
        po->ho_ = self->ho_;
        hoc_obj_ref(po->ho_);
        po->sym_ = sym;
        // evaluation deferred unless VAR,STRING,OBJECTVAR and not
        // an array
        int t = sym->type;
        if (t == VAR || t == STRING || t == OBJECTVAR || t == RANGEVAR || t == SECTION ||
            t == SECTIONREF || t == VARALIAS || t == OBJECTALIAS || t == RANGEOBJ) {
            if (sym != nrn_child_sym && !ISARRAY(sym)) {
                hoc_push_object(po->ho_);
                nrn_inpython_ = 1;
                component(po);
                if (nrn_inpython_ == 2) {  // error in component
                    nrn_inpython_ = 0;
                    PyErr_SetString(PyExc_TypeError, "No value");
                    Py_DECREF(po);
                    return NULL;
                }
                nrn_inpython_ = 0;
                Py_DECREF(po);
                if (t == SECTION || t == SECTIONREF) {
                    section_object_seen = 0;
                    result = nrnpy_cas(0, 0);
                    nrn_popsec();
                    return result;
                } else {
                    if (isptr) {
                        auto handle = hoc_pop_handle<double>();
                        return nrn_hocobj_handle(std::move(handle));
                    } else {
                        return nrnpy_hoc_pop("use-the-component-fork hocobj_getattr");
                    }
                }
            } else {
                if (isptr) {
                    po->type_ = PyHoc::HocArrayIncomplete;
                } else {
                    po->type_ = PyHoc::HocArray;
                }
                return result;
            }
        } else {
            po->type_ = PyHoc::HocFunction;
            return result;
        }
    }
    // top level interpreter fork
    HocTopContextSet
    switch (sym->type) {
    case VAR:  // double*
        if (!ISARRAY(sym)) {
            if (sym->subtype == USERINT) {
                result = Py_BuildValue("i", *(sym->u.pvalint));
                break;
            }
            if (sym->subtype == USERPROPERTY) {
                if (!nrn_noerr_access()) {
                    PyErr_SetString(PyExc_TypeError, "Section access unspecified");
                    break;
                }
                if (!isptr) {
                    if (sym->u.rng.type == CABLESECTION) {
                        result = Py_BuildValue("d", cable_prop_eval(sym));
                    } else {
                        result = Py_BuildValue("i", int(cable_prop_eval(sym)));
                    }
                    break;
                } else if (sym->u.rng.type != CABLESECTION) {
                    PyErr_SetString(PyExc_TypeError, "Cannot be a reference");
                    break;
                }
            }
            hoc_pushs(sym);
            hoc_evalpointer();
            if (isptr) {
                result = nrn_hocobj_ptr(hoc_pxpop());
            } else {
                result = Py_BuildValue("d", *hoc_pxpop());
            }
        } else {
            result = (PyObject*) intermediate(self, sym, -1);
            if (isptr) {
                ((PyHocObject*) result)->type_ = PyHoc::HocArrayIncomplete;
            } else {
            }
        }
        break;
    case STRING:  // char*
    {
        Inst fc, *pcsav;
        fc.sym = sym;
        pcsav = save_pc(&fc);
        hoc_push_string();
        hoc_pc = pcsav;
        result = Py_BuildValue("s", *hoc_strpop());
    } break;
    case OBJECTVAR:  // Object*
        if (!ISARRAY(sym)) {
            Inst fc, *pcsav;
            fc.sym = sym;
            pcsav = save_pc(&fc);
            hoc_objectvar();
            hoc_pc = pcsav;
            Object* ho = *hoc_objpop();
            result = nrnpy_ho2po(ho);
        } else {
            result = (PyObject*) intermediate(self, sym, -1);
        }
        break;
    case SECTION:
        if (!ISARRAY(sym)) {
            result = hocobj_getsec(sym);
        } else {
            result = (PyObject*) intermediate(self, sym, -1);
        }
        break;
    case PROCEDURE:
    case FUNCTION:
    case FUN_BLTIN:
    case BLTIN:
    case HOCOBJFUNCTION:
    case STRINGFUNC:
    case TEMPLATE:
    case OBJECTFUNC: {
        result = hocobj_new(hocobject_type, 0, 0);
        PyHocObject* po = (PyHocObject*) result;
        if (self->ho_) {
            po->ho_ = self->ho_;
            hoc_obj_ref(po->ho_);
        }
        po->type_ = PyHoc::HocFunction;
        po->sym_ = sym;
        // printf("function %s\n", po->sym_->name);
        break;
    }
    case SETPOINTERKEYWORD:
        result = toplevel_get(subself, n);
        break;
    default:  // otherwise
    {
        if (PyDict_GetItemString(pmech_types, n)) {
            result = PyObject_CallFunction(get_mech_object_, "s", n);
            break;
        } else if (PyDict_GetItemString(rangevars_, n)) {
            PyErr_Format(PyExc_TypeError,
                         "Cannot access %s directly; it is a range variable and may be accessed "
                         "via a section or segment.",
                         n);
        } else {
            PyErr_Format(PyExc_TypeError,
                         "Cannot access %s (NEURON type %d) directly.",
                         n,
                         sym->type);
            break;
        }
    }
    }
    HocContextRestore
    return result;
}

static PyObject* hocobj_baseattr(PyObject* subself, PyObject* args) {
    PyObject* name;
    if (!PyArg_ParseTuple(args, "O", &name)) {
        return NULL;
    }
    return hocobj_getattr(subself, name);
}

static int refuse_to_look;
static PyObject* hocobj_getattro(PyObject* subself, PyObject* name) {
    PyObject* result = 0;
    if ((PyTypeObject*) PyObject_Type(subself) != hocobject_type) {
        // printf("try generic %s\n", PyString_AsString(name));
        result = PyObject_GenericGetAttr(subself, name);
        if (result) {
            // printf("found generic %s\n", PyString_AsString(name));
            return result;
        } else {
            PyErr_Clear();
        }
    }
    if (!refuse_to_look) {
        result = hocobj_getattr(subself, name);
    }
    return result;
}

static int hocobj_setattro(PyObject* subself, PyObject* pyname, PyObject* value) {
    int err = 0;
    Inst* pcsav;
    Inst fc;

    int issub = ((PyTypeObject*) PyObject_Type(subself) != hocobject_type);
    if (issub) {
        // printf("try hasattr %s\n", PyString_AsString(name));
        refuse_to_look = 1;
        if (PyObject_HasAttr(subself, pyname)) {
            refuse_to_look = 0;
            // printf("found hasattr for %s\n", PyString_AsString(name));
            return PyObject_GenericSetAttr(subself, pyname, value);
        }
        refuse_to_look = 0;
    }

    PyHocObject* self = (PyHocObject*) subself;
    PyHocObject* po;

    if (self->type_ == PyHoc::HocObject && !self->ho_) {
        return 1;
    }
    Py2NRNString name(pyname);
    char* n = name.c_str();
    if (!n) {
        name.set_pyerr(PyExc_TypeError, "attribute name must be a string");
        return -1;
    }
    // printf("hocobj_setattro %s\n", n);
    Symbol* sym = getsym(n, self->ho_, 0);
    if (!sym) {
        if (issub) {
            return PyObject_GenericSetAttr(subself, pyname, value);
        } else if (!sym && self->type_ == PyHoc::HocObject &&
                   self->ho_->ctemplate->sym == nrnpy_pyobj_sym_) {
            PyObject* p = nrnpy_hoc2pyobject(self->ho_);
            return PyObject_GenericSetAttr(p, pyname, value);
        } else if (strncmp(n, "_ref_", 5) == 0) {
            Symbol* rvsym = getsym(n + 5, self->ho_, 0);
            if (rvsym && rvsym->type == RANGEVAR) {
                Prop* prop = ob2pntproc_0(self->ho_)->prop;
                if (!prop) {
                    PyErr_SetString(PyExc_TypeError, "Point_process not located in a section");
                    return -1;
                }
                err = nrn_pointer_assign(prop, rvsym, value);
                return err;
            }
            sym = getsym(n, self->ho_, 1);
        } else {
            sym = getsym(n, self->ho_, 1);
        }
    }
    if (!sym) {
        return -1;
    }
    if (self->ho_) {  // use the component fork.
        PyObject* result = hocobj_new(hocobject_type, 0, 0);
        po = (PyHocObject*) result;
        po->ho_ = self->ho_;
        hoc_obj_ref(po->ho_);
        po->sym_ = sym;
        // evaluation deferred unless VAR,STRING,OBJECTVAR and not
        // an array
        int t = sym->type;
        if (t == VAR || t == STRING || t == OBJECTVAR || t == RANGEVAR || t == VARALIAS ||
            t == OBJECTALIAS) {
            if (!ISARRAY(sym)) {
                hoc_push_object(po->ho_);
                nrn_inpython_ = 1;
                component(po);
                if (nrn_inpython_ == 2) {  // error in component
                    nrn_inpython_ = 0;
                    PyErr_SetString(PyExc_TypeError, "No value");
                    Py_DECREF(po);
                    return -1;
                }
                Py_DECREF(po);
                return set_final_from_stk(value);
            } else {
                char e[200];
                Sprintf(e, "'%s' requires subscript for assignment", n);
                PyErr_SetString(PyExc_TypeError, e);
                Py_DECREF(po);
                return -1;
            }
        } else {
            PyErr_SetString(PyExc_TypeError, "not assignable");
            Py_DECREF(po);
            return -1;
        }
    }
    HocTopContextSet
    switch (sym->type) {
    case VAR:  // double*
        if (ISARRAY(sym)) {
            PyErr_SetString(PyExc_TypeError, "Wrong number of subscripts");
            err = -1;
        } else {
            if (sym->subtype == USERINT) {
                err = PyArg_Parse(value, "i", sym->u.pvalint) == 0;
            } else if (sym->subtype == USERPROPERTY) {
                if (!nrn_noerr_access()) {
                    PyErr_SetString(PyExc_TypeError, "Section access unspecified");
                    err = -1;
                    break;
                }
                double x;
                if (sym->u.rng.type != CABLESECTION) {
                    int i;
                    if (PyArg_Parse(value, "i", &i) != 0 && i > 0 && i <= 32767) {
                        x = double(i);
                    } else {
                        PyErr_SetString(PyExc_ValueError,
                                        "nseg must be an integer in range 1 to 32767");
                        err = -1;
                    }
                } else {
                    err = PyArg_Parse(value, "d", &x) == 0;
                }
                if (!err) {
                    cable_prop_assign(sym, &x, 0);
                }
            } else {
                hoc_pushs(sym);
                if (hoc_evalpointer_err()) {  // not possible to raise error.
                    HocContextRestore;
                    return -1;
                }
                err = PyArg_Parse(value, "d", hoc_pxpop()) == 0;
            }
        }
        break;
    case STRING:  // char*
        fc.sym = sym;
        pcsav = save_pc(&fc);
        hoc_push_string();
        hoc_pc = pcsav;
        char* s;
        if (PyArg_Parse(value, "s", &s) == 1) {
            hoc_assign_str(hoc_strpop(), s);
        } else {
            err = 1;
        }
        break;
    case OBJECTVAR:  // Object*
    {
        err = hocobj_objectvar(sym);
        if (err) {
            break;
        }
        Object** op;
        op = hoc_objpop();
        PyObject* po;
        PyHocObject* pho;
        if (PyArg_Parse(value, "O", &po) == 1) {
            if (po == Py_None) {
                hoc_obj_unref(*op);
                *op = 0;
            } else if (PyObject_TypeCheck(po, hocobject_type)) {
                pho = (PyHocObject*) po;
                if (pho->sym_) {
                    PyErr_SetString(PyExc_TypeError,
                                    "argument cannot be a hoc object intermediate");
                    err = -1;
                } else {
                    hoc_obj_ref(pho->ho_);
                    hoc_obj_unref(*op);
                    *op = pho->ho_;
                }
            } else {  // it is a PythonObject in hoc
                pyobject_in_objptr(op, po);
            }
        } else {
            err = 1;
        }
        break;
    }
    default:
        PyErr_SetString(PyExc_TypeError, "not assignable");
        err = -1;
        break;
    }
    HocContextRestore
    return err;
}

static Symbol* sym_vec_x;
static Symbol* sym_mat_x;
static Symbol* sym_netcon_weight;

static int araylen(Arrayinfo* a, PyHocObject* po) {
    int nsub = a ? a->nsub : 0;
    if (nsub <= po->nindex_) {
        std::ostringstream oss;
        oss << "Too many subscripts (Redeclared the array?), hoc var " << po->sym_->name
            << " now has " << nsub << " but trying to access dimension " << (po->nindex_);
        PyErr_SetString(PyExc_TypeError, oss.str().c_str());
        return -1;
    }
    int n = 0;
    // Hoc Vector and Matrix are special cases because the sub[]
    // do not get filled in til just before hoc_araypt is called.
    // at least check the vector
    if (po->sym_ == sym_vec_x) {
        n = vector_capacity((IvocVect*) po->ho_->u.this_pointer);
    } else if (po->sym_ == sym_netcon_weight) {
        double* w;
        n = nrn_netcon_weight(static_cast<NetCon*>(po->ho_->u.this_pointer), &w);
    } else if (po->sym_ == nrn_child_sym) {
        n = nrn_secref_nchild((Section*) po->ho_->u.this_pointer);
    } else if (po->sym_ == sym_mat_x) {
        n = nrn_matrix_dim(po->ho_->u.this_pointer, po->nindex_);
    } else {
        n = a->sub[po->nindex_];
    }
    return n;
}

static int araychk(Arrayinfo* a, PyHocObject* po, int ix) {
    int n = araylen(a, po);
    if (n < 0) {
        return -1;
    }
    if (ix < 0 || n <= ix) {
        // printf("ix=%d nsub=%d nindex=%d sub[nindex]=%d\n", ix, a->nsub,
        // po->nindex_, a->sub[po->nindex_]);
        char e[200];
        Sprintf(e,
                "%s%s%s",
                po->ho_ ? hoc_object_name(po->ho_) : "",
                (po->ho_ && po->sym_) ? "." : "",
                po->sym_ ? po->sym_->name : "");
        PyErr_SetString(PyExc_IndexError, e);
        return -1;
    }
    return 0;
}

static Py_ssize_t hocobj_len(PyObject* self) {
    PyHocObject* po = (PyHocObject*) self;
    if (po->type_ == PyHoc::HocObject) {
        if (po->ho_->ctemplate == hoc_vec_template_) {
            return vector_capacity((Vect*) po->ho_->u.this_pointer);
        } else if (po->ho_->ctemplate == hoc_list_template_) {
            return ivoc_list_count(po->ho_);
        } else if (po->ho_->ctemplate == hoc_sectionlist_template_) {
            PyErr_SetString(PyExc_TypeError, "hoc.SectionList has no len()");
            return -1;
        }
    } else if (po->type_ == PyHoc::HocArray) {
        Arrayinfo* a = hocobj_aray(po->sym_, po->ho_);
        return araylen(a, po);
    } else if (po->sym_ && po->sym_->type == TEMPLATE) {
        return po->sym_->u.ctemplate->count;
    } else if (po->type_ == PyHoc::HocForallSectionIterator) {
        PyErr_SetString(PyExc_TypeError, "hoc all section iterator() has no len()");
        return -1;
    } else if (po->type_ == PyHoc::HocSectionListIterator) {
        PyErr_SetString(PyExc_TypeError, "hoc SectionList iterator() has no len()");
        return -1;
    }
    PyErr_SetString(PyExc_TypeError, "Most HocObject have no len()");
    return -1;
}

static int hocobj_nonzero(PyObject* self) {
    // printf("hocobj_nonzero\n");
    PyHocObject* po = (PyHocObject*) self;
    int b = 1;
    if (po->type_ == PyHoc::HocObject) {
        if (po->ho_->ctemplate == hoc_vec_template_) {
            b = vector_capacity((Vect*) po->ho_->u.this_pointer) > 0;
        } else if (po->ho_->ctemplate == hoc_list_template_) {
            b = ivoc_list_count(po->ho_) > 0;
        }
    } else if (po->type_ == PyHoc::HocArray) {
        Arrayinfo* a = hocobj_aray(po->sym_, po->ho_);
        int i = araylen(a, po);
        if (i < 0) {
            return -1;
        }
        b = i > 0;
    } else if (po->sym_ && po->sym_->type == TEMPLATE) {
        b = 1;  // prior behavior: po->sym_->u.ctemplate->count > 0;
    }
    return b;
}

PyObject* nrnpy_forall(PyObject* self, PyObject* args) {
    PyObject* po = hocobj_new(hocobject_type, 0, 0);
    PyHocObject* pho = (PyHocObject*) po;
    pho->type_ = PyHoc::HocForallSectionIterator;
    pho->u.its_ = PyHoc::Begin;
    pho->iteritem_ = section_list;
    return po;
}

static PyObject* hocobj_iter(PyObject* self) {
    //	printf("hocobj_iter %p\n", self);
    PyHocObject* po = (PyHocObject*) self;
    if (po->type_ == PyHoc::HocObject) {
        if (po->ho_->ctemplate == hoc_vec_template_) {
            return PySeqIter_New(self);
        } else if (po->ho_->ctemplate == hoc_list_template_) {
            return PySeqIter_New(self);
        } else if (po->ho_->ctemplate == hoc_sectionlist_template_) {
            // need a clone of self so nested loops do not share iteritem_
            PyObject* po2 = nrnpy_ho2po(po->ho_);
            PyHocObject* pho2 = (PyHocObject*) po2;
            pho2->type_ = PyHoc::HocSectionListIterator;
            pho2->u.its_ = PyHoc::Begin;
            pho2->iteritem_ = ((hoc_Item*) po->ho_->u.this_pointer);
            return po2;
        }
    } else if (po->type_ == PyHoc::HocForallSectionIterator) {
        po->iteritem_ = section_list;
        po->u.its_ = PyHoc::Begin;
        Py_INCREF(self);
        return self;
    } else if (po->type_ == PyHoc::HocArray) {
        return PySeqIter_New(self);
    } else if (po->sym_ && po->sym_->type == TEMPLATE) {
        po->iteritem_ = po->sym_->u.ctemplate->olist->next;
        Py_INCREF(self);
        return self;
    }
    PyErr_SetString(PyExc_TypeError, "Not an iterable HocObject");
    return NULL;
}

static hoc_Item* next_valid_secitem(hoc_Item* q, hoc_Item* ql) {
    hoc_Item* nextnext;
    hoc_Item* next;
    for (next = q->next; next != ql; next = nextnext) {
        nextnext = next->next;
        Section* sec = next->element.sec;
        if (sec->prop) {  // valid
            break;
        }
        hoc_l_delete(next);
        section_unref(sec);
    }
    return next;
}

static PyObject* iternext_sl(PyHocObject* po, hoc_Item* ql) {
    // Note that the longstanding behavior of changing the currently
    // accessed section during iteration no longer takes place because
    // we cannot guarantee that an iterate will complete with state
    // PyHoc::Last and so the previous Section would have been left on the
    // hoc section stack.

    // Primarily the Section is pushed and the currently accessed python
    // Section is returned during sequential iteration over the list ql.
    // On re-entry here, the previous Section is popped.
    // The complexity is due to the possibility that the returned nrn_Section
    // may be deleted with h.delete_section(sec=nrn_Section). This would
    // invalidate the po->iteritem_ (point to freed memory) if it were
    // the iteritem of the current Section. Thus we choose to store
    // the next iteritem pointer in the list. Although not 100% safe, since
    // the user body of the iterator is allowed to delete_section an arbitary
    // subset of sections, the obvious work around, making a copy of ql,
    // is considered not worth it.

    // In this implementation, the first call to internext_sl starts out
    // in state PyHoc::Begin with po->iteritem_ == ql. If there is no valid
    // item, it sets po->iteritem_ = NULL and returns NULL. If there is
    // a valid item, it moves to state PyHoc::Last or PyHoc::NextNotLast
    // depending on whether there is a valid secitem following the current
    // item. The current section is pushed and currently accessed python
    // section is returned.

    // Thereafter, re-entry with po->iteritem_ == NULL, immediately
    // returns NULL.

    // Re-entry in state PyHoc::NextNotLast, pops the previously pushed Section,
    // sets sec to the current Section from po->iteritem_, and
    // sets po->iteritem_ to the next_valid_section. If there is no next_valid
    // section, then move to state PyHoc::Last.
    // Push the current section and return currently accessed python Section.

    // Re-entry in state PyHoc::Last just pops the current section, sets
    // po->iteritem_ = NULL, and returns NULL.

    if (po->iteritem_ == NULL) {
        return NULL;
    }

    if (po->u.its_ == PyHoc::Begin) {
        assert(po->iteritem_ == ql);
        hoc_Item* curitem = next_valid_secitem((hoc_Item*) (po->iteritem_), ql);
        if (curitem != ql) {  // typical case, a valid current item
            Section* sec = curitem->element.sec;
            assert(sec->prop);
            // sec could be delete_section before return to internext.sl
            // which would invalidate curitem.
            // Not perfectly safe but leave po->iteritem_ as next valid
            // secitem after curitem.
            po->iteritem_ = next_valid_secitem(curitem, ql);
            if (po->iteritem_ == ql) {
                po->u.its_ = PyHoc::Last;
            } else {
                po->u.its_ = PyHoc::NextNotLast;
            }
            return (PyObject*) newpysechelp(sec);
        } else {  // no valid current item so stop
            po->iteritem_ = NULL;
            return NULL;
        }
    } else if (po->u.its_ == PyHoc::NextNotLast) {
        // it would be a bug if po->iteritem_ (now the curitem) has been delete_section
        Section* sec = ((hoc_Item*) (po->iteritem_))->element.sec;
        if (!sec->prop) {
            // Handle an edge case where sec is a Python
            // Section and happens to have had as its only reference the
            // iteration variable on the previous iteration.
            // i.e. when sec is returned from the previous iteration, it will
            // be referenced by the iteration variable and the previous reference
            // will go to 0, thus invalidating po->iteritem_->element.sec->prop
            po->iteritem_ = next_valid_secitem((hoc_Item*) (po->iteritem_), ql);
            if (po->iteritem_ == ql) {
                po->u.its_ = PyHoc::Last;
                po->iteritem_ = NULL;
                return NULL;
            } else {
                sec = ((hoc_Item*) (po->iteritem_))->element.sec;
            }
        }
        assert(sec->prop);
        po->iteritem_ = next_valid_secitem((hoc_Item*) (po->iteritem_), ql);
        if (po->iteritem_ == ql) {
            po->u.its_ = PyHoc::Last;
        }
        return (PyObject*) newpysechelp(sec);
    } else if (po->u.its_ == PyHoc::Last) {
        po->iteritem_ = NULL;
        return NULL;
    }
    return NULL;  // never get here as po->u.its_ is always a defined state.
}

static PyObject* hocobj_iternext(PyObject* self) {
    // printf("hocobj_iternext %p\n", self);
    PyHocObject* po = (PyHocObject*) self;
    if (po->type_ == PyHoc::HocSectionListIterator) {
        hoc_Item* ql = (hoc_Item*) po->ho_->u.this_pointer;
        return iternext_sl(po, ql);
    } else if (po->type_ == PyHoc::HocForallSectionIterator) {
        return iternext_sl(po, section_list);
    } else if (po->sym_->type == TEMPLATE) {
        hoc_Item* q = (hoc_Item*) po->iteritem_;
        if (q != po->sym_->u.ctemplate->olist) {
            po->iteritem_ = q->next;
            return nrnpy_ho2po(OBJ(q));
        }
    }
    return NULL;
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
    PyHocObject* po = (PyHocObject*) self;
    if (po->type_ > PyHoc::HocArray && po->type_ != PyHoc::HocArrayIncomplete) {
        if (ix != 0 && po->type_ != PyHoc::HocScalarPtr) {
            PyErr_SetString(PyExc_IndexError, "index for hoc ref must be 0");
            return NULL;
        }
        if (po->type_ == PyHoc::HocScalarPtr) {
            try {
                auto const h = po->u.px_.next_array_element(ix);
                if (nrn_chk_data_handle(h)) {
                    result = Py_BuildValue("d", *h);
                }
            } catch (std::exception const& e) {
                // next_array_element throws if ix is invalid
                PyErr_SetString(PyExc_IndexError, e.what());
                return nullptr;
            }
        } else if (po->type_ == PyHoc::HocRefNum) {
            result = Py_BuildValue("d", po->u.x_);
        } else if (po->type_ == PyHoc::HocRefStr) {
            result = Py_BuildValue("s", po->u.s_);
        } else if (po->type_ == PyHoc::HocRefPStr) {
            result = Py_BuildValue("s", *po->u.pstr_);
        } else {
            result = nrnpy_ho2po(po->u.ho_);
        }
        return result;
    }
    if (po->type_ == PyHoc::HocObject) {  // might be in an iterator context
        if (po->ho_->ctemplate == hoc_vec_template_) {
            Vect* hv = (Vect*) po->ho_->u.this_pointer;
            if (ix < 0) {
                ix += vector_capacity(hv);
            }
            if (ix < 0 || ix >= vector_capacity(hv)) {
                char e[200];
                Sprintf(e, "%s", hoc_object_name(po->ho_));
                PyErr_SetString(PyExc_IndexError, e);
                return NULL;
            } else {
                return PyFloat_FromDouble(vector_vec(hv)[ix]);
            }
        } else if (po->ho_->ctemplate == hoc_list_template_) {
            OcList* hl = (OcList*) po->ho_->u.this_pointer;
            if (ix < 0) {
                ix += hl->count();
            }
            if (ix < 0 || ix >= hl->count()) {
                char e[200];
                Sprintf(e, "%s", hoc_object_name(po->ho_));
                PyErr_SetString(PyExc_IndexError, e);
                return NULL;
            } else {
                return nrnpy_ho2po(hl->object(ix));
            }
        } else {
            PyErr_SetString(PyExc_TypeError, "unsubscriptable object");
            return NULL;
        }
    }
    if (!po->sym_) {
        // printf("unsubscriptable %s %d type=%d\n", hoc_object_name(po->ho_), ix,
        // po->type_);
        PyErr_SetString(PyExc_TypeError, "unsubscriptable object");
        return NULL;
    } else if (po->sym_->type == TEMPLATE) {
        hoc_Item *q, *ql = po->sym_->u.ctemplate->olist;
        Object* ob;
        ITERATE(q, ql) {
            ob = OBJ(q);
            if (ob->index == ix) {
                return nrnpy_ho2po(ob);
            }
        }
        char e[200];
        Sprintf(e, "%s[%ld] instance does not exist", po->sym_->name, ix);
        PyErr_SetString(PyExc_IndexError, e);
        return NULL;
    }
    if (po->type_ != PyHoc::HocArray && po->type_ != PyHoc::HocArrayIncomplete) {
        char e[200];
        Sprintf(e, "unsubscriptable object, type %d\n", po->type_);
        PyErr_SetString(PyExc_TypeError, e);
        return NULL;
    }
    Arrayinfo* a = hocobj_aray(po->sym_, po->ho_);
    if (araychk(a, po, ix)) {
        return NULL;
    }
    if (a->nsub - 1 > po->nindex_) {  // another intermediate
        PyHocObject* ponew = intermediate(po, po->sym_, ix);
        result = (PyObject*) ponew;
    } else {  // ready to evaluate
        if (po->ho_) {
            eval_component(po, ix);
            if (po->sym_->type == SECTION || po->sym_->type == SECTIONREF) {
                section_object_seen = 0;
                result = nrnpy_cas(0, 0);
                nrn_popsec();
                return result;
            } else {
                if (po->type_ == PyHoc::HocArrayIncomplete) {
                    result = nrn_hocobj_ptr(hoc_pxpop());
                } else {
                    result = nrnpy_hoc_pop("po->ho_ hocobj_getitem");
                }
            }
        } else {  // must be a top level intermediate
            HocTopContextSet
            switch (po->sym_->type) {
            case VAR:
                hocobj_pushtop(po, po->sym_, ix);
                if (hoc_evalpointer_err()) {
                    --po->nindex_;
                    HocContextRestore;
                    return NULL;
                }
                --po->nindex_;
                if (po->type_ == PyHoc::HocArrayIncomplete) {
                    assert(!po->u.px_);
                    result = nrn_hocobj_ptr(hoc_pxpop());
                } else {
                    result = Py_BuildValue("d", *hoc_pxpop());
                }
                break;
            case OBJECTVAR:
                hocobj_pushtop(po, 0, ix);
                if (hocobj_objectvar(po->sym_)) {
                    break;
                }
                --po->nindex_;
                result = nrnpy_ho2po(*hoc_objpop());
                break;
            case SECTION:
                hocobj_pushtop(po, 0, ix);
                result = hocobj_getsec(po->sym_);
                --po->nindex_;
                break;
            }
            HocContextRestore;
        }
    }
    return result;
}

static PyObject* hocobj_slice_getitem(PyObject* self, PyObject* slice) {
    // Non slice indexing still uses original function
    if (!PySlice_Check(slice)) {
        return hocobj_getitem(self, PyLong_AsLong(slice));
    }
    auto* po = (PyHocObject*) self;
    if (!po->ho_) {
        PyErr_SetString(PyExc_TypeError, "Obj is NULL");
        return nullptr;
    }
    if (po->type_ != PyHoc::HocObject || po->ho_->ctemplate != hoc_vec_template_) {
        PyErr_SetString(PyExc_TypeError, "sequence index must be integer, not 'slice'");
        return nullptr;
    }
    auto* v = (Vect*) po->ho_->u.this_pointer;
    Py_ssize_t start = 0;
    Py_ssize_t end = 0;
    Py_ssize_t step = 0;
    Py_ssize_t slicelen = 0;
    Py_ssize_t len = vector_capacity(v);
    PySlice_GetIndicesEx(slice, len, &start, &end, &step, &slicelen);
    if (step == 0) {
        PyErr_SetString(PyExc_ValueError, "slice step cannot be zero");
        return nullptr;
    }
    Object** obj = new_vect(v, slicelen, start, step);
    return nrnpy_ho2po(*obj);
}

static int hocobj_setitem(PyObject* self, Py_ssize_t i, PyObject* arg) {
    int err = -1;
    PyHocObject* po = (PyHocObject*) self;
    if (po->type_ > PyHoc::HocArray) {
        if (po->type_ == PyHoc::HocArrayIncomplete) {
            PyErr_SetString(PyExc_TypeError, "incomplete hoc pointer");
            return -1;
        }
        if (i != 0 && po->type_ != PyHoc::HocScalarPtr) {
            PyErr_SetString(PyExc_IndexError, "index for hoc ref must be 0");
            return -1;
        }
        if (po->type_ == PyHoc::HocScalarPtr) {
            try {
                auto const h = po->u.px_.next_array_element(i);
                if (nrn_chk_data_handle(h)) {
                    PyArg_Parse(arg, "d", static_cast<double const*>(h));
                } else {
                    return -1;
                }
            } catch (std::exception const& e) {
                // next_array_element throws if ix is invalid
                PyErr_SetString(PyExc_IndexError, e.what());
                return -1;
            }
        } else if (po->type_ == PyHoc::HocRefNum) {
            PyArg_Parse(arg, "d", &po->u.x_);
        } else if (po->type_ == PyHoc::HocRefStr) {
            char* ts;
            PyArg_Parse(arg, "s", &ts);
            hoc_assign_str(&po->u.s_, ts);
        } else if (po->type_ == PyHoc::HocRefPStr) {
            char* ts;
            PyArg_Parse(arg, "s", &ts);
            hoc_assign_str(po->u.pstr_, ts);
        } else {
            PyObject* tp;
            PyArg_Parse(arg, "O", &tp);
            po->u.ho_ = nrnpy_po2ho(tp);
        }
        return 0;
    }
    if (po->ho_) {
        if (po->ho_->ctemplate == hoc_vec_template_) {
            Vect* vec = (Vect*) po->ho_->u.this_pointer;
            int vec_size = vector_capacity(vec);
            // allow Python style negative indices
            if (i < 0) {
                i += vec_size;
            }
            if (i >= vec_size || i < 0) {
                PyErr_SetString(PyExc_IndexError, "index out of bounds");
                return -1;
            }
            PyArg_Parse(arg, "d", vector_vec(vec) + i);
            return 0;
        }
    }
    if (!po->sym_ || po->type_ != PyHoc::HocArray) {
        PyErr_SetString(PyExc_TypeError, "unsubscriptable object");
        return -1;
    }
    Arrayinfo* a = hocobj_aray(po->sym_, po->ho_);
    if (!a || a->nsub - 1 != po->nindex_) {
        int nsub = a ? a->nsub : 0;
        std::ostringstream oss;
        oss << "Wrong number of subscripts, hoc var " << po->sym_->name << " has " << nsub
            << " but compiled with " << (po->nindex_ + 1);
        PyErr_SetString(PyExc_TypeError, oss.str().c_str());
        return -1;
    }
    if (araychk(a, po, i)) {
        return -1;
    }
    if (po->ho_) {
        if (po->sym_->type == SECTION) {
            PyErr_SetString(PyExc_TypeError, "not assignable");
        } else {
            eval_component(po, i);
            err = set_final_from_stk(arg);
        }
    } else {  // must be a top level intermediate
        HocTopContextSet
        switch (po->sym_->type) {
        case VAR:
            hocobj_pushtop(po, po->sym_, i);
            if (hoc_evalpointer_err()) {
                HocContextRestore;
                --po->nindex_;
                return -1;
            }
            --po->nindex_;
            err = PyArg_Parse(arg, "d", hoc_pxpop()) != 1;
            break;
        case OBJECTVAR: {
            hocobj_pushtop(po, 0, i);
            err = hocobj_objectvar(po->sym_);
            if (err) {
                break;  // can't reach because of earlier array_chk
            }
            --po->nindex_;
            Object** op;
            op = hoc_objpop();
            PyObject* pyo;
            if (PyArg_Parse(arg, "O", &pyo) == 1) {
                Object* ho = nrnpy_po2ho(pyo);
                hoc_obj_unref(*op);
                *op = ho;
                err = 0;
            } else {
                err = 1;
            }
            break;
        }
        default:
            PyErr_SetString(PyExc_TypeError, "not assignable");
            break;
        }
        HocContextRestore;
    }
    return err;
}

static int hocobj_slice_setitem(PyObject* self, PyObject* slice, PyObject* arg) {
    // Non slice indexing still uses original function
    if (!PySlice_Check(slice)) {
        return hocobj_setitem(self, PyLong_AsLong(slice), arg);
    }
    auto* po = (PyHocObject*) self;
    if (!po->ho_) {
        PyErr_SetString(PyExc_TypeError, "Obj is NULL");
        return -1;
    }
    if (po->type_ != PyHoc::HocObject || po->ho_->ctemplate != hoc_vec_template_) {
        PyErr_SetString(PyExc_TypeError, "sequence index must be integer, not 'slice'");
        return -1;
    }
    auto v = (Vect*) po->ho_->u.this_pointer;
    Py_ssize_t start = 0;
    Py_ssize_t end = 0;
    Py_ssize_t step = 0;
    Py_ssize_t slicelen = 0;
    Py_ssize_t cap = vector_capacity(v);
    PySlice_GetIndicesEx(slice, cap, &start, &end, &step, &slicelen);
    // Slice index assignment requires a list of the same size as the slice
    PyObject* iter = PyObject_GetIter(arg);
    if (!iter) {
        PyErr_SetString(PyExc_TypeError, "can only assign an iterable");
        return -1;
    }
    PyObject* val = nullptr;
    for (Py_ssize_t i = 0; i < slicelen; ++i) {
        val = PyIter_Next(iter);
        if (!val) {
            Py_DECREF(iter);
            PyErr_SetString(PyExc_IndexError, "iterable object must have the same length as slice");
            return -1;
        }
        PyArg_Parse(val, "d", vector_vec(v) + (i * step + start));
        Py_DECREF(val);
    }
    val = PyIter_Next(iter);
    Py_DECREF(iter);
    if (val) {
        Py_DECREF(val);
        PyErr_SetString(PyExc_IndexError, "iterable object must have the same length as slice");
        return -1;
    }
    return 0;
}

static PyObject* mkref(PyObject* self, PyObject* args) {
    PyObject* pa;
    PyHocObject* result;
    if (PyArg_ParseTuple(args, "O", &pa) == 1) {
        result = (PyHocObject*) hocobj_new(hocobject_type, 0, 0);
        if (nrnpy_numbercheck(pa)) {
            result->type_ = PyHoc::HocRefNum;
            PyObject* pn = PyNumber_Float(pa);
            result->u.x_ = PyFloat_AsDouble(pn);
            Py_XDECREF(pn);
        } else if (is_python_string(pa)) {
            result->type_ = PyHoc::HocRefStr;
            result->u.s_ = 0;
            Py2NRNString str(pa);
            if (str.err()) {
                str.set_pyerr(PyExc_TypeError, "string arg must have only ascii characters");
                Py_XDECREF(result);
                return NULL;
            }
            char* cpa = str.c_str();
            hoc_assign_str(&result->u.s_, cpa);
        } else {
            result->type_ = PyHoc::HocRefObj;
            result->u.ho_ = nrnpy_po2ho(pa);
        }
        return (PyObject*) result;
    }
    PyErr_SetString(PyExc_TypeError, "single arg must be number, string, or Object");
    return NULL;
}

static PyObject* cpp2refstr(char** cpp) {
    // If cpp is from a hoc_temp_charptr (see src/oc/code.cpp) then create a
    // HocRefStr and copy *cpp. Otherwise, assume it is from a hoc strdef
    // or a HocRefStr which is persistent over the life time of this returned
    // PyObject so that it is safe to create a HocRefPStr such that
    // u.pstr_ = cpp and it is not needed
    // for the HocRefPStr destructor to delete either u.pstr_ or *u.pstr_.

    assert(cpp && *cpp);  // not really sure about the *cpp
    PyHocObject* result = (PyHocObject*) hocobj_new(hocobject_type, 0, 0);
    if (hoc_is_temp_charptr(cpp)) {  // return HocRefStr HocObject.
        result->type_ = PyHoc::HocRefStr;
        result->u.s_ = 0;
        hoc_assign_str(&result->u.s_, *cpp);
    } else {
        result->type_ = PyHoc::HocRefPStr;
        result->u.pstr_ = cpp;
    }
    return (PyObject*) result;
}

static PyObject* setpointer(PyObject* self, PyObject* args) {
    PyObject *ref, *name, *pp, *result = NULL;
    if (PyArg_ParseTuple(args, "O!OO", hocobject_type, &ref, &name, &pp) == 1) {
        PyHocObject* href = (PyHocObject*) ref;
        double** ppd = 0;
        if (href->type_ != PyHoc::HocScalarPtr) {
            goto done;
        }
        neuron::container::generic_data_handle* gh{};
        if (PyObject_TypeCheck(pp, hocobject_type)) {
            PyHocObject* hpp = (PyHocObject*) pp;
            if (hpp->type_ != PyHoc::HocObject) {
                goto done;
            }
            Py2NRNString str(name);
            char* n = str.c_str();
            if (str.err()) {
                str.set_pyerr(PyExc_TypeError, "POINTER name can contain only ascii characters");
                return NULL;
            }
            Symbol* sym = getsym(n, hpp->ho_, 0);
            if (!sym || sym->type != RANGEVAR || sym->subtype != NRNPOINTER) {
                goto done;
            }
            Prop* prop = ob2pntproc_0(hpp->ho_)->prop;
            if (!prop) {
                PyErr_SetString(PyExc_TypeError, "Point_process not located in a section");
                return NULL;
            }
            gh = &(prop->dparam[sym->u.rng.index]);
        } else {
            gh = nrnpy_setpointer_helper(name, pp);
            if (!gh) {
                goto done;
            }
        }
        *gh = neuron::container::generic_data_handle{href->u.px_};
        result = Py_None;
        Py_INCREF(result);
    }
done:
    if (!result) {
        PyErr_SetString(PyExc_TypeError,
                        "setpointer(_ref_hocvar, 'POINTER_name', point_process or "
                        "nrn.Mechanism))");
    }
    return result;
}

static PyObject* hocobj_vptr(PyObject* pself, PyObject* args) {
    Object* ho = ((PyHocObject*) pself)->ho_;
    PyObject* po = NULL;
    if (ho) {
        po = Py_BuildValue("O", PyLong_FromVoidPtr(ho));
    }
    if (!po) {
        PyErr_SetString(PyExc_TypeError, "HocObject does not wrap a Hoc Object");
    }
    return po;
}

static long hocobj_hash(PyHocObject* self) {
    return castptr2long self->ho_;
}

PyObject* nrn_ptr_richcmp(void* self_ptr, void* other_ptr, int op) {
    bool result = false;
    switch (op) {
    case Py_LT:
        result = self_ptr < other_ptr;
        break;
    case Py_LE:
        result = self_ptr <= other_ptr;
        break;
    case Py_EQ:
        result = self_ptr == other_ptr;
        break;
    case Py_NE:
        result = self_ptr != other_ptr;
        break;
    case Py_GT:
        result = self_ptr > other_ptr;
        break;
    case Py_GE:
        result = self_ptr >= other_ptr;
        break;
    }
    if (result) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

// TODO: unfortunately, this duplicates code from hocobj_same; consolidate?
static PyObject* hocobj_richcmp(PyHocObject* self, PyObject* other, int op) {
    auto* pyhoc_other = reinterpret_cast<PyHocObject*>(other);
    void* self_ptr = self->ho_;
    void* other_ptr = other;
    bool are_equal = true;
    if (PyObject_TypeCheck(other, hocobject_type)) {
        if (pyhoc_other->type_ == self->type_) {
            switch (self->type_) {
            case PyHoc::HocRefNum:
            case PyHoc::HocRefStr:
            case PyHoc::HocRefObj:
            case PyHoc::HocRefPStr:
                /* only same objects can point to same h.ref */
                self_ptr = (void*) self;
                break;
            case PyHoc::HocFunction:
                if (self->ho_ != pyhoc_other->ho_) {
                    if (op == Py_NE) {
                        Py_RETURN_TRUE;
                    } else if (op == Py_EQ) {
                        Py_RETURN_FALSE;
                    }
                    /* different classes, comparing < or > doesn't make sense */
                    PyErr_SetString(PyExc_TypeError, "this comparison is undefined");
                    return NULL;
                }
                self_ptr = self->sym_;
                other_ptr = pyhoc_other->sym_;
                break;
            case PyHoc::HocScalarPtr:
                // this seems rather dubious
                self_ptr = static_cast<double*>(self->u.px_);
                other_ptr = static_cast<double*>(pyhoc_other->u.px_);
                break;
            case PyHoc::HocArrayIncomplete:
            case PyHoc::HocArray:
                if (op != Py_EQ && op != Py_NE) {
                    /* comparing partial arrays doesn't make sense */
                    PyErr_SetString(PyExc_TypeError, "this comparison is undefined");
                    return NULL;
                }
                if (self->ho_ != pyhoc_other->ho_) {
                    /* different objects */
                    other_ptr = pyhoc_other->ho_;
                    break;
                }
                if (self->nindex_ != pyhoc_other->nindex_ || self->sym_ != pyhoc_other->sym_) {
                    if (op == Py_NE) {
                        Py_RETURN_TRUE;
                    }
                    Py_RETURN_FALSE;
                }
                for (int i = 0; i < self->nindex_; i++) {
                    if (self->indices_[i] != pyhoc_other->indices_[i]) {
                        are_equal = false;
                    }
                }
                if (are_equal == (op == Py_EQ)) {
                    Py_RETURN_TRUE;
                }
                Py_RETURN_FALSE;
            default:
                other_ptr = pyhoc_other->ho_;
            }
        } else {
            if (op == Py_EQ) {
                Py_RETURN_FALSE;
            } else if (op == Py_NE) {
                Py_RETURN_TRUE;
            }
            /* different NEURON object types are incomperable besides for (in)equality */
            PyErr_SetString(PyExc_TypeError, "this comparison is undefined");
            return NULL;
        }
    }
    return nrn_ptr_richcmp(self_ptr, other_ptr, op);
}

static PyObject* hocobj_same(PyHocObject* pself, PyObject* args) {
    PyObject* po;
    if (PyArg_ParseTuple(args, "O", &po)) {
        if (PyObject_TypeCheck(po, hocobject_type)) {
            if (((PyHocObject*) po)->ho_ == pself->ho_) {
                Py_RETURN_TRUE;
            }
        }
        Py_RETURN_FALSE;
    }
    return NULL;
}

static char* double_array_interface(PyObject* po, long& stride) {
    void* data = 0;
    PyObject* pstride;
    PyObject* psize;
    if (PyObject_HasAttrString(po, "__array_interface__")) {
        PyObject* ai = PyObject_GetAttrString(po, "__array_interface__");
        Py2NRNString typestr(PyDict_GetItemString(ai, "typestr"));
        if (strcmp(typestr.c_str(), array_interface_typestr) == 0) {
            data = PyLong_AsVoidPtr(PyTuple_GetItem(PyDict_GetItemString(ai, "data"), 0));
            // printf("double_array_interface idata = %ld\n", idata);
            if (PyErr_Occurred()) {
                data = 0;
            }
            pstride = PyDict_GetItemString(ai, "strides");
            if (pstride == Py_None) {
                stride = 8;
            } else if (PyTuple_Check(pstride)) {
                if (PyTuple_Size(pstride) == 1) {
                    psize = PyTuple_GetItem(pstride, 0);
                    if (PyLong_Check(psize)) {
                        stride = PyLong_AsLong(psize);
                    } else if (PyInt_Check(psize)) {
                        stride = PyInt_AS_LONG(psize);

                    } else {
                        PyErr_SetString(PyExc_TypeError,
                                        "array_interface stride element of invalid type.");
                        data = 0;
                    }

                } else
                    data = 0;  // don't handle >1 dimensions
            } else {
                PyErr_SetString(PyExc_TypeError, "array_interface stride object of invalid type.");
                data = 0;
            }
        }
        Py_DECREF(ai);
    }
    return static_cast<char*>(data);
}

static IvocVect* nrnpy_vec_from_python(void* v) {
    Vect* hv = (Vect*) v;
    //	printf("%s.from_array\n", hoc_object_name(hv->obj_));
    Object* ho = *hoc_objgetarg(1);
    if (ho->ctemplate->sym != nrnpy_pyobj_sym_) {
        hoc_execerror(hoc_object_name(ho), " is not a PythonObject");
    }
    PyObject* po = nrnpy_hoc2pyobject(ho);
    Py_INCREF(po);
    if (!PySequence_Check(po)) {
        if (!PyIter_Check(po)) {
            hoc_execerror(hoc_object_name(ho),
                          " does not support the Python Sequence or Iterator protocol");
        }
        PyObject* iterator = PyObject_GetIter(po);
        assert(iterator != NULL);
        int i = 0;
        PyObject* p;
        while ((p = PyIter_Next(iterator)) != NULL) {
            if (!PyNumber_Check(p)) {
                char buf[50];
                Sprintf(buf, "item %d not a number", i);
                hoc_execerror(buf, 0);
            }
            hv->push_back(PyFloat_AsDouble(p));
            Py_DECREF(p);
            ++i;
        }
        Py_DECREF(iterator);
    } else {
        int size = PySequence_Size(po);
        //		printf("size = %d\n", size);
        hv->resize(size);
        double* x = vector_vec(hv);
        long stride;
        char* y = double_array_interface(po, stride);
        if (y) {
            for (int i = 0, j = 0; i < size; ++i, j += stride) {
                x[i] = *(double*) (y + j);
            }
        } else {
            for (int i = 0; i < size; ++i) {
                PyObject* p = PySequence_GetItem(po, i);
                if (!PyNumber_Check(p)) {
                    char buf[50];
                    Sprintf(buf, "item %d not a number", i);
                    hoc_execerror(buf, 0);
                }
                x[i] = PyFloat_AsDouble(p);
                Py_DECREF(p);
            }
        }
    }
    Py_DECREF(po);
    return hv;
}

static PyObject* (*vec_as_numpy)(int, double*);
extern "C" int nrnpy_set_vec_as_numpy(PyObject* (*p)(int, double*) ) {
    vec_as_numpy = p;
    return 0;
}

static PyObject* store_savestate_ = NULL;
static PyObject* restore_savestate_ = NULL;


static void nrnpy_store_savestate_(char** save_data, uint64_t* save_data_size) {
    if (store_savestate_) {
        // call store_savestate_ with no arguments to get a byte array that we can write out
        PyObject* args = PyTuple_New(0);
        PyObject* result = PyObject_CallObject(store_savestate_, args);
        Py_INCREF(result);
        Py_DECREF(args);
        if (result == NULL) {
            hoc_execerror("SaveState:", "Data store failure.");
        }
        // free any old data and make a copy
        if (*save_data) {
            delete[](*save_data);
        }
        *save_data_size = PyByteArray_Size(result);
        *save_data = new char[*save_data_size];
        memcpy(*save_data, PyByteArray_AsString(result), *save_data_size);
        Py_DECREF(result);
    } else {
        *save_data_size = 0;
    }
}

static void nrnpy_restore_savestate_(int64_t size, char* data) {
    if (restore_savestate_) {
        PyObject* args = PyTuple_New(1);
        PyObject* py_data = PyByteArray_FromStringAndSize(data, size);
        Py_INCREF(py_data);
        if (py_data == NULL) {
            hoc_execerror("SaveState:", "Data restore failure.");
        }
        // note: PyTuple_SetItem steals a ref to py_data
        PyTuple_SetItem(args, 0, py_data);
        PyObject* result = PyObject_CallObject(restore_savestate_, args);
        Py_DECREF(args);
        if (result == NULL) {
            hoc_execerror("SaveState:", "Data restore failure.");
        }
    } else {
        if (size) {
            hoc_execerror("SaveState:", "Missing data restore function.");
        }
    }
}

extern "C" int nrnpy_set_toplevel_callbacks(PyObject* rvp_plot0,
                                            PyObject* plotshape_plot0,
                                            PyObject* get_mech_object_0,
                                            PyObject* store_savestate,
                                            PyObject* restore_savestate) {
    rvp_plot = rvp_plot0;
    plotshape_plot = plotshape_plot0;
    get_mech_object_ = get_mech_object_0;
    store_savestate_ = store_savestate;
    restore_savestate_ = restore_savestate;
    nrnpy_restore_savestate = nrnpy_restore_savestate_;
    nrnpy_store_savestate = nrnpy_store_savestate_;
    return 0;
}

static PyObject* gui_callback = NULL;
extern "C" int nrnpy_set_gui_callback(PyObject* new_gui_callback) {
    gui_callback = new_gui_callback;
    return 0;
}

static double object_to_double_(Object* obj) {
    PyObject* const pyobj = nrnpy_ho2po(obj);
    Py_INCREF(pyobj);
    const double result = PyFloat_AsDouble(pyobj);
    Py_DECREF(pyobj);
    return result;
}

static void* nrnpy_get_pyobj_(Object* obj) {
    // returns something wrapping a PyObject if it is a PyObject else NULL
    if (obj->ctemplate->sym == nrnpy_pyobj_sym_) {
        return (void*) nrnpy_ho2po(obj);
    }
    return NULL;
}

static void nrnpy_decref_(void* pyobj) {
    // note: this assumes that pyobj is really a PyObject
    if (pyobj) {
        Py_DECREF((PyObject*) pyobj);
    }
}

static PyObject* gui_helper_3_helper_(const char* name, Object* obj, int handle_strptr) {
    int narg = 1;
    while (ifarg(narg)) {
        narg++;
    }
    narg--;
    PyObject* args = PyTuple_New(narg + 3);
    PyObject* pyname = PyString_FromString(name);
    PyTuple_SetItem(args, 0, pyname);
    for (int iarg = 0; iarg < narg; iarg++) {
        const int iiarg = iarg + 1;
        if (hoc_is_object_arg(iiarg)) {
            PyObject* active_obj = nrnpy_ho2po(*hoc_objgetarg(iiarg));
            PyTuple_SetItem(args, iarg + 3, active_obj);
        } else if (hoc_is_pdouble_arg(iiarg)) {
            PyHocObject* ptr_nrn = (PyHocObject*) hocobj_new(hocobject_type, 0, 0);
            ptr_nrn->type_ = PyHoc::HocScalarPtr;
            ptr_nrn->u.px_ = hoc_hgetarg<double>(iiarg);
            PyObject* py_ptr = (PyObject*) ptr_nrn;
            Py_INCREF(py_ptr);
            PyTuple_SetItem(args, iarg + 3, py_ptr);
        } else if (hoc_is_str_arg(iiarg)) {
            if (handle_strptr > 0) {
                char** str_arg = hoc_pgargstr(iiarg);
                PyObject* py_ptr = cpp2refstr(str_arg);
                Py_INCREF(py_ptr);
                PyTuple_SetItem(args, iarg + 3, py_ptr);
            } else {
                PyObject* py_str = PyString_FromString(gargstr(iiarg));
                PyTuple_SetItem(args, iarg + 3, py_str);
            }
        } else if (hoc_is_double_arg(iiarg)) {
            PyObject* py_double = PyFloat_FromDouble(*getarg(iiarg));
            PyTuple_SetItem(args, iarg + 3, py_double);
        }
    }
    PyObject* my_obj;
    if (obj) {
        // there's a problem with this: if obj is intrinisically a PyObject, then this is increasing
        // it's refcount and that's
        my_obj = nrnpy_ho2po(obj);
    } else {
        my_obj = Py_None;
        Py_INCREF(Py_None);
    }
    PyTuple_SetItem(args, 1, my_obj);  // steals a reference
    PyObject* my_obj2;
    if (hoc_thisobject && name[0] != '~') {
        my_obj2 = nrnpy_ho2po(hoc_thisobject);  // in the case of a HOC object, such as happens with
                                                // List.browser, the ref count will be 1
    } else {
        my_obj2 = Py_None;
        Py_INCREF(Py_None);
    }

    PyTuple_SetItem(args, 2, my_obj2);  // steals a reference to my_obj2
    PyObject* po = PyObject_CallObject(gui_callback, args);
    if (PyErr_Occurred()) {
        // if there was an error, display it and return 0.
        // It's not a great solution, but it beats segfaulting
        PyErr_Print();
        po = PyLong_FromLong(0);
    }
    Py_DECREF(args);  // Note: this decreases the ref count of my_obj and my_obj2
    return po;
}

static Object** gui_helper_3_(const char* name, Object* obj, int handle_strptr) {
    if (gui_callback) {
        PyObject* po = gui_helper_3_helper_(name, obj, handle_strptr);
        // TODO: something that allows None (currently nrnpy_po2ho returns NULL if po == Py_None)
        Object* ho = nrnpy_po2ho(po);
        Py_DECREF(po);
        if (ho) {
            --ho->refcount;
        }
        return hoc_temp_objptr(ho);
    }
    return NULL;
}

static char** gui_helper_3_str_(const char* name, Object* obj, int handle_strptr) {
    if (gui_callback) {
        PyObject* po = gui_helper_3_helper_(name, obj, handle_strptr);
        char** ts = hoc_temp_charptr();
        Py2NRNString str(po, true);
        *ts = str.c_str();
        // TODO: is there a memory leak here? do I need to: s2free.push_back(*ts);
        Py_DECREF(po);
        return ts;
    }
    return NULL;
}


static Object** gui_helper_(const char* name, Object* obj) {
    return gui_helper_3_(name, obj, 0);
}

static Object** vec_as_numpy_helper(int size, double* data) {
    if (vec_as_numpy) {
        PyObject* po = (*vec_as_numpy)(size, data);
        if (po != Py_None) {
            Object* ho = nrnpy_po2ho(po);
            Py_DECREF(po);
            --ho->refcount;
            return hoc_temp_objptr(ho);
        }
    }
    hoc_execerror("Vector.as_numpy() error", 0);
    return NULL;
}

static Object** nrnpy_vec_to_python(void* v) {
    Vect* hv = (Vect*) v;
    int size = hv->size();
    double* x = vector_vec(hv);
    //	printf("%s.to_array\n", hoc_object_name(hv->obj_));
    PyObject* po;
    Object* ho = 0;

    // as_numpy_array=True is the case where this function is being called by the
    // ivocvect __array__ member
    // as such perhaps we should check here that no arguments were passed
    // although this should be the case unless the function is erroneously called
    // by the user.

    if (ifarg(1)) {
        ho = *hoc_objgetarg(1);
        if (ho->ctemplate->sym != nrnpy_pyobj_sym_) {
            hoc_execerror(hoc_object_name(ho), " is not a PythonObject");
        }
        po = nrnpy_hoc2pyobject(ho);
        if (!PySequence_Check(po)) {
            hoc_execerror(hoc_object_name(ho), " is not a Python Sequence");
        }
        if (size != PySequence_Size(po)) {
            hoc_execerror(hoc_object_name(ho), "Python Sequence not same size as Vector");
        }
    } else {
        if ((po = PyList_New(size)) == NULL) {
            hoc_execerror("Could not create new Python List with correct size.", 0);
        }

        ho = nrnpy_po2ho(po);
        Py_DECREF(po);
        --ho->refcount;
    }
    //	printf("size = %d\n", size);
    long stride;
    char* y = double_array_interface(po, stride);
    if (y) {
        for (int i = 0, j = 0; i < size; ++i, j += stride) {
            *(double*) (y + j) = x[i];
        }
    } else if (PyList_Check(po)) {  // PySequence_SetItem does DECREF of old items
        for (int i = 0; i < size; ++i) {
            PyObject* pn = PyFloat_FromDouble(x[i]);
            if (!pn || PyList_SetItem(po, i, pn) == -1) {
                char buf[50];
                Sprintf(buf, "%d of %d", i, size);
                hoc_execerror("Could not set a Python Sequence item", buf);
            }
        }
    } else {  // assume PySequence_SetItem works
        for (int i = 0; i < size; ++i) {
            PyObject* pn = PyFloat_FromDouble(x[i]);
            if (!pn || PySequence_SetItem(po, i, pn) == -1) {
                char buf[50];
                Sprintf(buf, "%d of %d", i, size);
                hoc_execerror("Could not set a Python Sequence item", buf);
            }
            Py_DECREF(pn);
        }
    }
    return hoc_temp_objptr(ho);
}

static Object* rvp_rxd_to_callable_(Object* obj) {
    if (obj) {
        PyObject* py_obj = nrnpy_ho2po(obj);
        PyObject* result = PyObject_CallFunctionObjArgs(nrnpy_rvp_pyobj_callback, py_obj, NULL);
        Py_DECREF(py_obj);
        Object* obj_result = nrnpy_po2ho(result);
        Py_DECREF(result);  // the previous line incremented the reference count
        return obj_result;
    } else {
        return 0;
    }
}


extern "C" PyObject* get_plotshape_data(PyObject* sp) {
    PyHocObject* pho = (PyHocObject*) sp;
    ShapePlotInterface* spi;
    if (!is_obj_type(pho->ho_, "PlotShape")) {
        PyErr_SetString(PyExc_TypeError, "get_plotshape_variable only takes PlotShape objects");
        return NULL;
    }
    void* that = pho->ho_->u.this_pointer;
#if HAVE_IV
    IFGUI
    spi = ((ShapePlot*) that);
}
else {
    spi = ((ShapePlotData*) that);
    ENDGUI
#else
    spi = ((ShapePlotData*) that);
#endif
    Object* sl = spi->neuron_section_list();
    PyObject* py_sl = nrnpy_ho2po(sl);
    PyObject* py_obj = (PyObject*) spi->varobj();
    if (!py_obj) {
        py_obj = Py_None;
    }
    // NOte: O increases the reference count; N does not
    return Py_BuildValue("sOffN", spi->varname(), py_obj, spi->low(), spi->high(), py_sl);
}

// poorly follows __reduce__ and __setstate__
// from numpy/core/src/multiarray/methods.c
static PyObject* hocpickle_reduce(PyObject* self, PyObject* args) {
    // printf("hocpickle_reduce\n");
    PyHocObject* pho = (PyHocObject*) self;
    if (!is_obj_type(pho->ho_, "Vector")) {
        PyErr_SetString(PyExc_TypeError, "HocObject: Only Vector instance can be pickled");
        return NULL;
    }
    Vect* vec = (Vect*) pho->ho_->u.this_pointer;

    // neuron module has a _pkl method that returns h.Vector(0)
    PyObject* mod = PyImport_ImportModule("neuron");
    if (mod == NULL) {
        return NULL;
    }
    PyObject* obj = PyObject_GetAttrString(mod, "_pkl");
    Py_DECREF(mod);
    if (obj == NULL) {
        PyErr_SetString(PyExc_Exception, "neuron module has no _pkl method.");
        return NULL;
    }

    PyObject* ret = PyTuple_New(3);
    if (ret == NULL) {
        return NULL;
    }
    PyTuple_SET_ITEM(ret, 0, obj);
    PyTuple_SET_ITEM(ret, 1, Py_BuildValue("(N)", PyInt_FromLong(0)));
    // see numpy implementation if more ret[1] stuff needed in case we
    // pickle anything but a hoc Vector. I don't think ret[1] can be None.

    // Fill object's state. Tuple with 4 args:
    // pickle version, 2.0 bytes to determine if swapbytes needed,
    // vector size, string data
    PyObject* state = PyTuple_New(4);
    if (state == NULL) {
        Py_DECREF(ret);
        return NULL;
    }
    PyTuple_SET_ITEM(state, 0, PyInt_FromLong(1));
    double x = 2.0;
    PyObject* str = PyBytes_FromStringAndSize((const char*) (&x), sizeof(double));
    if (str == NULL) {
        Py_DECREF(ret);
        Py_DECREF(state);
        return NULL;
    }
    PyTuple_SET_ITEM(state, 1, str);
    PyTuple_SET_ITEM(state, 2, PyInt_FromLong(vec->size()));
    str = PyBytes_FromStringAndSize((const char*) vector_vec(vec), vec->size() * sizeof(double));
    if (str == NULL) {
        Py_DECREF(ret);
        Py_DECREF(state);
        return NULL;
    }
    PyTuple_SET_ITEM(state, 3, str);
    PyTuple_SET_ITEM(ret, 2, state);
    return ret;
}

// following copied (except for nrn_need_byteswap line) from NEURON ivocvect.cpp
#define BYTEHEADER   \
    uint32_t _II__;  \
    char* _IN__;     \
    char _OUT__[16]; \
    int BYTESWAP_FLAG = 0;
#define BYTESWAP(_X__, _TYPE__)                                 \
    if (BYTESWAP_FLAG == 1) {                                   \
        _IN__ = (char*) &(_X__);                                \
        for (_II__ = 0; _II__ < sizeof(_TYPE__); _II__++) {     \
            _OUT__[_II__] = _IN__[sizeof(_TYPE__) - _II__ - 1]; \
        }                                                       \
        (_X__) = *((_TYPE__*) &_OUT__);                         \
    }

static PyObject* hocpickle_setstate(PyObject* self, PyObject* args) {
    BYTEHEADER
    int version = -1;
    int size = -1;
    PyObject* rawdata = NULL;
    PyObject* endian_data;
    PyHocObject* pho = (PyHocObject*) self;
    // printf("hocpickle_setstate %s\n", hoc_object_name(pho->ho_));
    Vect* vec = (Vect*) pho->ho_->u.this_pointer;
    if (!PyArg_ParseTuple(args, "(iOiO)", &version, &endian_data, &size, &rawdata)) {
        return NULL;
    }
    Py_INCREF(endian_data);
    Py_INCREF(rawdata);
    // printf("hocpickle version=%d size=%d\n", version, size);
    vector_resize(vec, size);
    if (!PyBytes_Check(rawdata) || !PyBytes_Check(endian_data)) {
        PyErr_SetString(PyExc_TypeError, "pickle not returning string");
        Py_DECREF(endian_data);
        Py_DECREF(rawdata);
        return NULL;
    }
    char* datastr;
    Py_ssize_t len;
    if (PyBytes_AsStringAndSize(endian_data, &datastr, &len) < 0) {
        Py_DECREF(endian_data);
        Py_DECREF(rawdata);
        return NULL;
    }
    if (len != sizeof(double)) {
        PyErr_SetString(PyExc_ValueError, "endian_data size is not sizeof(double)");
        Py_DECREF(endian_data);
        Py_DECREF(rawdata);
        return NULL;
    }
    BYTESWAP_FLAG = 0;
    if (*((double*) datastr) != 2.0) {
        BYTESWAP_FLAG = 1;
    }
    Py_DECREF(endian_data);
    // printf("byteswap = %d\n", BYTESWAP_FLAG);
    if (PyBytes_AsStringAndSize(rawdata, &datastr, &len) < 0) {
        Py_DECREF(rawdata);
        return NULL;
    }
    if (len != size * sizeof(double)) {
        PyErr_SetString(PyExc_ValueError, "buffer size does not match array size");
        Py_DECREF(rawdata);
        return NULL;
    }
    if (BYTESWAP_FLAG) {
        double* x = (double*) datastr;
        for (int i = 0; i < size; ++i) {
            BYTESWAP(x[i], double)
        }
    }
    memcpy((char*) vector_vec(vec), datastr, len);
    Py_DECREF(rawdata);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* libpython_path(PyObject* self, PyObject* args) {
#if defined(HAVE_DLFCN_H) && !defined(MINGW)
    Dl_info info;
    int rval = dladdr((const void*) Py_Initialize, &info);
    if (!rval) {
        PyErr_SetString(PyExc_Exception,
                        "dladdr: Py_Initialize could not be matched to a shared object");
        return NULL;
    }
    if (!info.dli_fname) {
        PyErr_SetString(PyExc_Exception,
                        "dladdr: No symbol matching Py_Initialize could be found.");
        return NULL;
    }
    return Py_BuildValue("s", info.dli_fname);
#else
    Py_INCREF(Py_None);
    return Py_None;
#endif
}

// available for every HocObject
static PyMethodDef hocobj_methods[] = {
    {"baseattr", hocobj_baseattr, METH_VARARGS, "To allow use of an overrided base method"},
    {"hocobjptr", hocobj_vptr, METH_NOARGS, "Hoc Object pointer as a long int"},
    {"same",
     (PyCFunction) hocobj_same,
     METH_VARARGS,
     "o1.same(o2) return True if o1 and o2 wrap the same internal HOC Object"},
    {"hname", hocobj_name, METH_NOARGS, "More specific than __str__() or __attr__()."},
    {"__reduce__", hocpickle_reduce, METH_VARARGS, "pickle interface"},
    {"__setstate__", hocpickle_setstate, METH_VARARGS, "pickle interface"},
    {NULL, NULL, 0, NULL}};

// only for a HocTopLevelInterpreter type HocObject
static PyMethodDef toplevel_methods[] = {
    {"ref", mkref, METH_VARARGS, "Wrap to allow call by reference in a hoc function"},
    {"cas", nrnpy_cas, METH_VARARGS, "Return the currently accessed section."},
    {"allsec", nrnpy_forall, METH_VARARGS, "Return iterator over all sections."},
    {"Section",
     (PyCFunction) nrnpy_newsecobj,
     METH_VARARGS | METH_KEYWORDS,
     "Return a new Section"},
    {"setpointer", setpointer, METH_VARARGS, "Assign hoc variable address to NMODL POINTER"},
    {"libpython_path",
     libpython_path,
     METH_NOARGS,
     "Return full path to file that contains Py_Initialize()"},
    {NULL, NULL, 0, NULL}};

static void add2topdict(PyObject* dict) {
    for (PyMethodDef* meth = toplevel_methods; meth->ml_name != NULL; meth++) {
        int err;
        PyObject* nn = Py_BuildValue("s", meth->ml_doc);
        if (!nn) {
            return;
        }
        err = PyDict_SetItemString(dict, meth->ml_name, nn);
        Py_DECREF(nn);
        if (err) {
            return;
        }
    }
}

static PyObject* nrnpy_vec_math = NULL;

extern "C" int nrnpy_vec_math_register(PyObject* callback) {
    nrnpy_vec_math = callback;
    return 0;
}

extern "C" int nrnpy_rvp_pyobj_callback_register(PyObject* callback) {
    nrnpy_rvp_pyobj_callback = callback;
    return 0;
}

static bool pyobj_is_vector(PyObject* obj) {
    if (PyObject_TypeCheck(obj, hocobject_type)) {
        PyHocObject* obj_h = (PyHocObject*) obj;
        if (obj_h->type_ == PyHoc::HocObject) {
            // this is an object (e.g. Vector) not a function
            if (obj_h->ho_->ctemplate == hoc_vec_template_) {
                return true;
            }
        }
    }
    return false;
}

static PyObject* py_hocobj_math(const char* op, PyObject* obj1, PyObject* obj2) {
    bool potentially_valid = false;
    int reversed = 0;
    if (pyobj_is_vector(obj1)) {
        potentially_valid = true;
    } else if (pyobj_is_vector(obj2)) {
        potentially_valid = true;
        reversed = 1;
    }
    if (!potentially_valid) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
    char buf[8];
    return PyObject_CallFunction(nrnpy_vec_math, strcpy(buf, "siOO"), op, reversed, obj1, obj2);
}

static PyObject* py_hocobj_math_unary(const char* op, PyObject* obj) {
    if (pyobj_is_vector(obj)) {
        char buf[8];
        return PyObject_CallFunction(nrnpy_vec_math, strcpy(buf, "siO"), op, 2, obj);
    }
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
}

static PyObject* py_hocobj_add(PyObject* obj1, PyObject* obj2) {
    return py_hocobj_math("add", obj1, obj2);
}

static PyObject* py_hocobj_uabs(PyObject* obj) {
    return py_hocobj_math_unary("uabs", obj);
}

static PyObject* py_hocobj_uneg(PyObject* obj) {
    return py_hocobj_math_unary("uneg", obj);
}

static PyObject* py_hocobj_upos(PyObject* obj) {
    return py_hocobj_math_unary("upos", obj);
}

static PyObject* py_hocobj_sub(PyObject* obj1, PyObject* obj2) {
    return py_hocobj_math("sub", obj1, obj2);
}

static PyObject* py_hocobj_mul(PyObject* obj1, PyObject* obj2) {
    return py_hocobj_math("mul", obj1, obj2);
}

static PyObject* py_hocobj_div(PyObject* obj1, PyObject* obj2) {
    return py_hocobj_math("div", obj1, obj2);
}

#include "nrnpy_hoc.h"

// Figure out the endian-ness of the system, and return
// 0 (error), '<' (little endian) or '>' (big endian)
char get_endian_character() {
    char endian_character = 0;

    PyObject* psys = PyImport_ImportModule("sys");
    if (psys == NULL) {
        PyErr_SetString(PyExc_ImportError, "Failed to import sys to determine system byteorder.");
        return 0;
    }

    PyObject* pbo = PyObject_GetAttrString(psys, "byteorder");
    if (pbo == NULL) {
        PyErr_SetString(PyExc_AttributeError, "sys module does not have attribute 'byteorder'!");
        return 0;
    }

    Py2NRNString byteorder(pbo);
    if (byteorder.c_str() == NULL) {
        return 0;
    }

    if (strcmp(byteorder.c_str(), "little") == 0) {
        endian_character = '<';
    } else if (strcmp(byteorder.c_str(), "big") == 0) {
        endian_character = '>';
    } else {
        PyErr_SetString(PyExc_RuntimeError, "Unknown system native byteorder.");
        return 0;
    }
    return endian_character;
}

static void sectionlist_helper_(void* sl, Object* args) {
    if (!args || args->ctemplate->sym != nrnpy_pyobj_sym_) {
        hoc_execerror("argument must be a Python iterable", "");
    }
    PyObject* pargs = nrnpy_hoc2pyobject(args);

    PyObject* iterator = PyObject_GetIter(pargs);
    PyObject* item;

    if (iterator == NULL) {
        PyErr_Clear();
        hoc_execerror("argument must be an iterable", "");
    }

    while ((item = PyIter_Next(iterator))) {
        if (!PyObject_TypeCheck(item, psection_type)) {
            hoc_execerror("iterable must contain only Section objects", 0);
        }
        NPySecObj* pysec = (NPySecObj*) item;
        lvappendsec_and_ref(sl, pysec->sec_);
        Py_DECREF(item);
    }

    Py_DECREF(iterator);
    if (PyErr_Occurred()) {
        PyErr_Clear();
        hoc_execerror("argument must be a Python iterable", "");
    }
}

/// value of neuron.coreneuron.enable as 0, 1 (-1 if error)
extern int (*nrnpy_nrncore_enable_value_p_)();

/// value of neuron.coreneuron.file_mode as 0, 1 (-1 if error)
extern int (*nrnpy_nrncore_file_mode_value_p_)();

/*
 * Helper function to inspect value of int/boolean option
 * under coreneuron module.
 *
 * \todo : seems like this could be generalized so that
 *  additional cases would require less code.
 */
static int get_nrncore_opt_value(const char* option) {
    PyObject* modules = PyImport_GetModuleDict();
    if (modules) {
        PyObject* module = PyDict_GetItemString(modules, "neuron.coreneuron");
        if (module) {
            PyObject* val = PyObject_GetAttrString(module, option);
            if (val) {
                long enable = PyLong_AsLong(val);
                Py_DECREF(val);
                if (enable != -1) {
                    return enable;
                }
            }
        }
    }
    if (PyErr_Occurred()) {
        PyErr_Print();
        return -1;
    }
    return 0;
}

/// return value of neuron.coreneuron.enable
static int nrncore_enable_value() {
    return get_nrncore_opt_value("enable");
}

/// return value of neuron.coreneuron.file_mode
static int nrncore_file_mode_value() {
    return get_nrncore_opt_value("file_mode");
}

/** Gets the python string returned by  neuron.coreneuron.nrncore_arg(tstop)
    return a strdup() copy of the string which should be free when the caller
    finishes with it. Return NULL if error or bool(neuron.coreneuron.enable)
    is False.
*/
extern char* (*nrnpy_nrncore_arg_p_)(double tstop);
static char* nrncore_arg(double tstop) {
    PyObject* modules = PyImport_GetModuleDict();
    if (modules) {
        PyObject* module = PyDict_GetItemString(modules, "neuron.coreneuron");
        if (module) {
            PyObject* callable = PyObject_GetAttrString(module, "nrncore_arg");
            if (callable) {
                PyObject* ts = Py_BuildValue("(d)", tstop);
                if (ts) {
                    PyObject* arg = PyObject_CallObject(callable, ts);
                    Py_DECREF(ts);
                    if (arg) {
                        Py2NRNString str(arg);
                        Py_DECREF(arg);
                        if (str.err()) {
                            str.set_pyerr(
                                PyExc_TypeError,
                                "neuron.coreneuron.nrncore_arg() must return an ascii string");
                            return NULL;
                        }
                        if (strlen(str.c_str()) > 0) {
                            return strdup(str.c_str());
                        }
                    }
                }
            }
        }
    }
    if (PyErr_Occurred()) {
        PyErr_Print();
    }
    return NULL;
}


static PyType_Spec obj_spec_from_name(const char* name) {
    return {
        name,
        sizeof(PyHocObject),
        0,
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        nrnpy_HocObjectType_slots,
    };
}

PyObject* nrnpy_hoc() {
    PyObject* m;
    PyObject* bases;
    PyTypeObject* pto;
    PyType_Spec spec;
    nrnpy_vec_from_python_p_ = nrnpy_vec_from_python;
    nrnpy_vec_to_python_p_ = nrnpy_vec_to_python;
    nrnpy_vec_as_numpy_helper_ = vec_as_numpy_helper;
    nrnpy_sectionlist_helper_ = sectionlist_helper_;
    nrnpy_get_pyobj = nrnpy_get_pyobj_;
    nrnpy_decref = nrnpy_decref_;
    nrnpy_nrncore_arg_p_ = nrncore_arg;
    nrnpy_nrncore_enable_value_p_ = nrncore_enable_value;
    nrnpy_nrncore_file_mode_value_p_ = nrncore_file_mode_value;
    nrnpy_rvp_rxd_to_callable = rvp_rxd_to_callable_;
    PyLockGIL lock;

    char endian_character = 0;

    int err = 0;
    PyObject* modules = PyImport_GetModuleDict();
    if ((m = PyDict_GetItemString(modules, "hoc")) != NULL && PyModule_Check(m)) {
        return m;
    }
    m = PyModule_Create(&hocmodule);
    assert(m);
    Symbol* s = NULL;
    spec = obj_spec_from_name("hoc.HocObject");
    hocobject_type = (PyTypeObject*) PyType_FromSpec(&spec);
    if (PyType_Ready(hocobject_type) < 0)
        goto fail;
    PyModule_AddObject(m, "HocObject", (PyObject*) hocobject_type);

    bases = PyTuple_Pack(1, hocobject_type);
    Py_INCREF(bases);
    for (auto name: py_exposed_classes) {
        // TODO: obj_spec_from_name needs a hoc. prepended
        exposed_py_type_names.push_back(std::string("hoc.") + name);
        spec = obj_spec_from_name(exposed_py_type_names.back().c_str());
        pto = (PyTypeObject*) PyType_FromSpecWithBases(&spec, bases);
        sym_to_type_map[hoc_lookup(name)] = pto;
        type_to_sym_map[pto] = hoc_lookup(name);
        if (PyType_Ready(pto) < 0)
            goto fail;
        PyModule_AddObject(m, name, (PyObject*) pto);
    }
    Py_DECREF(bases);

    topmethdict = PyDict_New();
    for (PyMethodDef* meth = toplevel_methods; meth->ml_name != NULL; meth++) {
        PyObject* descr;
        int err;
        descr = PyDescr_NewMethod(hocobject_type, meth);
        assert(descr);
        err = PyDict_SetItemString(topmethdict, meth->ml_name, descr);
        Py_DECREF(descr);
        if (err < 0) {
            goto fail;
        }
    }

    s = hoc_lookup("Vector");
    assert(s);
    hoc_vec_template_ = s->u.ctemplate;
    sym_vec_x = hoc_table_lookup("x", s->u.ctemplate->symtable);
    assert(sym_vec_x);
    s = hoc_lookup("List");
    assert(s);
    hoc_list_template_ = s->u.ctemplate;
    s = hoc_lookup("SectionList");
    assert(s);
    hoc_sectionlist_template_ = s->u.ctemplate;
    s = hoc_lookup("Matrix");
    assert(s);
    sym_mat_x = hoc_table_lookup("x", s->u.ctemplate->symtable);
    assert(sym_mat_x);
    s = hoc_lookup("NetCon");
    assert(s);
    sym_netcon_weight = hoc_table_lookup("weight", s->u.ctemplate->symtable);
    assert(sym_netcon_weight);

    nrnpy_nrn();
    endian_character = get_endian_character();
    if (endian_character == 0)
        goto fail;
    array_interface_typestr[0] = endian_character;

    // Setup bytesize in typestr
    snprintf(array_interface_typestr + 2, 3, "%ld", sizeof(double));
    err = PyDict_SetItemString(modules, "hoc", m);
    assert(err == 0);
    //  Py_DECREF(m);
    return m;
fail:
    return NULL;
}

void nrnpython_reg_real_nrnpy_hoc_cpp(neuron::python::impl_ptrs* ptrs) {
    ptrs->gui_helper = gui_helper_;
    ptrs->gui_helper3 = gui_helper_3_;
    ptrs->gui_helper3_str = gui_helper_3_str_;
    ptrs->object_to_double = object_to_double_;
}
