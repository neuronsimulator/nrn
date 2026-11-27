// The fast path of the interface's hot attribute and item accesses (prototype).
//
// Only the common case is decided here: a scalar range variable of a segment or of a density
// mechanism at an interior x, a Vector element at an int index, the segments of a section. The
// rules for this file:
// - It reads only tables and slots that the interface (Python) owns and fills after its own slow
//   path resolved a name. It never resolves a name or caches anything itself.
// - Every table has an invalidation owner. The symbol tables are cleared here whenever KSChan
//   renames or frees a symbol (nrn_kschan_symbols_changed); no other code renames or frees a
//   range variable's symbol.
// - Nothing is raised here. Anything else, and every failure, calls the interface's Python method
//   (_getattr, _setattr, _getitem, _setitem), which raises NEURON's error.
// - Only exact instances of the interface's Segment, Mechanism and Vector take it: a subclass
//   always goes through the Python methods.
// - NRN_PYTHON_FAST_PATH=0 (exactly "0", as NEURON compares its other switches, e.g.
//   NRN_NOGUI=1) leaves the slots out, so the Python classes are as without it.
#include <../../nrnconf.h>

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "hocdec.h"
#include "neuronapi.h"
#include "nrnpy_capi.h"
#include "nrnpy_object.h"
#include "nrnpy_types.h"

extern std::size_t nrn_kschan_symbols_changed;

