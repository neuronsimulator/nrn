#include "nrn_ansi.h"
#include "nrnpython.h"
#include <structmember.h>
#include <InterViews/resource.h>
#include "nrniv_mf.h"
#include <nrnoc2iv.h>
#include "nrnpy.h"
#include "nrnpy_utils.h"
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#include <membfunc.h>
#include <parse.hpp>

#include <cmath>
#include <cstring>
extern void nrn_pt3dremove(Section* sec, int i0);
extern void nrn_pt3dinsert(Section* sec, int i0, double x, double y, double z, double d);
extern void nrn_pt3dclear(Section* sec, int req);
extern void nrn_pt3dchange1(Section* sec, int i, double d);
extern void nrn_pt3dchange2(Section* sec, int i, double x, double y, double z, double diam);
extern void stor_pt3d(Section* sec, double x, double y, double z, double d);
extern void nrn_pt3dstyle1(Section* sec, double x, double y, double z);
extern void nrn_pt3dstyle0(Section* sec);
extern void nrn_area_ri(Section* sec);
extern void sec_free(hoc_Item*);
extern Symlist* hoc_built_in_symlist;
extern Section* nrn_noerr_access();
extern PyObject* nrn_ptr_richcmp(void* self_ptr, void* other_ptr, int op);
extern int has_membrane(char*, Section*);
// used to be static in nrnpy_hoc.cpp
extern int hocobj_pushargs(PyObject*, std::vector<char*>&);
extern void hocobj_pushargs_free_strings(std::vector<char*>&);

typedef struct {
    PyObject_HEAD
    NPySecObj* pysec_;
    int allseg_iter_;
} NPyAllSegOfSecIter;

typedef struct {
    PyObject_HEAD
    NPySecObj* pysec_;
    int seg_iter_;
} NPySegOfSecIter;

typedef struct {
    PyObject_HEAD
    NPySecObj* pysec_;
    double x_;
} NPySegObj;

typedef struct {
    PyObject_HEAD
    NPySegObj* pyseg_;
    Prop* prop_;
    // Following cannot be initialized when NPyMechObj allocated by Python. See new_pymechobj
    // wrapper.
    neuron::container::non_owning_identifier_without_container prop_id_;
    int type_;
} NPyMechObj;

typedef struct {
    PyObject_HEAD
    NPyMechObj* pymech_;
} NPyMechOfSegIter;

typedef struct {
    PyObject_HEAD
    NPyMechObj* pymech_;
    NPyDirectMechFunc* f_;
} NPyMechFunc;

typedef struct {
    PyObject_HEAD
    NPyMechObj* pymech_;
    Symbol* msym_;
    int i_;
} NPyVarOfMechIter;

typedef struct {
    PyObject_HEAD
    NPyMechObj* pymech_;
    int index_;
} NPyRVItr;

typedef struct {
    PyObject_HEAD
    NPyMechObj* pymech_;
    Symbol* sym_;
    int isptr_;
    int attr_from_sec_;  // so section.xraxial[0] = e assigns to all segments.
} NPyRangeVar;

PyTypeObject* psection_type;
static PyTypeObject* pallseg_of_sec_iter_type;
static PyTypeObject* pseg_of_sec_iter_type;
static PyTypeObject* psegment_type;
static PyTypeObject* pmech_of_seg_iter_generic_type;
static PyTypeObject* pmech_generic_type;
static PyTypeObject* pmechfunc_generic_type;
static PyTypeObject* pvar_of_mech_iter_generic_type;
static PyTypeObject* range_type;

PyObject* pmech_types;  // Python map for name to Mechanism
PyObject* rangevars_;   // Python map for name to Symbol

extern PyTypeObject* hocobject_type;
extern Section* nrnpy_newsection(NPySecObj*);
extern void simpleconnectsection();
extern void nrn_change_nseg(Section*, int);
extern double section_length(Section*);
extern double nrn_ra(Section*);
extern int can_change_morph(Section*);
extern short* nrn_is_artificial_;
extern cTemplate** nrn_pnt_template_;
extern void nrn_diam_change(Section*);
extern void nrn_length_change(Section*, double);
extern void mech_insert1(Section*, int);
extern void mech_uninsert1(Section*, Symbol*);
extern PyObject* nrnpy_forall(PyObject* self, PyObject* args);
static void nrnpy_reg_mech(int);
extern void (*nrnpy_reg_mech_p_)(int);
static int ob_is_seg(Object*);
extern int (*nrnpy_ob_is_seg)(Object*);
static Object* seg_from_sec_x(Section*, double x);
extern Object* (*nrnpy_seg_from_sec_x)(Section*, double x);
static Section* o2sec(Object*);
extern Section* (*nrnpy_o2sec_p_)(Object*);
static void o2loc(Object*, Section**, double*);
extern void (*nrnpy_o2loc_p_)(Object*, Section**, double*);
extern void (*nrnpy_o2loc2_p_)(Object*, Section**, double*);
static void nrnpy_unreg_mech(int);
extern char* (*nrnpy_pysec_name_p_)(Section*);
static char* pysec_name(Section*);
extern Object* (*nrnpy_pysec_cell_p_)(Section*);
static Object* pysec_cell(Section*);
static PyObject* pysec2cell(NPySecObj*);
extern int (*nrnpy_pysec_cell_equals_p_)(Section*, Object*);
static int pysec_cell_equals(Section*, Object*);
static void remake_pmech_types();

void nrnpy_sec_referr() {
    PyErr_SetString(PyExc_ReferenceError, "can't access a deleted section");
}

void nrnpy_prop_referr() {
    PyErr_SetString(PyExc_ReferenceError, "mechanism instance is invalid");
}

static char* pysec_name(Section* sec) {
    static char buf[512];
    if (sec->prop) {
        auto* ps = static_cast<NPySecObj*>(sec->prop->dparam[PROP_PY_INDEX].get<void*>());
        buf[0] = '\0';
        char* cp = buf + strlen(buf);
        if (ps->name_) {
            Sprintf(buf, "%s", ps->name_);
        } else {
            Sprintf(buf, "__nrnsec_%p", sec);
        }
        return buf;
    }
    return 0;
}

static Object* pysec_cell(Section* sec) {
    if (auto* pv = sec->prop->dparam[PROP_PY_INDEX].get<void*>(); sec->prop && pv) {
        PyObject* cell_weakref = static_cast<NPySecObj*>(pv)->cell_weakref_;
        if (cell_weakref) {
            PyObject* cell = PyWeakref_GetObject(cell_weakref);
            if (!cell) {
                PyErr_Print();
                hoc_execerror("Error getting cell for", secname(sec));
            } else if (cell != Py_None) {
                return nrnpy_po2ho(cell);
            }
        }
    }
    return NULL;
}

static int NPySecObj_contains(PyObject* sec, PyObject* obj) {
    /* report that we contain the object if it has a .sec that is equal to ourselves */
    PyObject* obj_sec;
    int result;
    if (!PyObject_HasAttrString(obj, "sec")) {
        return 0;
    }
    Py_INCREF(obj);
    obj_sec = PyObject_GetAttrString(obj, "sec");
    Py_DECREF(obj);
    result = PyObject_RichCompareBool(sec, obj_sec, Py_EQ);
    Py_XDECREF(obj_sec);
    return (result);
}

static int pysec_cell_equals(Section* sec, Object* obj) {
    if (auto* pv = sec->prop->dparam[PROP_PY_INDEX].get<void*>(); sec->prop && pv) {
        PyObject* cell_weakref = static_cast<NPySecObj*>(pv)->cell_weakref_;
        if (cell_weakref) {
            PyObject* cell = PyWeakref_GetObject(cell_weakref);
            if (!cell) {
                PyErr_Print();
                hoc_execerror("Error getting cell for", secname(sec));
            }
            return nrnpy_ho_eq_po(obj, cell);
        }
        return nrnpy_ho_eq_po(obj, Py_None);
    }
    return 0;
}

static void NPySecObj_dealloc(NPySecObj* self) {
    // printf("NPySecObj_dealloc %p %s\n", self, secname(self->sec_));
    if (self->sec_) {
        if (self->name_) {
            nrnpy_pysecname2sec_remove(self->sec_);
            delete[] self->name_;
        }
        Py_XDECREF(self->cell_weakref_);
        if (self->sec_->prop) {
            self->sec_->prop->dparam[PROP_PY_INDEX] = nullptr;
        }
        if (self->sec_->prop && !self->sec_->prop->dparam[0].get<Symbol*>()) {
            sec_free(self->sec_->prop->dparam[8].get<hoc_Item*>());
        } else {
            section_unref(self->sec_);
        }
    }
    ((PyObject*) self)->ob_type->tp_free((PyObject*) self);
}

static void NPyAllSegOfSecIter_dealloc(NPyAllSegOfSecIter* self) {
    // printf("NPyAllSegOfSecIter_dealloc %p %s\n", self, secname(self->pysec_->sec_));
    Py_XDECREF(self->pysec_);
    ((PyObject*) self)->ob_type->tp_free((PyObject*) self);
}

static void NPySegOfSecIter_dealloc(NPySegOfSecIter* self) {
    // printf("NPySegOfSecIter_dealloc %p %s\n", self, secname(self->pysec_->sec_));
    Py_XDECREF(self->pysec_);
    ((PyObject*) self)->ob_type->tp_free((PyObject*) self);
}

static void NPySegObj_dealloc(NPySegObj* self) {
    // printf("NPySegObj_dealloc %p\n", self);
    Py_XDECREF(self->pysec_);
    ((PyObject*) self)->ob_type->tp_free((PyObject*) self);
}

static void NPyRangeVar_dealloc(NPyRangeVar* self) {
    // printf("NPyRangeVar_dealloc %p\n", self);
    Py_XDECREF(self->pymech_);
    ((PyObject*) self)->ob_type->tp_free((PyObject*) self);
}

static void NPyMechObj_dealloc(NPyMechObj* self) {
    // printf("NPyMechObj_dealloc %p %s\n", self, self->ob_type->tp_name);
    Py_XDECREF(self->pyseg_);
    // Must manually call destructor since it was manually constructed in new_pymechobj wrapper
    self->prop_id_.~non_owning_identifier_without_container();
    ((PyObject*) self)->ob_type->tp_free((PyObject*) self);
}

static NPyMechObj* new_pymechobj() {
    NPyMechObj* m = PyObject_New(NPyMechObj, pmech_generic_type);
    if (m) {
        // Use "placement new" idiom since Python C allocation cannot call the initializer to start
        // it as "null". So later `a = b` might segfault because copy constructor decrements the
        // refcount of `a`s nonsense memory.
        new (&m->prop_id_) neuron::container::non_owning_identifier_without_container;
    }

    return m;
}

// Only call if p is valid
static NPyMechObj* new_pymechobj(NPySegObj* pyseg, Prop* p) {
    NPyMechObj* m = new_pymechobj();
    if (!m) {
        return NULL;
    }
    m->pyseg_ = pyseg;
    Py_INCREF(m->pyseg_);
    m->prop_ = p;
    m->prop_id_ = p->id();
    m->type_ = p->_type;
    return m;
}

static void NPyMechFunc_dealloc(NPyMechFunc* self) {
    // printf("NPyMechFunc_dealloc %p %s\n", self, self->ob_type->tp_name);
    Py_XDECREF(self->pymech_);
    ((PyObject*) self)->ob_type->tp_free((PyObject*) self);
}

static void NPyMechOfSegIter_dealloc(NPyMechOfSegIter* self) {
    // printf("NPyMechOfSegIter_dealloc %p %s\n", self, self->ob_type->tp_name);
    Py_XDECREF(self->pymech_);
    ((PyObject*) self)->ob_type->tp_free((PyObject*) self);
}

static void NPyVarOfMechIter_dealloc(NPyVarOfMechIter* self) {
    // printf("NPyVarOfMechIter_dealloc %p %s\n", self, self->ob_type->tp_name);
    Py_XDECREF(self->pymech_);
    ((PyObject*) self)->ob_type->tp_free((PyObject*) self);
}

// A new (or inited) python Section object with no arg creates a nrnoc section
// which persists only while the python object exists. The nrnoc section
// structure
// has no hoc Symbol but the Python Section pointer is filled in
// and secname(sec) returns the Python Section object name from the hoc section.
// If a Python Section object is created from an existing nrnoc section
// (with a filled in Symbol) the nrnoc section will continue to exist until
// the hoc delete_section() is called on it.
//