namespace neuron::python::fast {
namespace {

struct Tables {
    // Filled by the interface: name -> Symbol address (an int).
    PyObject* rangevars{};   // Segment: scalar range variables, not diam, not POINTERs
    PyObject* mechanisms{};  // mechanism name -> {unsuffixed name -> Symbol address}
    std::size_t symbols_seen{};
    // The interface's model epoch (a list of one element) and section address -> nseg generation.
    // Set once as class attributes of the Segment base; the interface never rebinds them.
    PyObject *epoch{}, *nseg_generation{};
    // The interface's own classes: only exact instances take the fast path, so a subclass always
    // has the Python methods' behaviour (its class attributes first, then _getattr).
    PyObject *segment_class{}, *mechanism_class{}, *vector_class{};
    PyObject *s_getattr{}, *s_setattr{}, *s_getitem{}, *s_setitem{}, *zero{};
} t;

/// Forget every name when KSChan renamed or freed a symbol since the last look.
void forget_moved_symbols() {
    if (t.symbols_seen != nrn_kschan_symbols_changed) {
        t.symbols_seen = nrn_kschan_symbols_changed;
        py.PyDict_Clear(t.rangevars);
        py.PyDict_Clear(t.mechanisms);
    }
}

bool is_str(PyObject* o) {
    return type_of(o) == py.PyUnicode_Type;
}

/// An underscore name that is not _ref_*: one of the interface's slots or methods.
bool internal(PyObject* name) {
    char const* const s = py.PyUnicode_AsUTF8AndSize(name, nullptr);
    if (!s) {
        py.PyErr_Clear();
        return false;
    }
    return s[0] == '_' && std::strncmp(s, "_ref_", 5) != 0;
}

/// The section of a Section wrapper, or null.
Section* section_of(PyObject* wrapper) {
    if (!wrapper || !is_instance(wrapper, types.section)) {
        return nullptr;
    }
    auto* const address = slot(wrapper, SectionSlot::sec);
    return address && type_of(address) == py.PyLong_Type
               ? static_cast<Section*>(py.PyLong_AsVoidPtr(address))
               : nullptr;
}

/// An interior x from a float slot.
bool interior(PyObject* position, double& x) {
    if (!position || type_of(position) != py.PyFloat_Type) {
        return false;
    }
    x = py.PyFloat_AsDouble(position);
    return 0.0 < x && x < 1.0;
}

bool segment_location(PyObject* segment, Section*& sec, double& x) {
    sec = section_of(slot(segment, SegmentSlot::sec));
    return sec && interior(slot(segment, SegmentSlot::x), x);
}

/// a == b for two ints, without an error.
bool same(PyObject* a, PyObject* b) {
    if (a == b) {
        return true;
    }
    if (!a || !b || type_of(a) != py.PyLong_Type || type_of(b) != py.PyLong_Type) {
        return false;
    }
    long const x = py.PyLong_AsLong(a), y = py.PyLong_AsLong(b);
    if ((x == -1 || y == -1) && py.PyErr_Occurred()) {
        py.PyErr_Clear();
        return false;
    }
    return x == y;
}

/// A class attribute of the Segment base that the interface sets, borrowed; read once.
PyObject* shared(PyObject*& cache, const char* name) {
    if (!cache && !(cache = py.PyObject_GetAttrString(types.segment, name))) {
        py.PyErr_Clear();
    }
    return cache;
}

/// The interface's model epoch, borrowed, or null while it is unknown (None).
PyObject* current_epoch() {
    auto* const list = shared(t.epoch, "_fast_epoch");
    auto* const epoch = list ? py.PyList_GetItem(list, 0) : nullptr;
    if (!epoch) {
        py.PyErr_Clear();
    }
    return epoch == py._Py_NoneStruct ? nullptr : epoch;
}

/// A held Segment's cached Mechanism, when the interface's own test would accept it: its epoch is
/// the current one and so is its section's nseg generation (Segment._getattr).
PyObject* cached_mechanism(PyObject* self, PyObject* name) {
    auto* const cache = slot(self, SegmentSlot::mech_cache);
    auto* const mech = cache && type_of(cache) == py.PyDict_Type ? py.PyDict_GetItem(cache, name)
                                                                 : nullptr;
    auto* const epoch = current_epoch();
    auto* const section_wrapper = slot(self, SegmentSlot::sec);
    auto* const generations = shared(t.nseg_generation, "_fast_nseg_generation");
    if (!mech || !epoch || !generations || !is_instance(mech, types.mechanism) ||
        !same(slot(mech, MechanismSlot::epoch), epoch) || !section_of(section_wrapper)) {
        return nullptr;
    }
    auto* g = py.PyDict_GetItem(generations, slot(section_wrapper, SectionSlot::sec));
    return same(slot(mech, MechanismSlot::nseg), g ? g : t.zero) ? Ref::borrow(mech).release()
                                                                 : nullptr;
}

/// Section and interior x of a Mechanism whose validity the interface checked at this epoch.
bool mechanism_location(PyObject* self, Section*& sec, double& x) {
    auto* const segment = slot(self, MechanismSlot::seg);
    auto* const epoch = current_epoch();
    if (!segment || !epoch || !is_instance(segment, types.segment) ||
        !same(slot(self, MechanismSlot::epoch), epoch)) {
        return false;
    }
    sec = section_of(slot(segment, SegmentSlot::sec));
    return sec && interior(slot(self, MechanismSlot::x), x);
}

PyObject* rangevar_symbol(PyObject*, PyObject* name) {
    return py.PyDict_GetItem(t.rangevars, name);
}

PyObject* mechanism_symbol(PyObject* self, PyObject* name) {
    auto* const mech_name = slot(self, MechanismSlot::name);
    auto* const variables = mech_name && is_str(mech_name)
                                ? py.PyDict_GetItem(t.mechanisms, mech_name)
                                : nullptr;
    return variables ? py.PyDict_GetItem(variables, name) : nullptr;
}

Symbol* symbol(PyObject* address) {
    return static_cast<Symbol*>(py.PyLong_AsVoidPtr(address));
}

/// A range variable write of an exact int or float: false for anything the slow path must see.
bool write(PyObject* address, Section* sec, double x, PyObject* value) {
    double d;
    if (type_of(value) == py.PyFloat_Type) {
        d = py.PyFloat_AsDouble(value);
    } else if (type_of(value) != py.PyLong_Type) {
        return false;
    } else if ((d = py.PyLong_AsDouble(value)) == -1.0 && py.PyErr_Occurred()) {
        py.PyErr_Clear();
        return false;
    }
    // diam and POINTERs are never in the tables; set_nothrow does the rest of HOC's assignment.
    return nrn_rangevar_set_nothrow(symbol(address), sec, x, 0, d, nullptr, 0) == 0;
}

int status(PyObject* result) {
    Ref const r{result};
    return r ? 0 : -1;
}

/// The interface's method of the class, or null with no error if it has none.
Ref method(PyObject* self, PyObject* name) {
    Ref f{py.PyObject_GetAttr(type_of(self), name)};
    if (!f && py.PyErr_ExceptionMatches(py.PyExc_AttributeError)) {
        py.PyErr_Clear();
    }
    return f;
}

/// The interface's _getattr, or the AttributeError of generic lookup for a class without it (one
/// with a Python __getattr__, which Python calls next).
PyObject* getattr_fallback(PyObject* self, PyObject* name) {
    if (Ref const f = method(self, t.s_getattr)) {
        return py.PyObject_CallFunctionObjArgs(f.get(), self, name, nullptr);
    }
    if (!py.PyErr_Occurred()) {
        py.PyErr_SetObject(py.PyExc_AttributeError, name);
    }
    return nullptr;
}

int setattr_fallback(PyObject* self, PyObject* name, PyObject* value) {
    if (Ref const f = method(self, t.s_setattr)) {
        return status(py.PyObject_CallFunctionObjArgs(f.get(), self, name, value, nullptr));
    }
    return py.PyErr_Occurred() ? -1 : py.PyObject_GenericSetAttr(self, name, value);
}

using Lookup = PyObject* (*) (PyObject*, PyObject*);
using Locate = bool (*)(PyObject*, Section*&, double&);
using Getattro = PyObject* (*) (PyObject*, PyObject*);

/// Is self an instance of exactly the interface's class (set as a class attribute of the base)?
bool exact(PyObject* self, PyObject*& cls, const char* name) {
    return type_of(self) == shared(cls, name);
}

/// tp_getattro of Segment and Mechanism. entry is the slot function itself: a Python subclass
/// that defines __getattr__ reaches it through CPython's hook, which then calls that __getattr__,
/// as without the fast path.
template <Lookup lookup, Locate locate, bool segment, Getattro entry>
PyObject* getattro(PyObject* self, PyObject* name) {
    bool const fast = segment ? exact(self, t.segment_class, "_fast_segment_class")
                              : exact(self, t.mechanism_class, "_fast_mechanism_class");
    if (fast && is_str(name) && !internal(name)) {
        forget_moved_symbols();
        if (auto* const address = lookup(self, name)) {
            Section* sec;
            double x;
            if (locate(self, sec, x)) {
                double const v = nrn_rangevar_get_or_nan(symbol(address), sec, x);
                if (!std::isnan(v)) {
                    return py.PyFloat_FromDouble(v);
                }
            }
            return getattr_fallback(self, name);
        }
        if (segment && py.PyDict_GetItem(t.mechanisms, name)) {
            auto* const mech = cached_mechanism(self, name);
            return mech ? mech : getattr_fallback(self, name);
        }
    }
    auto* const found = py.PyObject_GenericGetAttr(self, name);
    if (found || !py.PyErr_ExceptionMatches(py.PyExc_AttributeError) ||
        (!fast &&
         py.PyType_GetSlot(type_of(self), stable::tp_getattro) != reinterpret_cast<void*>(entry))) {
        return found;  // the subclass's own __getattr__ comes next
    }
    py.PyErr_Clear();
    return getattr_fallback(self, name);
}

/// tp_setattro of Segment and Mechanism.
template <Lookup lookup, Locate locate, bool segment>
int setattro(PyObject* self, PyObject* name, PyObject* value) {
    bool const fast = segment ? exact(self, t.segment_class, "_fast_segment_class")
                              : exact(self, t.mechanism_class, "_fast_mechanism_class");
    if (!value || !is_str(name) || internal(name)) {
        return py.PyObject_GenericSetAttr(self, name, value);
    }
    if (!fast) {
        return setattr_fallback(self, name, value);
    }
    forget_moved_symbols();
    if (auto* const address = lookup(self, name)) {
        Section* sec;
        double x;
        if (locate(self, sec, x) && write(address, sec, x, value)) {
            return 0;
        }
    }
    return setattr_fallback(self, name, value);
}

// ---- the segments of a section

struct SegmentIterator: Head {
    PyObject* section;
    std::ptrdiff_t i;
};
PyObject* iterator_type{};

SegmentIterator* iterator(PyObject* self) {
    return static_cast<SegmentIterator*>(reinterpret_cast<Head*>(self));
}

PyObject* iterator_self(PyObject* self) {
    return Ref::borrow(self).release();
}

// As NEURON: nseg is read again at each step, and a deleted section ends the loop.
PyObject* iterator_next(PyObject* self) {
    auto* const it = iterator(self);
    Section* const sec = it->section ? section(it->section) : nullptr;
    if (!sec || !nrn_section_is_active(sec) || it->i >= nrn_nseg_get(sec)) {
        return nullptr;
    }
    double const x = (it->i++ + 0.5) / nrn_nseg_get(sec);
    return segment(it->section, x).release();
}

int iterator_traverse(PyObject* self, int (*visit)(PyObject*, void*), void* arg) {
    int const r = visit(type_of(self), arg);
    return r || !iterator(self)->section ? r : visit(iterator(self)->section, arg);
}

int iterator_clear(PyObject* self) {
    Ref{std::exchange(iterator(self)->section, nullptr)};
    return 0;
}

void iterator_dealloc(PyObject* self) {
    auto* const type = type_of(self);
    py.PyObject_GC_UnTrack(self);
    iterator_clear(self);
    py.PyObject_GC_Del(self);
    py.Py_DecRef(type);
}

// Section._segments(): an iterator over the segments; the interface checks the section first.
PyObject* section_segments(PyObject* self, PyObject*) {
    auto* const it = py.PyType_GenericAlloc(iterator_type, 0);
    if (it) {
        iterator(it)->section = Ref::borrow(self).release();
    }
    return it;
}

// ---- Vector elements

cTemplate* vector_template{};

/// The element of a Vector for an exact int key in range, or null.
double* element(PyObject* self, PyObject* key) {
    Object* const o = type_of(key) == py.PyLong_Type &&
                              exact(self, t.vector_class, "_fast_vector_class")
                          ? hoc_object(self)
                          : nullptr;
    if (!o) {
        return nullptr;
    }
    if (!vector_template && !std::strcmp(o->ctemplate->sym->name, "Vector")) {
        vector_template = o->ctemplate;  // once: only the interface's Vector has this base
    }
    std::ptrdiff_t i = py.PyLong_AsSsize_t(key);
    if (o->ctemplate != vector_template || (i == -1 && py.PyErr_Occurred())) {
        py.PyErr_Clear();
        return nullptr;
    }
    std::ptrdiff_t const size = nrn_vector_capacity(o);
    i += i < 0 ? size : 0;
    return i >= 0 && i < size ? nrn_vector_data(o) + i : nullptr;
}

PyObject* vector_getitem(PyObject* self, PyObject* key) {
    if (double const* const pd = element(self, key)) {
        return py.PyFloat_FromDouble(*pd);
    }
    return py.PyObject_CallMethodObjArgs(self, t.s_getitem, key, nullptr);
}

int vector_setitem(PyObject* self, PyObject* key, PyObject* value) {
    if (!value) {
        py.PyErr_SetString(py.PyExc_TypeError, "'Vector' object doesn't support item deletion");
        return -1;
    }
    double* const pd = type_of(value) == py.PyFloat_Type || type_of(value) == py.PyLong_Type
                           ? element(self, key)
                           : nullptr;
    if (pd) {
        double const d = type_of(value) == py.PyFloat_Type ? py.PyFloat_AsDouble(value)
                                                           : py.PyLong_AsDouble(value);
        if (!(d == -1.0 && py.PyErr_Occurred())) {
            *pd = d;
            return 0;
        }
        py.PyErr_Clear();
    }
    return status(py.PyObject_CallMethodObjArgs(self, t.s_setitem, key, value, nullptr));
}

// The sequence protocol (PySequence_Check, reversed, the C bisect) as without the fast path:
// through the same code as vec[i].
PyObject* vector_item(PyObject* self, std::ptrdiff_t i) {
    Ref const key{py.PyLong_FromLongLong(i)};
    return key ? vector_getitem(self, key.get()) : nullptr;
}

PyObject* table(const char* name) {
    Ref dict{py.PyDict_New()};
    if (!dict || py.PyObject_SetAttrString(types.segment, name, dict.get())) {
        return nullptr;
    }
    return dict.release();
}

}  // namespace

stable::Method section_methods[]{
    {"_segments", reinterpret_cast<void*>(section_segments), stable::meth_noargs, nullptr},
    {nullptr, nullptr, 0, nullptr}};

PyObject* segment_getattro(PyObject* self, PyObject* name) {
    return getattro<rangevar_symbol, segment_location, true, segment_getattro>(self, name);
}

int segment_setattro(PyObject* self, PyObject* name, PyObject* value) {
    return setattro<rangevar_symbol, segment_location, true>(self, name, value);
}

PyObject* mechanism_getattro(PyObject* self, PyObject* name) {
    return getattro<mechanism_symbol, mechanism_location, false, mechanism_getattro>(self, name);
}

int mechanism_setattro(PyObject* self, PyObject* name, PyObject* value) {
    return setattro<mechanism_symbol, mechanism_location, false>(self, name, value);
}

bool enabled() {
    char const* const setting = std::getenv("NRN_PYTHON_FAST_PATH");
    return !setting || std::strcmp(setting, "0") != 0;
}

bool make_types() {
    using stable::Slot;
    t.s_getattr = py.PyUnicode_InternFromString("_getattr");
    t.s_setattr = py.PyUnicode_InternFromString("_setattr");
    t.s_getitem = py.PyUnicode_InternFromString("_getitem");
    t.s_setitem = py.PyUnicode_InternFromString("_setitem");
    t.zero = py.PyLong_FromLongLong(0);
    t.rangevars = table("_fast_rangevars");
    t.mechanisms = table("_fast_mechanisms");
    t.symbols_seen = nrn_kschan_symbols_changed;

    static Slot iterator_slots[]{{stable::tp_iter, reinterpret_cast<void*>(iterator_self)},
                                 {stable::tp_iternext, reinterpret_cast<void*>(iterator_next)},
                                 {stable::tp_traverse, reinterpret_cast<void*>(iterator_traverse)},
                                 {stable::tp_clear, reinterpret_cast<void*>(iterator_clear)},
                                 {stable::tp_dealloc, reinterpret_cast<void*>(iterator_dealloc)},
                                 {0, nullptr}};
    static stable::Spec iterator_spec{"nrn._SegmentIterator",
                                      sizeof(SegmentIterator),
                                      0,
                                      // Only Section._segments() makes one.
                                      static_cast<unsigned>(stable::tpflags_have_gc |
                                                            stable::tpflags_disallow_instantiation),
                                      iterator_slots};
    iterator_type = py.PyType_FromSpec(&iterator_spec);

    // No fields of its own: the interface's Vector derives from it and from its Object class.
    static Slot vector_slots[]{{stable::mp_subscript, reinterpret_cast<void*>(vector_getitem)},
                               {stable::mp_ass_subscript, reinterpret_cast<void*>(vector_setitem)},
                               {stable::sq_item, reinterpret_cast<void*>(vector_item)},
                               {0, nullptr}};
    // The collector's flag and functions are inherited from the base.
    static stable::Spec vector_spec{
        "hoc._Vector", 0, 0, static_cast<unsigned>(stable::tpflags_basetype), vector_slots};
    types.vector = py.PyType_FromSpecWithBases(&vector_spec, types.object);
    return t.s_getattr && t.s_setattr && t.s_getitem && t.s_setitem && t.zero && t.rangevars &&
           t.mechanisms && iterator_type && types.vector;
}

}  // namespace neuron::python::fast