static int NPySecObj_init(NPySecObj* self, PyObject* args, PyObject* kwds) {
    // printf("NPySecObj_init %p %p\n", self, self->sec_);
    static const char* kwlist[] = {"name", "cell", NULL};
    if (self != NULL && !self->sec_) {
        if (self->name_) {
            delete[] self->name_;
        }
        self->name_ = 0;
        self->cell_weakref_ = 0;
        char* name = 0;
        PyObject* cell = 0;
        // avoid "warning: deprecated conversion from string constant to char*"
        // someday eliminate the (char**) when python changes their prototype
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sO", (char**) kwlist, &name, &cell)) {
            return -1;
        }
        if (cell && cell != Py_None) {
            self->cell_weakref_ = PyWeakref_NewRef(cell, NULL);
            if (!self->cell_weakref_) {
                return -1;
            }
        } else {
            cell = 0;
        }
        if (name) {
            size_t n = strlen(name) + 1;  // include terminator

            if (cell) {
                // include cellname in name so nrnpy_pysecname2sec_remove can determine

                cell = PyObject_Str(cell);
                if (cell == NULL) {
                    Py_XDECREF(self->cell_weakref_);
                    return -1;
                }
                Py2NRNString str(cell);
                Py_DECREF(cell);
                if (str.err()) {
                    str.set_pyerr(PyExc_TypeError, "cell name contains non ascii character");
                    return -1;
                }
                char* cp = str.c_str();
                n += strlen(cp) + 1;  // include dot
                self->name_ = new char[n];
                std::snprintf(self->name_, n, "%s.%s", cp, name);
            } else {
                self->name_ = new char[n];
                std::strncpy(self->name_, name, n);
            }
        }
        self->sec_ = nrnpy_newsection(self);
        nrnpy_pysecname2sec_add(self->sec_);
    }
    return 0;
}

static int NPyAllSegOfSecIter_init(NPyAllSegOfSecIter* self, PyObject* args, PyObject* kwds) {
    NPySecObj* pysec;
    // printf("NPyAllSegOfSecIter_init %p %p\n", self, self->sec_);
    if (self != NULL && !self->pysec_) {
        if (!PyArg_ParseTuple(args, "O!", psection_type, &pysec)) {
            return -1;
        }
        self->allseg_iter_ = 0;
        self->pysec_ = pysec;
        Py_INCREF(pysec);
    }
    return 0;
}

PyObject* NPySecObj_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    NPySecObj* self;
    self = (NPySecObj*) type->tp_alloc(type, 0);
    // printf("NPySecObj_new %p\n", self);
    if (self != NULL) {
        if (NPySecObj_init(self, args, kwds) != 0) {
            Py_DECREF(self);
            return NULL;
        }
    }
    return (PyObject*) self;
}

PyObject* NPyAllSegOfSecIter_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    NPyAllSegOfSecIter* self;
    self = (NPyAllSegOfSecIter*) type->tp_alloc(type, 0);
    // printf("NPyAllSegOfSecIter_new %p\n", self);
    if (self != NULL) {
        if (NPyAllSegOfSecIter_init(self, args, kwds) != 0) {
            Py_DECREF(self);
            return NULL;
        }
    }
    return (PyObject*) self;
}

PyObject* nrnpy_newsecobj(PyObject* self, PyObject* args, PyObject* kwds) {
    return NPySecObj_new(psection_type, args, kwds);
}

static PyObject* NPySegObj_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    NPySecObj* pysec;
    double x;
    if (!PyArg_ParseTuple(args, "O!d", psection_type, &pysec, &x)) {
        return NULL;
    }
    if (x > 1.0 && x < 1.0001) {
        x = 1.0;
    }
    if (x < 0. || x > 1.0) {
        PyErr_SetString(PyExc_ValueError, "segment position range is 0 <= x <= 1");
        return NULL;
    }
    NPySegObj* self;
    self = (NPySegObj*) type->tp_alloc(type, 0);
    // printf("NPySegObj_new %p\n", self);
    if (self != NULL) {
        self->pysec_ = pysec;
        self->x_ = x;
        Py_INCREF(self->pysec_);
    }
    return (PyObject*) self;
}

static PyObject* NPyMechObj_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    NPySegObj* pyseg;
    if (!PyArg_ParseTuple(args, "O!", psegment_type, &pyseg)) {
        return NULL;
    }
    NPyMechObj* self;
    self = (NPyMechObj*) type->tp_alloc(type, 0);
    // printf("NPyMechObj_new %p %s\n", self,
    // ((PyObject*)self)->ob_type->tp_name);
    if (self != NULL) {
        new (self) NPyMechObj;
        self->pyseg_ = pyseg;
        Py_INCREF(self->pyseg_);
    }
    return (PyObject*) self;
}

static PyObject* NPyRangeVar_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    NPyRangeVar* self;
    self = (NPyRangeVar*) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->pymech_ = NULL;
        self->sym_ = NULL;
        self->isptr_ = 0;
        self->attr_from_sec_ = 0;
    }
    return (PyObject*) self;
}

static int NPySegObj_init(NPySegObj* self, PyObject* args, PyObject* kwds) {
    // printf("NPySegObj_init %p %p\n", self, self->pysec_);
    NPySecObj* pysec;
    double x;
    if (!PyArg_ParseTuple(args, "O!d", psection_type, &pysec, &x)) {
        return -1;
    }
    if (x > 1.0 && x < 1.0001) {
        x = 1.0;
    }
    if (x < 0. || x > 1.0) {
        PyErr_SetString(PyExc_ValueError, "segment position range is 0 <= x <= 1");
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

static int ob_is_seg(Object* o) {
    if (!o || o->ctemplate->sym != nrnpy_pyobj_sym_) {
        return 0;
    }
    PyObject* po = nrnpy_hoc2pyobject(o);
    if (!PyObject_TypeCheck(po, psegment_type)) {
        return 0;
    }
    return 1;
}

static Section* o2sec(Object* o) {
    if (o->ctemplate->sym != nrnpy_pyobj_sym_) {
        hoc_execerror("not a Python nrn.Section", 0);
    }
    PyObject* po = nrnpy_hoc2pyobject(o);
    if (!PyObject_TypeCheck(po, psection_type)) {
        hoc_execerror("not a Python nrn.Section", 0);
    }
    NPySecObj* pysec = (NPySecObj*) po;
    return pysec->sec_;
}

static void o2loc(Object* o, Section** psec, double* px) {
    if (o->ctemplate->sym != nrnpy_pyobj_sym_) {
        hoc_execerror("not a Python nrn.Segment", 0);
    }
    PyObject* po = nrnpy_hoc2pyobject(o);
    if (!PyObject_TypeCheck(po, psegment_type)) {
        hoc_execerror("not a Python nrn.Segment", 0);
    }
    NPySegObj* pyseg = (NPySegObj*) po;
    *psec = pyseg->pysec_->sec_;
    if (!(*psec)->prop) {
        hoc_execerr_ext("nrn.Segment associated with deleted internal Section");
    }
    *px = pyseg->x_;
}


static void o2loc2(Object* o, Section** psec, double* px) {
    bool free_po = false;
    if (o->ctemplate->sym != nrnpy_pyobj_sym_) {
        hoc_execerror("not a Python nrn.Segment, rxd.node, or other with a segment property", 0);
    }
    PyObject* po = nrnpy_hoc2pyobject(o);
    if (!PyObject_TypeCheck(po, psegment_type)) {
        if (PyList_Check(po)) {
            if (PyList_Size(po) != 1) {
                hoc_execerror("If a list is supplied, it must be of length 1", 0);
            } else {
                PyObject* old_po = po;
                Py_INCREF(old_po);
                po = PyList_GetItem(po, 0);
                Py_DECREF(old_po);
                free_po = true;
            }
        }
        if (!PyObject_HasAttrString(po, "segment")) {
            if (free_po) {
                Py_DECREF(po);
            }
            hoc_execerror("not a Python nrn.Segment, rxd.node, or other with a segment property",
                          0);
        }
        PyObject* obj = po;
        Py_INCREF(obj);
        po = PyObject_GetAttrString(obj, "segment");
        Py_DECREF(obj);
        if (free_po) {
            // don't need the element from the list anymore
            Py_DECREF(obj);
        }
        free_po = true;
    }
    NPySegObj* pyseg = (NPySegObj*) po;
    *psec = pyseg->pysec_->sec_;
    *px = pyseg->x_;
    if (free_po) {
        Py_DECREF(po);
    }
    if (!(*psec)->prop) {
        hoc_execerr_ext("nrn.Segment associated with deleted internal Section");
    }
}

static int NPyMechObj_init(NPyMechObj* self, PyObject* args, PyObject* kwds) {
    // printf("NPyMechObj_init %p %p %s\n", self, self->pyseg_,
    // ((PyObject*)self)->ob_type->tp_name);
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

static PyObject* NPySecObj_n3d(NPySecObj* self) {
    CHECK_SEC_INVALID(self->sec_);
    return PyInt_FromLong(self->sec_->npt3d);
}

static PyObject* NPySecObj_pt3dremove(NPySecObj* self, PyObject* args) {
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    int i0, n;
    if (!PyArg_ParseTuple(args, "i", &i0)) {
        return NULL;
    }
    if (i0 < 0 || i0 >= sec->npt3d) {
        PyErr_SetString(PyExc_Exception, "Arg out of range\n");
        return NULL;
    }

    nrn_pt3dremove(sec, i0);
    Py_RETURN_NONE;
}

static PyObject* NPySecObj_pt3dclear(NPySecObj* self, PyObject* args) {
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    int req = 0;
    Py_ssize_t narg = PyTuple_GET_SIZE(args);
    if (narg) {
        if (!PyArg_ParseTuple(args, "i", &req)) {
            return NULL;
        }
    }
    if (req < 0) {
        PyErr_SetString(PyExc_Exception, "Arg out of range\n");
        return NULL;
    }
    nrn_pt3dclear(sec, req);
    return PyInt_FromLong(sec->pt3d_bsize);
}

static PyObject* NPySecObj_pt3dchange(NPySecObj* self, PyObject* args) {
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    int i;
    double x, y, z, diam;
    Py_ssize_t narg = PyTuple_GET_SIZE(args);
    if (narg == 2) {
        if (!PyArg_ParseTuple(args, "id", &i, &diam)) {
            return NULL;
        }
        if (i < 0 || i >= sec->npt3d) {
            PyErr_SetString(PyExc_Exception, "Arg out of range\n");
            return NULL;
        }
        nrn_pt3dchange1(sec, i, diam);
    } else if (narg == 5) {
        if (!PyArg_ParseTuple(args, "idddd", &i, &x, &y, &z, &diam)) {
            return NULL;
        }
        if (i < 0 || i >= sec->npt3d) {
            PyErr_SetString(PyExc_Exception, "Arg out of range\n");
            return NULL;
        }
        nrn_pt3dchange2(sec, i, x, y, z, diam);
    } else {
        PyErr_SetString(PyExc_Exception, "Wrong number of arguments\n");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject* NPySecObj_pt3dinsert(NPySecObj* self, PyObject* args) {
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    int i;
    double x, y, z, d;
    if (!PyArg_ParseTuple(args, "idddd", &i, &x, &y, &z, &d)) {
        return NULL;
    }
    if (i < 0 || i > sec->npt3d) {
        PyErr_SetString(PyExc_Exception, "Arg out of range\n");
        return NULL;
    }
    nrn_pt3dinsert(sec, i, x, y, z, d);
    Py_RETURN_NONE;
}

static PyObject* NPySecObj_pt3dadd(NPySecObj* self, PyObject* args) {
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    double x, y, z, d;
    // TODO: add support for iterables
    if (!PyArg_ParseTuple(args, "dddd", &x, &y, &z, &d)) {
        return NULL;
    }
    stor_pt3d(sec, x, y, z, d);
    Py_RETURN_NONE;
}

static PyObject* NPySecObj_pt3dstyle(NPySecObj* self, PyObject* args) {
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    int style;
    double x, y, z;
    Py_ssize_t narg = PyTuple_GET_SIZE(args);

    if (narg) {
        if (narg == 1) {
            if (!PyArg_ParseTuple(args, "i", &style)) {
                return NULL;
            }
            if (style) {
                PyErr_SetString(PyExc_AttributeError, "If exactly one argument, it must be 0.");
                return NULL;
            }
            nrn_pt3dstyle0(sec);
        } else if (narg == 4) {
            // TODO: figure out some way of reading the logical connection point
            // don't use hoc refs
            if (!PyArg_ParseTuple(args, "iddd", &style, &x, &y, &z)) {
                return NULL;
            }
            nrn_pt3dstyle1(sec, x, y, z);

        } else {
            PyErr_SetString(PyExc_Exception, "Wrong number of arguments.");
            return NULL;
        }
    }

    if (sec->logical_connection) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* NPySecObj_x3d(NPySecObj* self,
                               PyObject* args) {  // returns x value at index of 3d list
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    int n, i;
    if (!PyArg_ParseTuple(args, "i", &i)) {
        return NULL;
    }
    n = sec->npt3d - 1;
    if (i < 0 || i > n) {
        PyErr_SetString(PyExc_Exception, "Arg out of range\n");
        return NULL;
    }
    return PyFloat_FromDouble((double) sec->pt3d[i].x);
}

static PyObject* NPySecObj_y3d(NPySecObj* self,
                               PyObject* args) {  // returns y value at index of 3d list
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    int n, i;
    if (!PyArg_ParseTuple(args, "i", &i)) {
        return NULL;
    }
    n = sec->npt3d - 1;
    if (i < 0 || i > n) {
        PyErr_SetString(PyExc_Exception, "Arg out of range\n");
        return NULL;
    }
    return PyFloat_FromDouble((double) sec->pt3d[i].y);
}

static PyObject* NPySecObj_z3d(NPySecObj* self,
                               PyObject* args) {  // returns z value at index of 3d list
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    int n, i;
    if (!PyArg_ParseTuple(args, "i", &i)) {
        return NULL;
    }
    n = sec->npt3d - 1;
    if (i < 0 || i > n) {
        PyErr_SetString(PyExc_Exception, "Arg out of range\n");
        return NULL;
    }
    return PyFloat_FromDouble((double) sec->pt3d[i].z);
}

static PyObject* NPySecObj_arc3d(NPySecObj* self,
                                 PyObject* args) {  // returns arc position value at index of 3d
                                                    // list
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    int n, i;
    if (!PyArg_ParseTuple(args, "i", &i)) {
        return NULL;
    }
    n = sec->npt3d - 1;
    if (i < 0 || i > n) {
        PyErr_SetString(PyExc_Exception, "Arg out of range\n");
        return NULL;
    }
    return PyFloat_FromDouble((double) sec->pt3d[i].arc);
}

static PyObject* NPySecObj_diam3d(NPySecObj* self,
                                  PyObject* args) {  // returns diam value at index of 3d list
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    int n, i;
    if (!PyArg_ParseTuple(args, "i", &i)) {
        return NULL;
    }
    n = sec->npt3d - 1;
    if (i < 0 || i > n) {
        PyErr_SetString(PyExc_Exception, "Arg out of range\n");
        return NULL;
    }
    return PyFloat_FromDouble((double) fabs(sec->pt3d[i].d));
}

static PyObject* NPySecObj_spine3d(NPySecObj* self,
                                   PyObject* args) {  // returns True/False depending on if spine
                                                      // present
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    int n, i;
    if (!PyArg_ParseTuple(args, "i", &i)) {
        return NULL;
    }
    n = sec->npt3d - 1;
    if (i < 0 || i > n) {
        PyErr_SetString(PyExc_Exception, "Arg out of range\n");
        return NULL;
    }
    if (sec->pt3d[i].d < 0) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* pysec_repr(PyObject* p) {
    NPySecObj* psec = (NPySecObj*) p;
    if (psec->sec_ && psec->sec_->prop) {
        return NPySecObj_name(psec);
    }
    return PyString_FromString("<deleted section>");
}

static PyObject* pyseg_repr(PyObject* p) {
    NPySegObj* pyseg = (NPySegObj*) p;
    if (pyseg->pysec_->sec_ && pyseg->pysec_->sec_->prop) {
        const char* sname = secname(pyseg->pysec_->sec_);
        auto const name_size = strlen(sname) + 100;
        char* name = new char[name_size];
        std::snprintf(name, name_size, "%s(%g)", sname, pyseg->x_);
        PyObject* result = PyString_FromString(name);
        delete[] name;
        return result;
    }
    return PyString_FromString("<segment of deleted section>");
}

static PyObject* hoc_internal_name(NPySecObj* self) {
    PyObject* result;
    char buf[256];
    Sprintf(buf, "__nrnsec_%p", self->sec_);
    result = PyString_FromString(buf);
    return result;
}

static PyObject* nrnpy_psection;
static PyObject* nrnpy_set_psection(PyObject* self, PyObject* args) {
    PyObject* po;
    if (!PyArg_ParseTuple(args, "O", &po)) {
        return NULL;
    }
    if (PyCallable_Check(po) == 0) {
        PyErr_SetString(PyExc_TypeError, "argument must be a callable");
        return NULL;
    }
    if (nrnpy_psection) {
        Py_DECREF(nrnpy_psection);
        nrnpy_psection = NULL;
    }
    nrnpy_psection = po;
    Py_INCREF(po);
    return po;
}

static PyObject* NPySecObj_psection(NPySecObj* self) {
    CHECK_SEC_INVALID(self->sec_);
    if (nrnpy_psection) {
        PyObject* arglist = Py_BuildValue("(O)", self);
        PyObject* result = PyObject_CallObject(nrnpy_psection, arglist);
        Py_DECREF(arglist);
        return result;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* is_pysec(NPySecObj* self) {
    CHECK_SEC_INVALID(self->sec_);
    if (self->sec_->prop && self->sec_->prop->dparam[PROP_PY_INDEX].get<void*>()) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

NPySecObj* newpysechelp(Section* sec) {
    if (!sec || !sec->prop) {
        return NULL;
    }
    NPySecObj* pysec = NULL;
    if (auto* pv = sec->prop->dparam[PROP_PY_INDEX].get<void*>(); pv) {
        pysec = static_cast<NPySecObj*>(pv);
        Py_INCREF(pysec);
        assert(pysec->sec_ == sec);
    } else {
        pysec = (NPySecObj*) psection_type->tp_alloc(psection_type, 0);
        pysec->sec_ = sec;
        section_ref(sec);
        pysec->name_ = 0;
        pysec->cell_weakref_ = 0;
    }
    return pysec;
}

static PyObject* newpyseghelp(Section* sec, double x) {
    NPySegObj* seg = (NPySegObj*) PyObject_New(NPySegObj, psegment_type);
    if (seg == NULL) {
        return NULL;
    }
    seg->x_ = x;
    seg->pysec_ = newpysechelp(sec);
    return (PyObject*) seg;
}

static PyObject* pysec_disconnect(NPySecObj* self) {
    CHECK_SEC_INVALID(self->sec_);
    nrn_disconnect(self->sec_);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* pysec_parentseg(NPySecObj* self) {
    CHECK_SEC_INVALID(self->sec_);
    Section* psec = self->sec_->parentsec;
    if (psec == NULL || psec->prop == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    double x = nrn_connection_position(self->sec_);
    return newpyseghelp(psec, x);
}

static PyObject* pysec_trueparentseg(NPySecObj* self) {
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    Section* psec = NULL;
    for (psec = sec->parentsec; psec; psec = psec->parentsec) {
        if (psec == NULL || psec->prop == NULL) {
            Py_INCREF(Py_None);
            return Py_None;
        }
        if (nrn_at_beginning(sec)) {
            sec = psec;
        } else {
            break;
        }
    }
    if (psec == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    double x = nrn_connection_position(sec);
    return newpyseghelp(psec, x);
}

static PyObject* pysec_orientation(NPySecObj* self) {
    CHECK_SEC_INVALID(self->sec_);
    double x = nrn_section_orientation(self->sec_);
    return Py_BuildValue("d", x);
}

static bool lappendsec(PyObject* const sl, Section* const s) {
    PyObject* item = (PyObject*) newpysechelp(s);
    if (!item) {
        return false;
    }
    if (PyList_Append(sl, item) != 0) {
        return false;
    }
    Py_XDECREF(item);
    return true;
}

static PyObject* pysec_children1(PyObject* const sl, Section* const sec) {
    for (Section* s = sec->child; s; s = s->sibling) {
        if (!lappendsec(sl, s)) {
            return NULL;
        }
    }
    return sl;
}

static PyObject* pysec_children(NPySecObj* const self) {
    Section* const sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    PyObject* const result = PyList_New(0);
    if (!result) {
        return NULL;
    }
    return pysec_children1(result, sec);
}

static PyObject* pysec_subtree1(PyObject* const sl, Section* const sec) {
    if (!lappendsec(sl, sec)) {
        return NULL;
    }
    for (Section* s = sec->child; s; s = s->sibling) {
        if (!pysec_subtree1(sl, s)) {
            return NULL;
        }
    }
    return sl;
}

static PyObject* pysec_subtree(NPySecObj* const self) {
    Section* const sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    PyObject* const result = PyList_New(0);
    if (!result) {
        return NULL;
    }
    return pysec_subtree1(result, sec);
}

static PyObject* pysec_wholetree(NPySecObj* const self) {
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    Section* s;
    PyObject* result = PyList_New(0);
    if (!result) {
        return NULL;
    }
    for (s = sec; s->parentsec; s = s->parentsec) {
    }
    return pysec_subtree1(result, s);
}

static PyObject* pysec2cell(NPySecObj* self) {
    PyObject* result;
    if (self->cell_weakref_) {
        result = PyWeakref_GET_OBJECT(self->cell_weakref_);
        Py_INCREF(result);
    } else if (auto* o = self->sec_->prop->dparam[6].get<Object*>(); self->sec_->prop && o) {
        result = nrnpy_ho2po(o);
    } else {
        result = Py_None;
        Py_INCREF(result);
    }
    return result;
}

static long pysec_hash(PyObject* self) {
    return castptr2long((NPySecObj*) self)->sec_;
}

static long pyseg_hash(PyObject* self) {
    NPySegObj* seg = (NPySegObj*) self;
    return castptr2long node_exact(seg->pysec_->sec_, seg->x_);
}

static PyObject* pyseg_richcmp(NPySegObj* self, PyObject* other, int op) {
    PyObject* pysec;
    bool result = false;
    NPySegObj* seg = (NPySegObj*) self;
    void* self_ptr = (void*) node_exact(seg->pysec_->sec_, seg->x_);
    void* other_ptr = (void*) other;
    if (PyObject_TypeCheck(other, psegment_type)) {
        seg = (NPySegObj*) other;
        other_ptr = (void*) node_exact(seg->pysec_->sec_, seg->x_);
    }
    return nrn_ptr_richcmp(self_ptr, other_ptr, op);
}

static PyObject* pysec_richcmp(NPySecObj* self, PyObject* other, int op) {
    PyObject* pysec;
    bool result = false;
    void* self_ptr = (void*) (self->sec_);
    void* other_ptr = (void*) other;
    if (PyObject_TypeCheck(other, psection_type)) {
        void* self_ptr = (void*) (self->sec_);
        other_ptr = (void*) (((NPySecObj*) other)->sec_);
        return nrn_ptr_richcmp(self_ptr, other_ptr, op);
    }
    if (PyObject_TypeCheck(other, hocobject_type) || PyObject_TypeCheck(other, psegment_type)) {
        // preserves comparison with NEURON objects as it existed prior to 7.7
        return nrn_ptr_richcmp(self_ptr, other_ptr, op);
    }
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
}

static PyObject* pysec_same(NPySecObj* self, PyObject* args) {
    PyObject* pysec;
    if (PyArg_ParseTuple(args, "O", &pysec)) {
        if (PyObject_TypeCheck(pysec, psection_type)) {
            if (((NPySecObj*) pysec)->sec_ == self->sec_) {
                Py_RETURN_TRUE;
            }
        }
    }
    Py_RETURN_FALSE;
}

static PyObject* NPyMechObj_name(NPyMechObj* self) {
    std::string s = memb_func[self->type_].sym->name;
    if (!self->prop_id_) {
        Section* sec = self->pyseg_->pysec_->sec_;
        if (!sec || !sec->prop) {
            s = "<mechanism of deleted section>";  // legacy message
        } else {
            s = "<segment invalid or or mechanism uninserted>";
        }
    }
    PyObject* result = PyString_FromString(s.c_str());
    return result;
}

static PyObject* NPyMechFunc_name(NPyMechFunc* self) {
    PyObject* result = NULL;
    std::string s = memb_func[self->pymech_->type_].sym->name;
    s += ".";
    s += self->f_->name;
    result = PyString_FromString(s.c_str());
    return result;
}

static PyObject* NPyMechFunc_call(NPyMechFunc* self, PyObject* args) {
    CHECK_PROP_INVALID(self->pymech_->prop_id_);
    PyObject* result = NULL;
    auto pyseg = self->pymech_->pyseg_;
    auto& f = self->f_->func;

    // patterning after fcall
    Symbol sym{};  // in case of error, need the name.
    sym.name = (char*) self->f_->name;
    std::vector<char*> strings_to_free;
    int narg = hocobj_pushargs(args, strings_to_free);
    hoc_push_frame(&sym, narg);  // get_argument uses the current frame
    try {
        double x = (f) (self->pymech_->prop_);
        result = Py_BuildValue("d", x);
    } catch (std::exception const& e) {
        std::ostringstream oss;
        oss << "mechanism.function call error: " << e.what();
        PyErr_SetString(PyExc_RuntimeError, oss.str().c_str());
    }
    hoc_pop_frame();
    hocobj_pushargs_free_strings(strings_to_free);

    return result;
}

static PyObject* NPyMechObj_is_ion(NPyMechObj* self) {
    CHECK_PROP_INVALID(self->prop_id_);
    if (nrn_is_ion(self->type_)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* NPyMechObj_segment(NPyMechObj* self) {
    CHECK_PROP_INVALID(self->prop_id_);
    PyObject* result = NULL;
    if (self->pyseg_) {
        result = (PyObject*) (self->pyseg_);
        Py_INCREF(result);
    }
    return result;
}

static PyObject* NPyMechFunc_mech(NPyMechFunc* self) {
    PyObject* result = NULL;
    if (self->pymech_) {
        CHECK_PROP_INVALID(self->pymech_->prop_id_);
        result = (PyObject*) (self->pymech_);
        Py_INCREF(result);
    }
    return result;
}

static PyObject* pymech_repr(PyObject* p) {
    NPyMechObj* pymech = (NPyMechObj*) p;
    return NPyMechObj_name(pymech);
}

static PyObject* pymechfunc_repr(PyObject* p) {
    NPyMechFunc* pyfunc = (NPyMechFunc*) p;
    return NPyMechFunc_name(pyfunc);
}

static PyObject* NPyRangeVar_name(NPyRangeVar* self) {
    PyObject* result = NULL;
    if (self->sym_) {
        if (self->isptr_) {
            char buf[256];
            Sprintf(buf, "_ref_%s", self->sym_->name);
            result = PyString_FromString(buf);
        } else {
            result = PyString_FromString(self->sym_->name);
        }
    } else {
        CHECK_SEC_INVALID(self->pymech_->pyseg_->pysec_->sec_);
        PyErr_SetString(PyExc_ReferenceError, "no Symbol");
    }
    return result;
}

static PyObject* NPyRangeVar_mech(NPyRangeVar* self) {
    CHECK_SEC_INVALID(self->pymech_->pyseg_->pysec_->sec_);
    PyObject* result = NULL;
    if (self->pymech_) {
        result = (PyObject*) self->pymech_;
        Py_INCREF(result);
    }
    return result;
}

static PyObject* NPySecObj_connect(NPySecObj* self, PyObject* args) {
    CHECK_SEC_INVALID(self->sec_);
    PyObject* p;
    NPySecObj* parent;
    double parentx, childend;
    parentx = -1000.;
    childend = 0.;
    if (!PyArg_ParseTuple(args, "O|dd", &p, &parentx, &childend)) {
        return NULL;
    }
    if (PyObject_TypeCheck(p, psection_type)) {
        parent = (NPySecObj*) p;
        if (parentx == -1000.) {
            parentx = 1.;
        }
    } else if (PyObject_TypeCheck(p, psegment_type)) {
        parent = ((NPySegObj*) p)->pysec_;
        if (parentx != -1000.) {
            childend = parentx;
        }
        parentx = ((NPySegObj*) p)->x_;
    } else {
        PyErr_SetString(PyExc_TypeError, "first arg not a nrn.Section or nrn.Segment");
        return NULL;
    }
    CHECK_SEC_INVALID(parent->sec_);

    // printf("NPySecObj_connect %s %g %g\n", parent, parentx, childend);
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
    return (PyObject*) self;
}

static PyObject* NPySecObj_insert(NPySecObj* self, PyObject* args) {
    CHECK_SEC_INVALID(self->sec_);
    char* tname;
    PyObject *tpyobj, *tpyobj2;
    if (!PyArg_ParseTuple(args, "s", &tname)) {
        PyErr_Clear();
        // if called with an object that has an insert method, use that
        if (PyArg_ParseTuple(args, "O", &tpyobj)) {
            Py_INCREF(tpyobj);
            Py_INCREF((PyObject*) self);
            tpyobj2 = PyObject_CallMethod(tpyobj, "insert", "O", (PyObject*) self);
            Py_DECREF(tpyobj);
            if (tpyobj2 == NULL) {
                Py_DECREF((PyObject*) self);
                PyErr_Clear();
                PyErr_SetString(
                    PyExc_TypeError,
                    "insert argument must be either a string or an object with an insert method");
                return NULL;
            }
            Py_DECREF(tpyobj2);
            return (PyObject*) self;
        }
        PyErr_Clear();
        PyErr_SetString(PyExc_TypeError, "insert takes a single positional argument");
        return NULL;
    }
    PyObject* otype = PyDict_GetItemString(pmech_types, tname);
    if (!otype) {
        // check first to see if the pmech_types needs to be
        // augmented by any new KSChan
        remake_pmech_types();
        otype = PyDict_GetItemString(pmech_types, tname);
        if (!otype) {
            PyErr_SetString(PyExc_ValueError, "argument not a density mechanism name.");
            return NULL;
        }
    }
    int type = PyInt_AsLong(otype);
    // printf("NPySecObj_insert %s %d\n", tname, type);
    mech_insert1(self->sec_, type);
    Py_INCREF(self);
    return (PyObject*) self;
}

static PyObject* NPySecObj_uninsert(NPySecObj* self, PyObject* args) {
    CHECK_SEC_INVALID(self->sec_);
    char* tname;
    if (!PyArg_ParseTuple(args, "s", &tname)) {
        return NULL;
    }
    PyObject* otype = PyDict_GetItemString(pmech_types, tname);
    if (!otype) {
        // check first to see if the pmech_types needs to be
        // augmented by any new KSChan
        remake_pmech_types();
        otype = PyDict_GetItemString(pmech_types, tname);
        if (!otype) {
            PyErr_SetString(PyExc_ValueError, "argument not a density mechanism name.");
            return NULL;
        }
    }
    int type = PyInt_AsLong(otype);
    // printf("NPySecObj_uninsert %s %d\n", tname, memb_func[type].sym);
    mech_uninsert1(self->sec_, memb_func[type].sym);
    Py_INCREF(self);
    return (PyObject*) self;
}

static PyObject* NPySecObj_has_membrane(NPySecObj* self, PyObject* args) {
    CHECK_SEC_INVALID(self->sec_);
    char* mechanism_name;
    PyObject* result;
    if (!PyArg_ParseTuple(args, "s", &mechanism_name)) {
        return NULL;
    }
    result = has_membrane(mechanism_name, self->sec_) ? Py_True : Py_False;
    Py_XINCREF(result);
    return result;
}

PyObject* nrnpy_pushsec(PyObject* sec) {
    if (PyObject_TypeCheck(sec, psection_type)) {
        nrn_pushsec(((NPySecObj*) sec)->sec_);
        return sec;
    }
    return NULL;
}

static PyObject* NPySecObj_push(NPySecObj* self, PyObject* args) {
    CHECK_SEC_INVALID(self->sec_);
    nrn_pushsec(self->sec_);
    Py_INCREF(self);
    return (PyObject*) self;
}

static PyObject* seg_of_section_iter(NPySecObj* self) {  // iterates over segments
    CHECK_SEC_INVALID(self->sec_);
    // printf("section_iter\n");
    NPySegOfSecIter* segiter;
    segiter = PyObject_New(NPySegOfSecIter, pseg_of_sec_iter_type);
    if (segiter == NULL) {
        return NULL;
    }

    segiter->seg_iter_ = 0;
    Py_INCREF(self);
    segiter->pysec_ = self;
    return (PyObject*) segiter;
}

static PyObject* allseg(NPySecObj* self) {
    CHECK_SEC_INVALID(self->sec_);
    // printf("allseg\n");
    NPyAllSegOfSecIter* ai = PyObject_New(NPyAllSegOfSecIter, pallseg_of_sec_iter_type);
    ai->pysec_ = self;
    Py_INCREF(self);
    ai->allseg_iter_ = -1;
    return (PyObject*) ai;
}

static PyObject* allseg_of_sec_iter(NPyAllSegOfSecIter* self) {
    Py_INCREF(self);
    self->allseg_iter_ = -1;
    return (PyObject*) self;
}

static PyObject* allseg_of_sec_next(NPyAllSegOfSecIter* self) {
    NPySegObj* seg;
    int n1 = self->pysec_->sec_->nnode - 1;
    if (self->allseg_iter_ > n1) {
        // end of iteration
        return NULL;
    }
    seg = PyObject_New(NPySegObj, psegment_type);
    if (seg == NULL) {
        // error
        return NULL;
    }
    seg->pysec_ = self->pysec_;
    Py_INCREF(self->pysec_);
    if (self->allseg_iter_ == -1) {
        seg->x_ = 0.;
    } else if (self->allseg_iter_ == n1) {
        seg->x_ = 1.;
    } else {
        seg->x_ = (double(self->allseg_iter_) + 0.5) / ((double) n1);
    }
    ++self->allseg_iter_;
    return (PyObject*) seg;
}

static PyObject* seg_of_sec_next(NPySegOfSecIter* self) {
    NPySegObj* seg;
    int n1 = self->pysec_->sec_->nnode - 1;
    if (self->seg_iter_ >= n1) {
        // end of iteration
        return NULL;
    }
    seg = PyObject_New(NPySegObj, psegment_type);
    if (seg == NULL) {
        // error
        return NULL;
    }
    seg->pysec_ = self->pysec_;
    Py_INCREF(self->pysec_);
    seg->x_ = (double(self->seg_iter_) + 0.5) / ((double) n1);
    ++self->seg_iter_;
    return (PyObject*) seg;
}

static PyObject* seg_point_processes(NPySegObj* self) {
    Section* sec = self->pysec_->sec_;
    CHECK_SEC_INVALID(sec);
    Node* nd = node_exact(sec, self->x_);
    PyObject* result = PyList_New(0);
    for (Prop* p = nd->prop; p; p = p->next) {
        if (memb_func[p->_type].is_point) {
            auto* pp = p->dparam[1].get<Point_process*>();
            PyObject* item = nrnpy_ho2po(pp->ob);
            int err = PyList_Append(result, item);
            assert(err == 0);
            Py_XDECREF(item);
        }
    }
    return result;
}

static PyObject* node_index1(NPySegObj* self) {
    Section* sec = self->pysec_->sec_;
    CHECK_SEC_INVALID(sec);
    Node* nd = node_exact(sec, self->x_);
    PyObject* result = Py_BuildValue("i", nd->v_node_index);
    return result;
}

static PyObject* seg_area(NPySegObj* self) {
    Section* sec = self->pysec_->sec_;
    CHECK_SEC_INVALID(sec);
    if (sec->recalc_area_) {
        nrn_area_ri(sec);
    }
    double x = self->x_;
    double a = 0.0;
    if (x > 0. && x < 1.) {
        Node* nd = node_exact(sec, x);
        a = NODEAREA(nd);
    }
    PyObject* result = Py_BuildValue("d", a);
    return result;
}

static inline double scaled_frustum_volume(double length, double d0, double d1) {
    // this is proportional to the volume of a frustum... need to multiply by pi/12
    return length * (d0 * d0 + d0 * d1 + d1 * d1);
}

static inline double interpolate(double x0, double x1, double y0, double y1, double xnew) {
    // computes ynew on the line between (x0, y0) and (x1, y1)
    // recall: y - y0 = m (x - x0)
    if (x1 == x0) {
        // avoid dividing by 0 if no length...
        return y0;
    } else {
        return y0 + (y1 - y0) * (xnew - x0) / (x1 - x0);
    }
}

static int arg_bisect_arc3d(Section* sec, int npt3d, double x) {
    // returns the index in sec where the arc is no more than x, but where the next points arc is
    // more right endpoint is never included
    int left = 0;
    int right = npt3d;
    while (right - left > 1) {
        int mid = (left + right) / 2;
        if (sec->pt3d[mid].arc < x) {
            left = mid;
        } else {
            right = mid;
        }
    }
    return left;
}

static PyObject* seg_volume(NPySegObj* self) {
    Section* sec = self->pysec_->sec_;
    CHECK_SEC_INVALID(sec);
    int i;
    if (sec->recalc_area_) {
        nrn_area_ri(sec);
    }
    double x = self->x_;
    double a = 0.0;
    // volume only exists on the interior
    if (x > 0. && x < 1.) {
        // every section owns one extra node (the conservation node)
        int nseg = sec->nnode - 1;
        double length = section_length(sec) / nseg;
        int iseg = x * nseg;
        double seg_left_arc = iseg * length;
        double seg_right_arc = (iseg + 1) * length;
        if (sec->npt3d > 1) {
            int i_left = arg_bisect_arc3d(sec, sec->npt3d, seg_left_arc);
            double left_diam = fabs(sec->pt3d[i_left].d);
            double right_diam = fabs(sec->pt3d[i_left + 1].d);
            double left_arc = seg_left_arc;
            left_diam = interpolate(
                sec->pt3d[i_left].arc, sec->pt3d[i_left + 1].arc, left_diam, right_diam, left_arc);

            for (i = i_left + 1; i < sec->npt3d && sec->pt3d[i].arc < seg_right_arc; i++) {
                right_diam = fabs(sec->pt3d[i].d);
                a += scaled_frustum_volume(sec->pt3d[i].arc - left_arc, left_diam, right_diam);
                left_arc = sec->pt3d[i].arc;
                left_diam = right_diam;
            }
            if (i < sec->npt3d) {
                right_diam = interpolate(
                    left_arc, sec->pt3d[i].arc, left_diam, fabs(sec->pt3d[i].d), seg_right_arc);
                a += scaled_frustum_volume(seg_right_arc - left_arc, left_diam, right_diam);
            }

            // the scaled_frustum_volume formula factored out a pi/12
            a *= M_PI / 12;
        } else {
            // 0 or 1 3D points... so give cylinder volume
            Node* nd = node_exact(sec, x);
            for (Prop* p = nd->prop; p; p = p->next) {
                if (p->_type == MORPHOLOGY) {
                    double diam = p->param(0);
                    a = M_PI * diam * diam / 4 * length;
                    break;
                }
            }
        }
    }
    PyObject* result = Py_BuildValue("d", a);
    return result;
}


static PyObject* seg_ri(NPySegObj* self) {
    Section* sec = self->pysec_->sec_;
    CHECK_SEC_INVALID(sec);
    if (sec->recalc_area_) {
        nrn_area_ri(sec);
    }
    double ri = 1e30;
    Node* nd = node_exact(sec, self->x_);
    if (NODERINV(nd)) {
        ri = 1. / NODERINV(nd);
    }
    PyObject* result = Py_BuildValue("d", ri);
    return result;
}

static Prop* mech_of_segment_prop(Prop* p) {
    // p must be in the pmech_types list. But I no longer know if that
    // is ever not the case.
    for (;;) {
        if (!p) {
            break;
        }
        // printf("segment_iter %d %s\n", p->_type, memb_func[p->_type].sym->name);
        // Only return density mechanisms (skip POINT_PROCESS)
        if (PyDict_GetItemString(pmech_types, memb_func[p->_type].sym->name)) {
            // printf("segment_iter found\n");
            break;
        }
        p = p->next;
    }
    return p;
}

static PyObject* mech_of_segment_iter(NPySegObj* self) {
    Section* sec = self->pysec_->sec_;
    if (!sec->prop) {
        PyErr_SetString(PyExc_ReferenceError, "nrn.Segment can't access a deleted section");
        return NULL;
    }
    // printf("mech_of_segment_iter\n");
    Node* nd = node_exact(sec, self->x_);
    Prop* p = mech_of_segment_prop(nd->prop);
    NPyMechOfSegIter* mi = PyObject_New(NPyMechOfSegIter, pmech_of_seg_iter_generic_type);
    if (!mi) {
        return NULL;
    }
    NPyMechObj* m = new_pymechobj(self, p);
    if (!m) {
        Py_XDECREF(mi);
        return NULL;
    }
    mi->pymech_ = m;
    return (PyObject*) mi;
}

static Object* seg_from_sec_x(Section* sec, double x) {
    PyObject* pyseg = (PyObject*) PyObject_New(NPySegObj, psegment_type);
    NPySegObj* pseg = (NPySegObj*) pyseg;
    auto* pysec = static_cast<NPySecObj*>(sec->prop->dparam[PROP_PY_INDEX].get<void*>());
    if (pysec) {
        pseg->pysec_ = pysec;
        Py_INCREF(pysec);
    } else {
        pysec = (NPySecObj*) psection_type->tp_alloc(psection_type, 0);
        pysec->sec_ = sec;
        pysec->name_ = 0;
        pysec->cell_weakref_ = 0;
        Py_INCREF(pysec);
        pseg->pysec_ = pysec;
    }
    pseg->x_ = x;
    Object* ho = nrnpy_pyobject_in_obj(pyseg);
    Py_DECREF(pyseg);
    return ho;
}

static Object** pp_get_segment(void* vptr) {
    Point_process* pnt = (Point_process*) vptr;
    // printf("pp_get_segment %s\n", hoc_object_name(pnt->ob));
    Object* ho = NULL;
    if (pnt->prop) {
        Section* sec = pnt->sec;
        double x = nrn_arc_position(sec, pnt->node);
        ho = seg_from_sec_x(sec, x);
    }
    if (!ho) {
        ho = nrnpy_pyobject_in_obj(Py_None);
    }
    Object** tobj = hoc_temp_objptr(ho);
    --ho->refcount;
    return tobj;
}

static void rv_noexist(Section* sec, const char* n, double x, int err) {
    char buf[200];
    if (err == 2) {
        Sprintf(buf, "%s was not made to point to anything at %s(%g)", n, secname(sec), x);
    } else if (err == 1) {
        Sprintf(buf, "%s, the mechanism does not exist at %s(%g)", n, secname(sec), x);
    } else {
        Sprintf(buf, "%s does not exist at %s(%g)", n, secname(sec), x);
    }
    PyErr_SetString(PyExc_AttributeError, buf);
}

static NPyRangeVar* rvnew(Symbol* sym, NPySecObj* sec, double x) {
    NPyRangeVar* r = PyObject_New(NPyRangeVar, range_type);
    if (!r) {
        return NULL;
    }
    r->pymech_ = new_pymechobj();
    r->pymech_->pyseg_ = PyObject_New(NPySegObj, psegment_type);
    r->pymech_->pyseg_->pysec_ = sec;
    Py_INCREF(sec);
    r->pymech_->pyseg_->x_ = 0.5;
    r->sym_ = sym;
    r->isptr_ = 0;
    r->attr_from_sec_ = 1;
    return r;
}

static PyObject* section_getattro(NPySecObj* self, PyObject* pyname) {
    Section* sec = self->sec_;
    CHECK_SEC_INVALID(sec);
    PyObject* rv;
    Py_INCREF(pyname);
    Py2NRNString name(pyname);
    char* n = name.c_str();
    if (name.err()) {
        name.set_pyerr(PyExc_TypeError, "attribute name must be a string");
        Py_DECREF(pyname);
        return NULL;
    }
    // printf("section_getattr %s\n", n);
    PyObject* result = 0;
    if (strcmp(n, "L") == 0) {
        result = Py_BuildValue("d", section_length(sec));
    } else if (strcmp(n, "Ra") == 0) {
        result = Py_BuildValue("d", nrn_ra(sec));
    } else if (strcmp(n, "nseg") == 0) {
        result = Py_BuildValue("i", sec->nnode - 1);
    } else if ((rv = PyDict_GetItemString(rangevars_, n)) != NULL) {
        Symbol* sym = ((NPyRangeVar*) rv)->sym_;
        if (ISARRAY(sym)) {
            NPyRangeVar* r = rvnew(sym, self, 0.5);
            result = (PyObject*) r;
        } else {
            int err;
            auto const d = nrnpy_rangepointer(sec, sym, 0.5, &err, 0 /* idx */);
            if (!d) {
                rv_noexist(sec, n, 0.5, err);
                result = nullptr;
            } else {
                if (sec->recalc_area_ && sym->u.rng.type == MORPHOLOGY) {
                    nrn_area_ri(sec);
                }
                result = Py_BuildValue("d", *d);
            }
        }
    } else if (strcmp(n, "rallbranch") == 0) {
        result = Py_BuildValue("d", sec->prop->dparam[4].get<double>());
    } else if (strcmp(n, "__dict__") == 0) {
        result = PyDict_New();
        int err = PyDict_SetItemString(result, "L", Py_None);
        assert(err == 0);
        err = PyDict_SetItemString(result, "Ra", Py_None);
        assert(err == 0);
        err = PyDict_SetItemString(result, "nseg", Py_None);
        assert(err == 0);
        err = PyDict_SetItemString(result, "rallbranch", Py_None);
        assert(err == 0);
    } else {
        result = PyObject_GenericGetAttr((PyObject*) self, pyname);
    }
    Py_DECREF(pyname);
    return result;
}

static int section_setattro(NPySecObj* self, PyObject* pyname, PyObject* value) {
    Section* sec = self->sec_;
    if (!sec->prop) {
        PyErr_SetString(PyExc_ReferenceError, "can't access a deleted section");
        return -1;
    }
    PyObject* rv;
    int err = 0;
    Py_INCREF(pyname);
    Py2NRNString name(pyname);
    char* n = name.c_str();
    if (name.err()) {
        name.set_pyerr(PyExc_TypeError, "attribute name must be a string");
        Py_DECREF(pyname);
        return -1;
    }
    // printf("section_setattro %s\n", n);
    if (strcmp(n, "L") == 0) {
        double x;
        if (PyArg_Parse(value, "d", &x) == 1 && x > 0.) {
            if (can_change_morph(sec)) {
                sec->prop->dparam[2] = x;
                nrn_length_change(sec, x);
                diam_changed = 1;
                sec->recalc_area_ = 1;
            }
        } else {
            PyErr_SetString(PyExc_ValueError, "L must be > 0.");
            err = -1;
        }
    } else if (strcmp(n, "Ra") == 0) {
        double x;
        if (PyArg_Parse(value, "d", &x) == 1 && x > 0.) {
            sec->prop->dparam[7] = x;
            diam_changed = 1;
            sec->recalc_area_ = 1;
        } else {
            PyErr_SetString(PyExc_ValueError, "Ra must be > 0.");
            err = -1;
        }
    } else if (strcmp(n, "nseg") == 0) {
        int nseg;
        if (PyArg_Parse(value, "i", &nseg) == 1 && nseg > 0 && nseg <= 32767) {
            nrn_change_nseg(sec, nseg);
        } else {
            PyErr_SetString(PyExc_ValueError, "nseg must be an integer in range 1 to 32767");
            err = -1;
        }
        // printf("section_setattro err=%d nseg=%d nnode\n", err, nseg,
        // sec->nnode);
    } else if ((rv = PyDict_GetItemString(rangevars_, n)) != NULL) {
        Symbol* sym = ((NPyRangeVar*) rv)->sym_;
        if (ISARRAY(sym)) {
            PyErr_SetString(PyExc_IndexError, "missing index");
            err = -1;
        } else {
            int errp;
            auto const d = nrnpy_rangepointer(sec, sym, 0.5, &errp, 0 /* idx */);
            if (!d) {
                rv_noexist(sec, n, 0.5, errp);
                err = -1;
            } else if (!PyArg_Parse(value, "d", static_cast<double const*>(d))) {
                PyErr_SetString(PyExc_ValueError, "bad value");
                err = -1;
            } else {
                // only need to do following if nseg > 1, VINDEX, or EXTRACELL
                nrn_rangeconst(sec, sym, d, 0);
            }
        }
    } else if (strcmp(n, "rallbranch") == 0) {
        double x;
        if (PyArg_Parse(value, "d", &x) == 1 && x > 0.) {
            sec->prop->dparam[4] = x;
            diam_changed = 1;
            sec->recalc_area_ = 1;
        } else {
            PyErr_SetString(PyExc_ValueError, "rallbranch must be > 0");
            err = -1;
        }
    } else {
        err = PyObject_GenericSetAttr((PyObject*) self, pyname, value);
    }
    Py_DECREF(pyname);
    return err;
}

static PyObject* mech_of_seg_next(NPyMechOfSegIter* self) {
    // printf("mech_of_seg_next\n");
    // The return on this iteration is self->pymech_. NULL means it's over.
    NPyMechObj* m = self->pymech_;
    if (!m) {
        return NULL;
    }
    if (!m->prop_id_) {
        PyErr_SetString(PyExc_ReferenceError,
                        "mechanism instance became invalid in middle of the mechanism iterator");
        return NULL;
    }
    Prop* pnext = mech_of_segment_prop(m->prop_->next);
    NPyMechObj* mnext{};
    if (pnext) {
        mnext = new_pymechobj(m->pyseg_, pnext);
    }
    self->pymech_ = mnext;
    return (PyObject*) m;
}

static PyObject* var_of_mech_iter(NPyMechObj* self) {
    Section* sec = self->pyseg_->pysec_->sec_;
    if (!sec->prop) {
        PyErr_SetString(PyExc_ReferenceError, "nrn.Mechanism can't access a deleted section");
        return NULL;
    }

    // printf("var_of_mech_iter\n");
    NPyVarOfMechIter* vmi = PyObject_New(NPyVarOfMechIter, pvar_of_mech_iter_generic_type);
    if (!self->prop_) {
        return NULL;
    }
    vmi->pymech_ = self;
    Py_INCREF(vmi->pymech_);
    vmi->msym_ = memb_func[self->prop_->_type].sym;
    vmi->i_ = 0;
    return (PyObject*) vmi;
}

static PyObject* var_of_mech_next(NPyVarOfMechIter* self) {
    if (self->i_ >= self->msym_->s_varn) {
        return NULL;
    }
    // printf("var_of_mech_next %d %s\n", self->i_, self->msym_->name);
    Symbol* sym = self->msym_->u.ppsym[self->i_];
    self->i_++;
    NPyRangeVar* r = (NPyRangeVar*) PyObject_New(NPyRangeVar, range_type);
    r->pymech_ = self->pymech_;
    Py_INCREF(r->pymech_);
    r->sym_ = sym;
    r->isptr_ = 0;
    r->attr_from_sec_ = 0;
    return (PyObject*) r;
}

static PyObject* segment_getattro(NPySegObj* self, PyObject* pyname) {
    Section* sec = self->pysec_->sec_;
    if (!sec->prop) {
        PyErr_SetString(PyExc_ReferenceError, "nrn.Segment can't access a deleted section");
        return NULL;
    }
    Symbol* sym;
    Py_INCREF(pyname);
    Py2NRNString name(pyname);
    char* n = name.c_str();
    if (name.err()) {
        name.set_pyerr(PyExc_TypeError, "attribute name must be a string");
        Py_DECREF(pyname);
        return NULL;
    }
    // printf("segment_getattr %s\n", n);
    PyObject* result = NULL;
    PyObject* otype = NULL;
    PyObject* rv = NULL;
    if (strcmp(n, "v") == 0) {
        Node* nd = node_exact(sec, self->x_);
        result = Py_BuildValue("d", NODEV(nd));
    } else if ((otype = PyDict_GetItemString(pmech_types, n)) != NULL) {
        int type = PyInt_AsLong(otype);
        // printf("segment_getattr type=%d\n", type);
        Node* nd = node_exact(sec, self->x_);
        Prop* p = nrn_mechanism(type, nd);
        if (!p) {
            rv_noexist(sec, n, self->x_, 1);
            result = NULL;
        } else {
            result = (PyObject*) new_pymechobj(self, p);
        }
    } else if ((rv = PyDict_GetItemString(rangevars_, n)) != NULL) {
        sym = ((NPyRangeVar*) rv)->sym_;
        if (sym->type == RANGEOBJ) {
            int mtype = sym->u.rng.type;
            Node* nd = node_exact(sec, self->x_);
            Prop* p = nrn_mechanism(mtype, nd);
            Object* ob = nrn_nmodlrandom_wrap(p, sym);
            result = nrnpy_ho2po(ob);
        } else if (ISARRAY(sym)) {
            NPyRangeVar* r = PyObject_New(NPyRangeVar, range_type);
            r->pymech_ = new_pymechobj();
            r->pymech_->pyseg_ = self;
            Py_INCREF(r->pymech_->pyseg_);
            r->sym_ = sym;
            r->isptr_ = 0;
            r->attr_from_sec_ = 0;
            result = (PyObject*) r;
        } else {
            int err;
            auto const d = nrnpy_rangepointer(sec, sym, self->x_, &err, 0 /* idx */);
            if (!d) {
                rv_noexist(sec, n, self->x_, err);
                result = NULL;
            } else {
                if (sec->recalc_area_ && sym->u.rng.type == MORPHOLOGY) {
                    nrn_area_ri(sec);
                }
                result = Py_BuildValue("d", *d);
            }
        }
    } else if (strncmp(n, "_ref_", 5) == 0) {
        if (strcmp(n + 5, "v") == 0) {
            Node* nd = node_exact(sec, self->x_);
            result = nrn_hocobj_handle(nd->v_handle());
        } else if ((sym = hoc_table_lookup(n + 5, hoc_built_in_symlist)) != 0 &&
                   sym->type == RANGEVAR) {
            if (ISARRAY(sym)) {
                NPyRangeVar* r = PyObject_New(NPyRangeVar, range_type);
                r->pymech_ = new_pymechobj();
                r->pymech_->pyseg_ = self;
                Py_INCREF(self);
                r->sym_ = sym;
                r->isptr_ = 1;
                r->attr_from_sec_ = 0;
                result = (PyObject*) r;
            } else {
                int err;
                auto const d = nrnpy_rangepointer(sec, sym, self->x_, &err, 0 /* idx */);
                if (!d) {
                    rv_noexist(sec, n + 5, self->x_, err);
                    result = NULL;
                } else {
                    result = nrn_hocobj_handle(d);
                }
            }
        } else {
            rv_noexist(sec, n, self->x_, 2);
            result = NULL;
        }
    } else if (strcmp(n, "__dict__") == 0) {
        Node* nd = node_exact(sec, self->x_);
        result = PyDict_New();
        int err = PyDict_SetItemString(result, "v", Py_None);
        assert(err == 0);
        PyDict_SetItemString(result, "diam", Py_None);
        assert(err == 0);
        PyDict_SetItemString(result, "cm", Py_None);
        assert(err == 0);
        for (Prop* p = nd->prop; p; p = p->next) {
            if (p->_type > CAP && !memb_func[p->_type].is_point) {
                char* pn = memb_func[p->_type].sym->name;
                err = PyDict_SetItemString(result, pn, Py_None);
                assert(err == 0);
            }
        }
    } else {
        result = PyObject_GenericGetAttr((PyObject*) self, pyname);
    }
    Py_DECREF(pyname);
    return result;
}

int nrn_pointer_assign(Prop* prop, Symbol* sym, PyObject* value) {
    int err = 0;
    if (sym->subtype == NRNPOINTER) {
        if (neuron::container::data_handle<double> dh{}; nrn_is_hocobj_ptr(value, dh)) {
            // The challenge is that we need to store a data handle here,
            // because POINTER variables are set up before the data are
            // permuted, but that handle then gets read as part of the
            // translated mechanism code, inside the translated MOD files, where
            // we might not otherwise like to pay the extra cost of indirection.
            prop->dparam[sym->u.rng.index] = std::move(dh);
        } else {
            PyErr_SetString(PyExc_ValueError, "must be a hoc pointer");
            err = -1;
        }
    } else {
        PyErr_SetString(PyExc_AttributeError,
                        " For assignment, only POINTER var can have a _ref_ prefix");
        err = -1;
    }
    return err;
}

static int segment_setattro(NPySegObj* self, PyObject* pyname, PyObject* value) {
    Section* sec = self->pysec_->sec_;
    if (!sec->prop) {
        PyErr_SetString(PyExc_ReferenceError, "nrn.Segment can't access a deleted section");
        return -1;
    }
    PyObject* rv;
    Symbol* sym;
    int err = 0;
    Py_INCREF(pyname);
    Py2NRNString name(pyname);
    char* n = name.c_str();
    if (name.err()) {
        name.set_pyerr(PyExc_TypeError, "attribute name must be a string");
        Py_DECREF(pyname);
        return -1;
    }
    // printf("segment_setattro %s\n", n);
    if (strcmp(n, "x") == 0) {
        int nseg;
        double x;
        if (PyArg_Parse(value, "d", &x) == 1 && x > 0. && x <= 1.) {
            if (x < 1e-9) {
                self->x_ = 0.;
            } else if (x > 1. - 1e-9) {
                self->x_ = 1.;
            } else {
                self->x_ = x;
            }
        } else {
            PyErr_SetString(PyExc_ValueError, "x must be in range 0. to 1.");
            err = -1;
        }
    } else if ((rv = PyDict_GetItemString(rangevars_, n)) != NULL) {
        sym = ((NPyRangeVar*) rv)->sym_;
        if (ISARRAY(sym)) {
            char s[200];
            Sprintf(s, "%s needs an index for assignment", sym->name);
            PyErr_SetString(PyExc_IndexError, s);
            err = -1;
        } else {
            int errp;
            auto const d = nrnpy_rangepointer(sec, sym, self->x_, &errp, 0 /* idx */);
            if (!d) {
                rv_noexist(sec, n, self->x_, errp);
                Py_DECREF(pyname);
                return -1;
            }
            if (!PyArg_Parse(value, "d", static_cast<double const*>(d))) {
                PyErr_SetString(PyExc_ValueError, "bad value");
                Py_DECREF(pyname);
                return -1;
            } else if (sym->u.rng.type == MORPHOLOGY) {
                diam_changed = 1;
                sec->recalc_area_ = 1;
                nrn_diam_change(sec);
            } else if (sym->u.rng.type == EXTRACELL && sym->u.rng.index == 0) {
                // cannot execute because xraxial is an array
                diam_changed = 1;
            }
        }
    } else if (strncmp(n, "_ref_", 5) == 0) {
        Symbol* rvsym = hoc_table_lookup(n + 5, hoc_built_in_symlist);
        if (rvsym && rvsym->type == RANGEVAR) {
            Node* nd = node_exact(sec, self->x_);
            assert(nd);
            Prop* prop = nrn_mechanism(rvsym->u.rng.type, nd);
            assert(prop);
            err = nrn_pointer_assign(prop, rvsym, value);
        } else {
            err = PyObject_GenericSetAttr((PyObject*) self, pyname, value);
        }
    } else {
        err = PyObject_GenericSetAttr((PyObject*) self, pyname, value);
    }
    Py_DECREF(pyname);
    return err;
}

static bool striptrail(char* buf, int sz, const char* n, const char* m) {
    int nlen = strlen(n);
    int mlen = strlen(m);
    int u = nlen - mlen - 1;     // should be location of _
    if (u > 0 && n[u] == '_') {  // likely n is name_m
        if (strcmp(n + (u + 1), m) != 0) {
            return false;
        }
        strncpy(buf, n, sz);
        buf[u] = '\0';
        return true;
    }
    return false;
}

static PyObject* mech_getattro(NPyMechObj* self, PyObject* pyname) {
    Section* sec = self->pyseg_->pysec_->sec_;
    if (!sec->prop) {
        PyErr_SetString(PyExc_ReferenceError, "nrn.Mechanism can't access a deleted section");
        return NULL;
    }
    CHECK_PROP_INVALID(self->prop_id_);
    Py_INCREF(pyname);
    Py2NRNString name(pyname);
    char* n = name.c_str();
    if (!n) {
        name.set_pyerr(PyExc_TypeError, "attribute name must be a string");
        Py_DECREF(pyname);
        return NULL;
    }
    // printf("mech_getattro %s\n", n);
    PyObject* result = NULL;
    NrnProperty np(self->prop_);
    int isptr = (strncmp(n, "_ref_", 5) == 0);
    char* mname = memb_func[self->prop_->_type].sym->name;
    int mnamelen = strlen(mname);
    int bufsz = strlen(n) + mnamelen + 2;
    char* buf = new char[bufsz];
    if (nrn_is_ion(self->prop_->_type)) {
        strcpy(buf, isptr ? n + 5 : n);
    } else {
        std::snprintf(buf, bufsz, "%s_%s", isptr ? n + 5 : n, mname);
    }
    Symbol* sym = np.find(buf);
    if (sym && sym->type == RANGEVAR) {
        // printf("mech_getattro sym %s\n", sym->name);
        if (ISARRAY(sym)) {
            NPyRangeVar* r = PyObject_New(NPyRangeVar, range_type);
            r->pymech_ = self;
            Py_INCREF(self);
            r->sym_ = sym;
            r->isptr_ = isptr;
            r->attr_from_sec_ = 0;
            result = (PyObject*) r;
        } else {
            auto const px = np.prop_pval(sym, 0);
            if (!px) {
                rv_noexist(sec, sym->name, self->pyseg_->x_, 2);
            } else if (isptr) {
                result = nrn_hocobj_handle(px);
            } else {
                result = Py_BuildValue("d", *px);
            }
        }
    } else if (sym && sym->type == RANGEOBJ) {
        Object* ob = nrn_nmodlrandom_wrap(self->prop_, sym);
        result = nrnpy_ho2po(ob);
    } else if (strcmp(n, "__dict__") == 0) {
        result = PyDict_New();
        for (Symbol* s = np.first_var(); np.more_var(); s = np.next_var()) {
            if (!striptrail(buf, bufsz, s->name, mname)) {
                strcpy(buf, s->name);
            }
            int err = PyDict_SetItemString(result, buf, Py_None);
            assert(err == 0);
        }
        // FUNCTION and PROCEDURE
        for (auto& it: nrn_mech2funcs_map[self->prop_->_type]) {
            int err = PyDict_SetItemString(result, it.first.c_str(), Py_None);
            assert(err == 0);
        }
    } else {
        bool found_func{false};
        if (self->prop_) {
            auto& funcs = nrn_mech2funcs_map[self->prop_->_type];
            if (funcs.count(n)) {
                found_func = true;
                auto& f = funcs[n];
                NPyMechFunc* pymf = PyObject_New(NPyMechFunc, pmechfunc_generic_type);
                pymf->pymech_ = self;
                Py_INCREF(self);
                pymf->f_ = f;
                result = (PyObject*) pymf;
            }
        }
        if (!found_func) {
            result = PyObject_GenericGetAttr((PyObject*) self, pyname);
        }
    }
    Py_DECREF(pyname);
    delete[] buf;
    return result;
}

static int mech_setattro(NPyMechObj* self, PyObject* pyname, PyObject* value) {
    Section* sec = self->pyseg_->pysec_->sec_;
    if (!sec->prop) {
        PyErr_SetString(PyExc_ReferenceError, "nrn.Mechanism can't access a deleted section");
        return -1;
    }

    int err = 0;
    Py_INCREF(pyname);
    Py2NRNString name(pyname);
    char* n = name.c_str();
    if (name.err()) {
        name.set_pyerr(PyExc_TypeError, "attribute name must be a string");
        Py_DECREF(pyname);
        return -1;
    }
    // printf("mech_setattro %s\n", n);
    NrnProperty np(self->prop_);
    int isptr = (strncmp(n, "_ref_", 5) == 0);
    char* mname = memb_func[self->prop_->_type].sym->name;
    int mnamelen = strlen(mname);
    int bufsz = strlen(n) + mnamelen + 2;
    char* buf = new char[bufsz];
    if (nrn_is_ion(self->prop_->_type)) {
        strcpy(buf, isptr ? n + 5 : n);
    } else {
        std::snprintf(buf, bufsz, "%s_%s", isptr ? n + 5 : n, mname);
    }
    Symbol* sym = np.find(buf);
    delete[] buf;
    if (sym) {
        if (isptr) {
            err = nrn_pointer_assign(self->prop_, sym, value);
        } else {
            double x;
            auto pd = np.prop_pval(sym, 0);
            if (pd) {
                if (PyArg_Parse(value, "d", &x) == 1) {
                    *pd = x;
                } else {
                    PyErr_SetString(PyExc_ValueError, "must be a double");
                    err = -1;
                }
            } else {
                rv_noexist(sec, sym->name, self->pyseg_->x_, 2);
                err = 1;
            }
        }
    } else {
        err = PyObject_GenericSetAttr((PyObject*) self, pyname, value);
    }
    Py_DECREF(pyname);
    return err;
}

neuron::container::generic_data_handle* nrnpy_setpointer_helper(PyObject* pyname, PyObject* mech) {
    if (PyObject_TypeCheck(mech, pmech_generic_type) == 0) {
        return nullptr;
    }
    NPyMechObj* m = (NPyMechObj*) mech;
    NrnProperty np(m->prop_);
    char buf[200];
    Py2NRNString name(pyname);
    char* n = name.c_str();
    if (!n) {
        return nullptr;
    }
    Sprintf(buf, "%s_%s", n, memb_func[m->prop_->_type].sym->name);
    Symbol* sym = np.find(buf);
    if (!sym || sym->type != RANGEVAR || sym->subtype != NRNPOINTER) {
        return nullptr;
    }
    return &(m->prop_->dparam[np.prop_index(sym)]);
}

static PyObject* NPySecObj_call(NPySecObj* self, PyObject* args) {
    CHECK_SEC_INVALID(self->sec_);
    double x = 0.5;
    PyArg_ParseTuple(args, "|d", &x);
    PyObject* segargs = Py_BuildValue("(O,d)", self, x);
    PyObject* seg = NPySegObj_new(psegment_type, segargs, 0);
    Py_DECREF(segargs);
    return seg;
}

static Py_ssize_t rv_len(PyObject* self) {
    NPyRangeVar* r = (NPyRangeVar*) self;
    assert(r->sym_);
    if (r->sym_->arayinfo) {
        assert(r->sym_->arayinfo->nsub == 1);
        return r->sym_->arayinfo->sub[0];
    }
    return 1;
}
static PyObject* rv_getitem(PyObject* self, Py_ssize_t ix) {
    NPyRangeVar* r = (NPyRangeVar*) self;
    Section* sec = r->pymech_->pyseg_->pysec_->sec_;
    if (!sec->prop) {
        PyErr_SetString(PyExc_ReferenceError, "nrn.RangeVar can't access a deleted section");
        return NULL;
    }

    PyObject* result = NULL;
    if (ix < 0 || ix >= rv_len(self)) {
        PyErr_SetString(PyExc_IndexError, r->sym_->name);
        return NULL;
    }
    if (ISARRAY(r->sym_)) {
        assert(r->sym_->arayinfo->nsub == 1);
        auto const array_dim = r->sym_->arayinfo->sub[0];
        assert(ix < array_dim);
        // r represents a range variable that is an array of size array_dim, and
        // we need to apply the offset `ix` to get the correct element of that
        // array.
    } else {
        // Not an array variable, so the index should always be zero
        assert(ix == 0);
    }
    int err;
    auto const d = nrnpy_rangepointer(sec, r->sym_, r->pymech_->pyseg_->x_, &err, ix);
    if (!d) {
        rv_noexist(sec, r->sym_->name, r->pymech_->pyseg_->x_, err);
        return NULL;
    }
    if (r->isptr_) {
        result = nrn_hocobj_handle(d);
    } else {
        result = Py_BuildValue("d", *d);
    }
    return result;
}
static int rv_setitem(PyObject* self, Py_ssize_t ix, PyObject* value) {
    NPyRangeVar* r = (NPyRangeVar*) self;
    Section* sec = r->pymech_->pyseg_->pysec_->sec_;
    if (!sec->prop) {
        PyErr_SetString(PyExc_ReferenceError, "nrn.RangeVar can't access a deleted section");
        return -1;
    }

    if (ix < 0 || ix >= rv_len(self)) {
        PyErr_SetString(PyExc_IndexError, r->sym_->name);
        return -1;
    }
    int err;
    auto const d = nrnpy_rangepointer(sec, r->sym_, r->pymech_->pyseg_->x_, &err, ix);
    if (!d) {
        rv_noexist(sec, r->sym_->name, r->pymech_->pyseg_->x_, err);
        return -1;
    }
    if (r->attr_from_sec_) {
        // the range variable array is from a section not a segment and so
        // assignment is over the entire section.
        double x;
        if (!PyArg_Parse(value, "d", &x)) {
            PyErr_SetString(PyExc_ValueError, "bad value");
            return -1;
        }
        hoc_pushx(double(ix));
        hoc_push_ndim(1);
        nrn_rangeconst(r->pymech_->pyseg_->pysec_->sec_,
                       r->sym_,
                       neuron::container::data_handle<double>{neuron::container::do_not_search, &x},
                       0);
    } else {
        if (!PyArg_Parse(value, "d", static_cast<double const*>(d))) {
            PyErr_SetString(PyExc_ValueError, "bad value");
            return -1;
        }
    }
    if (r->sym_->u.rng.type == EXTRACELL && r->sym_->u.rng.index == 0) {
        diam_changed = 1;
    }
    return 0;
}

static PyMethodDef NPySecObj_methods[] = {
    {"name", (PyCFunction) NPySecObj_name, METH_NOARGS, "Section name (same as hoc secname())"},
    {"hname", (PyCFunction) NPySecObj_name, METH_NOARGS, "Section name (same as hoc secname())"},
    {"has_membrane",
     (PyCFunction) NPySecObj_has_membrane,
     METH_VARARGS,
     "Returns True if the section's membrane has this density mechanism.\nThis "
     "is not for point processes."},
    {"connect",
     (PyCFunction) NPySecObj_connect,
     METH_VARARGS,
     "childSection.connect(parentSection, [parentX], [childEnd]) "
     "or\nchildSection.connect(parentSegment, [childEnd])"},
    {"insert",
     (PyCFunction) NPySecObj_insert,
     METH_VARARGS,
     "section.insert(densityMechanismName) e.g. soma.insert('hh')"},
    {"uninsert",
     (PyCFunction) NPySecObj_uninsert,
     METH_VARARGS,
     "section.uninsert(densityMechanismName) e.g. soma.insert('hh')"},
    {"push",
     (PyCFunction) NPySecObj_push,
     METH_VARARGS,
     "section.push() makes it the currently accessed section. Should end with "
     "a corresponding hoc.pop_section()"},
    {"allseg",
     (PyCFunction) allseg,
     METH_VARARGS,
     "iterate over segments. Includes x=0 and x=1 zero-area nodes in the "
     "iteration."},
    {"cell",
     (PyCFunction) pysec2cell,
     METH_NOARGS,
     "Return the object that owns the Section. Possibly None."},
    {"same",
     (PyCFunction) pysec_same,
     METH_VARARGS,
     "sec1.same(sec2) returns True if sec1 and sec2 wrap the same NEURON "
     "Section"},
    {"hoc_internal_name",
     (PyCFunction) hoc_internal_name,
     METH_NOARGS,
     "Hoc accepts this name wherever a section is syntactically valid."},
    {"disconnect",
     (PyCFunction) pysec_disconnect,
     METH_NOARGS,
     "disconnect from the parent section."},
    {"parentseg",
     (PyCFunction) pysec_parentseg,
     METH_NOARGS,
     "Return the nrn.Segment specified by the connect method. Possibly None."},
    {"trueparentseg",
     (PyCFunction) pysec_trueparentseg,
     METH_NOARGS,
     "Return the nrn.Segment this section connects to which is closer to the "
     "root. Possibly None. (same as parentseg unless parentseg.x == "
     "parentseg.sec.orientation()"},
    {"orientation",
     (PyCFunction) pysec_orientation,
     METH_NOARGS,
     "Returns 0.0 or 1.0  depending on the x value closest to parent."},
    {"children",
     (PyCFunction) pysec_children,
     METH_NOARGS,
     "Return list of child sections. Possibly an empty list"},
    {"subtree",
     (PyCFunction) pysec_subtree,
     METH_NOARGS,
     "Return list of sections in the subtree rooted at this section (including this section)."},
    {"wholetree",
     (PyCFunction) pysec_wholetree,
     METH_NOARGS,
     "Return list of all sections with a path to this section (including this section). The list "
     "has the important property that sections are in root to leaf order, depth-first traversal."},
    {"n3d", (PyCFunction) NPySecObj_n3d, METH_NOARGS, "Returns the number of 3D points."},
    {"x3d",
     (PyCFunction) NPySecObj_x3d,
     METH_VARARGS,
     "Returns the x coordinate of the ith 3D point."},
    {"y3d",
     (PyCFunction) NPySecObj_y3d,
     METH_VARARGS,
     "Returns the y coordinate of the ith 3D point."},
    {"z3d",
     (PyCFunction) NPySecObj_z3d,
     METH_VARARGS,
     "Returns the z coordinate of the ith 3D point."},
    {"arc3d",
     (PyCFunction) NPySecObj_arc3d,
     METH_VARARGS,
     "Returns the arc position of the ith 3D point."},
    {"diam3d",
     (PyCFunction) NPySecObj_diam3d,
     METH_VARARGS,
     "Returns the diam of the ith 3D point."},
    {"spine3d",
     (PyCFunction) NPySecObj_spine3d,
     METH_VARARGS,
     "Returns True or False depending on whether a spine exists at the ith 3D point."},
    {"pt3dremove", (PyCFunction) NPySecObj_pt3dremove, METH_VARARGS, "Removes the ith 3D point."},
    {"pt3dclear",
     (PyCFunction) NPySecObj_pt3dclear,
     METH_VARARGS,
     "Clears all 3D points. Optionally takes a buffer size."},
    {"pt3dinsert",
     (PyCFunction) NPySecObj_pt3dinsert,
     METH_VARARGS,
     "Insert the point (so it becomes the i'th point) to section. If i is equal to sec.n3d(), the "
     "point is appended (equivalent to sec.pt3dadd())"},
    {"pt3dchange",
     (PyCFunction) NPySecObj_pt3dchange,
     METH_VARARGS,
     "Change the i'th 3-d point info. If only two args then the second arg is the diameter and the "
     "location is unchanged."},
    {"pt3dadd",
     (PyCFunction) NPySecObj_pt3dadd,
     METH_VARARGS,
     "Add the 3d location and diameter point (or points in the second form) at the end of the "
     "current pt3d list. Assume that successive additions increase the arc length monotonically."},
    {"pt3dstyle",
     (PyCFunction) NPySecObj_pt3dstyle,
     METH_VARARGS,
     "Returns True if using a logical connection point, else False. With first arg 0 sets to no "
     "logical connection point. With first arg 1 and x, y, z arguments, sets the logical "
     "connection point. Return value includes the result of any setting."},
    {"is_pysec",
     (PyCFunction) is_pysec,
     METH_NOARGS,
     "Returns True if Section created from Python, False if created from HOC."},
    {"psection",
     (PyCFunction) NPySecObj_psection,
     METH_NOARGS,
     "Returns dict of info about Section contents."},
    {NULL}};

static PyMethodDef NPySegObj_methods[] = {
    {"point_processes",
     (PyCFunction) seg_point_processes,
     METH_NOARGS,
     "seg.point_processes() returns list of POINT_PROCESS instances in the "
     "segment."},
    {"node_index",
     (PyCFunction) node_index1,
     METH_NOARGS,
     "seg.node_index() returns index of v, rhs, etc. in the _actual arrays of "
     "the appropriate NrnThread."},
    {"area",
     (PyCFunction) seg_area,
     METH_NOARGS,
     "Segment area (um2) (same as h.area(sec(x), sec=sec))"},
    {"ri",
     (PyCFunction) seg_ri,
     METH_NOARGS,
     "Segment resistance to parent segment (Megohms) (same as h.ri(sec(x), "
     "sec=sec))"},
    {"volume", (PyCFunction) seg_volume, METH_NOARGS, "Segment volume (um3)"},
    {NULL}};

// I'm guessing Python should change their typedef to get rid of the
// four "deprecated conversion from string constant to 'char*'" warnings.
// Could avoid by casting each to (char*) but probably better to keep the
// warnings. For now we get rid of the warnings by copying the string to
// char array.
static char* cpstr(const char* s) {
    char* s2 = new char[strlen(s) + 1];
    strcpy(s2, s);
    return s2;
}
static PyMemberDef NPySegObj_members[] = {
    {cpstr("x"),
     T_DOUBLE,
     offsetof(NPySegObj, x_),
     0,
     cpstr("location in the section (segment containing x)")},
    {cpstr("sec"), T_OBJECT_EX, offsetof(NPySegObj, pysec_), 0, cpstr("Section")},
    {NULL}};

static PyMethodDef NPyMechObj_methods[] = {
    {"name",
     (PyCFunction) NPyMechObj_name,
     METH_NOARGS,
     "Mechanism name (same as hoc suffix for density mechanism)"},
    {"is_ion", (PyCFunction) NPyMechObj_is_ion, METH_NOARGS, "Returns True if an ion mechanism"},
    {"segment",
     (PyCFunction) NPyMechObj_segment,
     METH_NOARGS,
     "Returns the segment of the Mechanism instance"},
    {NULL}};

static PyMethodDef NPyMechFunc_methods[] = {
    {"name", (PyCFunction) NPyMechFunc_name, METH_NOARGS, "Mechanism function"},
    {"mech",
     (PyCFunction) NPyMechFunc_mech,
     METH_NOARGS,
     "Returns the Mechanism for this instance"},
    {NULL}};

static PyMethodDef NPyRangeVar_methods[] = {
    {"name", (PyCFunction) NPyRangeVar_name, METH_NOARGS, "Range variable name name"},
    {"mech",
     (PyCFunction) NPyRangeVar_mech,
     METH_NOARGS,
     "Returns nrn.Mechanism of the  RangeVariable instance"},
    {NULL}};

static PyMemberDef NPyMechObj_members[] = {{NULL}};

PyObject* nrnpy_cas(PyObject* self, PyObject* args) {
    Section* sec = nrn_noerr_access();
    if (!sec) {
        PyErr_SetString(PyExc_TypeError, "Section access unspecified");
        return NULL;
    }
    // printf("nrnpy_cas %s\n", secname(sec));
    return (PyObject*) newpysechelp(sec);
}

static PyMethodDef nrnpy_methods[] = {
    {"cas", nrnpy_cas, METH_VARARGS, "Return the currently accessed section."},
    {"allsec", nrnpy_forall, METH_VARARGS, "Return iterator over all sections."},
    {"set_psection",
     nrnpy_set_psection,
     METH_VARARGS,
     "Specify the nrn.Section.psection callback."},
    {NULL}};

#include "nrnpy_nrn.h"

static PyObject* nrnmodule_;

static void rangevars_add(Symbol* sym) {
    assert(sym && (sym->type == RANGEVAR || sym->type == RANGEOBJ));
    NPyRangeVar* r = PyObject_New(NPyRangeVar, range_type);
    // printf("%s\n", sym->name);
    r->sym_ = sym;
    r->isptr_ = 0;
    r->attr_from_sec_ = 0;
    PyDict_SetItemString(rangevars_, sym->name, (PyObject*) r);
}

PyObject* nrnpy_nrn(void) {
    int i;
    PyObject* m;

    int err = 0;
    PyObject* modules = PyImport_GetModuleDict();
    if ((m = PyDict_GetItemString(modules, "nrn")) != NULL && PyModule_Check(m)) {
        return m;
    }
    psection_type = (PyTypeObject*) PyType_FromSpec(&nrnpy_SectionType_spec);
    psection_type->tp_new = PyType_GenericNew;
    if (PyType_Ready(psection_type) < 0)
        goto fail;
    Py_INCREF(psection_type);

    pallseg_of_sec_iter_type = (PyTypeObject*) PyType_FromSpec(&nrnpy_AllSegOfSecIterType_spec);
    pseg_of_sec_iter_type = (PyTypeObject*) PyType_FromSpec(&nrnpy_SegOfSecIterType_spec);
    pallseg_of_sec_iter_type->tp_new = PyType_GenericNew;
    pseg_of_sec_iter_type->tp_new = PyType_GenericNew;
    if (PyType_Ready(pallseg_of_sec_iter_type) < 0)
        goto fail;
    if (PyType_Ready(pseg_of_sec_iter_type) < 0)
        goto fail;
    Py_INCREF(pallseg_of_sec_iter_type);
    Py_INCREF(pseg_of_sec_iter_type);

    psegment_type = (PyTypeObject*) PyType_FromSpec(&nrnpy_SegmentType_spec);
    psegment_type->tp_new = PyType_GenericNew;
    if (PyType_Ready(psegment_type) < 0)
        goto fail;
    if (PyType_Ready(pallseg_of_sec_iter_type) < 0)
        goto fail;
    if (PyType_Ready(pseg_of_sec_iter_type) < 0)
        goto fail;
    Py_INCREF(psegment_type);
    Py_INCREF(pallseg_of_sec_iter_type);
    Py_INCREF(pseg_of_sec_iter_type);

    range_type = (PyTypeObject*) PyType_FromSpec(&nrnpy_RangeType_spec);
    range_type->tp_new = PyType_GenericNew;
    if (PyType_Ready(range_type) < 0)
        goto fail;
    Py_INCREF(range_type);

    m = PyModule_Create(&nrnsectionmodule);  // like nrn but namespace will not include mechanims.
    PyModule_AddObject(m, "Section", (PyObject*) psection_type);
    PyModule_AddObject(m, "Segment", (PyObject*) psegment_type);

    err = PyDict_SetItemString(modules, "_neuron_section", m);
    assert(err == 0);
    Py_DECREF(m);
    m = PyModule_Create(&nrnmodule);  //
    nrnmodule_ = m;
    PyModule_AddObject(m, "Section", (PyObject*) psection_type);
    PyModule_AddObject(m, "Segment", (PyObject*) psegment_type);

    pmech_generic_type = (PyTypeObject*) PyType_FromSpec(&nrnpy_MechanismType_spec);
    pmechfunc_generic_type = (PyTypeObject*) PyType_FromSpec(&nrnpy_MechFuncType_spec);
    pmech_of_seg_iter_generic_type = (PyTypeObject*) PyType_FromSpec(&nrnpy_MechOfSegIterType_spec);
    pvar_of_mech_iter_generic_type = (PyTypeObject*) PyType_FromSpec(&nrnpy_VarOfMechIterType_spec);
    pmech_generic_type->tp_new = PyType_GenericNew;
    pmechfunc_generic_type->tp_new = PyType_GenericNew;
    pmech_of_seg_iter_generic_type->tp_new = PyType_GenericNew;
    pvar_of_mech_iter_generic_type->tp_new = PyType_GenericNew;
    if (PyType_Ready(pmech_generic_type) < 0)
        goto fail;
    if (PyType_Ready(pmechfunc_generic_type) < 0)
        goto fail;
    if (PyType_Ready(pmech_of_seg_iter_generic_type) < 0)
        goto fail;
    if (PyType_Ready(pvar_of_mech_iter_generic_type) < 0)
        goto fail;
    Py_INCREF(pmech_generic_type);
    Py_INCREF(pmechfunc_generic_type);
    Py_INCREF(pmech_of_seg_iter_generic_type);
    Py_INCREF(pvar_of_mech_iter_generic_type);
    PyModule_AddObject(m, "Mechanism", (PyObject*) pmech_generic_type);
    PyModule_AddObject(m, "MechFunc", (PyObject*) pmechfunc_generic_type);
    PyModule_AddObject(m, "MechOfSegIterator", (PyObject*) pmech_of_seg_iter_generic_type);
    PyModule_AddObject(m, "VarOfMechIterator", (PyObject*) pvar_of_mech_iter_generic_type);
    remake_pmech_types();
    nrnpy_reg_mech_p_ = nrnpy_reg_mech;
    nrnpy_ob_is_seg = ob_is_seg;
    nrnpy_seg_from_sec_x = seg_from_sec_x;
    nrnpy_o2sec_p_ = o2sec;
    nrnpy_o2loc_p_ = o2loc;
    nrnpy_o2loc2_p_ = o2loc2;
    nrnpy_pysec_name_p_ = pysec_name;
    nrnpy_pysec_cell_p_ = pysec_cell;
    nrnpy_pysec_cell_equals_p_ = pysec_cell_equals;

    err = PyDict_SetItemString(modules, "nrn", m);
    assert(err == 0);
    Py_DECREF(m);
    return m;
fail:
    return NULL;
}

void remake_pmech_types() {
    int i;
    Py_XDECREF(pmech_types);
    Py_XDECREF(rangevars_);
    pmech_types = PyDict_New();
    rangevars_ = PyDict_New();
    rangevars_add(hoc_table_lookup("diam", hoc_built_in_symlist));
    rangevars_add(hoc_table_lookup("cm", hoc_built_in_symlist));
    rangevars_add(hoc_table_lookup("v", hoc_built_in_symlist));
    rangevars_add(hoc_table_lookup("i_cap", hoc_built_in_symlist));
    rangevars_add(hoc_table_lookup("i_membrane_", hoc_built_in_symlist));
    for (i = 4; i < n_memb_func; ++i) {  // start at pas
        nrnpy_reg_mech(i);
    }
}

void nrnpy_reg_mech(int type) {
    int i;
    char* s;
    Memb_func& mf = memb_func[type];
    if (!nrnmodule_) {
        return;
    }
    if (mf.is_point) {
        if (nrn_is_artificial_[type] == 0) {
            Symlist* sl = nrn_pnt_template_[type]->symtable;
            Symbol* s = hoc_table_lookup("get_segment", sl);
            if (!s) {
                s = hoc_install("get_segment", OBFUNCTION, 0, &sl);
                s->cpublic = 1;
                s->u.u_proc->defn.pfo = (Object * *(*) ()) pp_get_segment;
            }
        }
        return;
    }
    s = mf.sym->name;
    // printf("nrnpy_reg_mech %s %d\n", s, type);
    if (PyDict_GetItemString(pmech_types, s)) {
        hoc_execerror(s, "mechanism already exists");
    }
    Py_INCREF(pmech_generic_type);
    PyModule_AddObject(nrnmodule_, s, (PyObject*) pmech_generic_type);
    PyDict_SetItemString(pmech_types, s, Py_BuildValue("i", type));
    for (i = 0; i < mf.sym->s_varn; ++i) {
        Symbol* sym = mf.sym->u.ppsym[i];
        rangevars_add(sym);
    }
}

void nrnpy_unreg_mech(int type) {
    // not implemented but needed when KSChan name changed.
}
