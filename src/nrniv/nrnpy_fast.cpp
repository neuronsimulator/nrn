// The fast path of the interface's hot attribute and item accesses (prototype).
//
// Only the common case is decided here: a scalar range variable of a segment or of a density
// mechanism at an interior x, a Vector element at an int index, the segments of a section. The
// names and the cache validity are the interface's (Python) decisions: it fills the tables below
// after its own slow path has resolved a name once. Anything else, and every failure, calls the
// interface's Python method (_getattr, _setattr, _getitem, _setitem), which raises NEURON's error.
// No error is raised here, so the Python method is the only place that knows the error rules.
#include <../../nrnconf.h>

#include <cmath>
#include <cstring>

#include "hocdec.h"
#include "neuronapi.h"
#include "nrnpy_capi.h"
#include "nrnpy_object.h"
#include "nrnpy_types.h"

namespace neuron::python::fast {
namespace {

/// What the interface fills: name -> Symbol address (an int).
struct Tables {
    PyObject* rangevars{};   // Segment: scalar range variables, not diam, not POINTERs
    PyObject* mechanisms{};  // mechanism name -> {unsuffixed name -> Symbol address}
    PyObject* misses{};      // Segment: names that generic lookup does not find
    PyObject* epoch{};       // the interface's model epoch, a list of one element
    PyObject* nseg_generation{};  // section address -> nseg generation
    PyObject* nseg_synced_at{};   // section address -> epoch of the last nseg read
    PyObject* mechanism_class{};  // the interface's Mechanism
    PyObject *s_getattr{}, *s_setattr{}, *s_getitem{}, *s_setitem{}, *s_mech_cache{}, *zero{};
} t;

Symbol* symbol(PyObject* address) {
    return static_cast<Symbol*>(py.PyLong_AsVoidPtr(address));
}

bool is_str(PyObject* o) {
    return type_of(o) == py.PyUnicode_Type;
}

/// Section and interior x of a Segment, or false.
bool location(PyObject* segment, Section*& sec, double& x) {
    auto* const section_wrapper = slot(segment, SegmentSlot::sec);
    auto* const position = slot(segment, SegmentSlot::x);
    if (!section_wrapper || !position || type_of(position) != py.PyFloat_Type ||
        !is_instance(section_wrapper, types.section)) {
        return false;
    }
    auto* const address = slot(section_wrapper, SectionSlot::sec);
    if (!address || type_of(address) != py.PyLong_Type) {
        return false;
    }
    x = py.PyFloat_AsDouble(position);
    sec = static_cast<Section*>(py.PyLong_AsVoidPtr(address));
    return sec && 0.0 < x && x < 1.0;
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

/// The interface's model epoch, borrowed, or null while it is unknown (None).
PyObject* current_epoch() {
    if (!t.epoch) {
        t.epoch = py.PyObject_GetAttrString(types.segment, "_fast_epoch");
        if (!t.epoch) {
            py.PyErr_Clear();
            return nullptr;
        }
    }
    auto* const epoch = py.PyList_GetItem(t.epoch, 0);
    if (!epoch) {
        py.PyErr_Clear();
    }
    return epoch == py._Py_NoneStruct ? nullptr : epoch;
}

/// A class attribute of the Segment base that the interface sets, borrowed; cached.
PyObject* attribute(PyObject*& cache, const char* name) {
    if (!cache) {
        cache = py.PyObject_GetAttrString(types.segment, name);
        if (!cache) {
            py.PyErr_Clear();
        }
    }
    return cache;
}

PyObject* generation(PyObject* section_address) {
    if (!t.nseg_generation) {
        t.nseg_generation = py.PyObject_GetAttrString(types.segment, "_fast_nseg_generation");
        if (!t.nseg_generation) {
            py.PyErr_Clear();
            return nullptr;
        }
    }
    auto* const g = py.PyDict_GetItem(t.nseg_generation, section_address);
    return g ? g : t.zero;
}

/// A range variable read: NaN for anything the slow path must look at.
double read(PyObject* address, Section* sec, double x) {
    return nrn_rangevar_get_or_nan(symbol(address), sec, x);
}

/// A range variable write of an int or float: false for anything the slow path must look at.
bool write(PyObject* address, Section* sec, double x, PyObject* value) {
    double d;
    if (type_of(value) == py.PyFloat_Type) {
        d = py.PyFloat_AsDouble(value);
    } else if (type_of(value) == py.PyLong_Type) {
        d = py.PyLong_AsDouble(value);
        if (d == -1.0 && py.PyErr_Occurred()) {
            py.PyErr_Clear();
            return false;
        }
    } else {
        return false;
    }
    // diam and POINTERs are never in the tables; set_nothrow does the rest of HOC's assignment.
    return nrn_rangevar_set_nothrow(symbol(address), sec, x, 0, d, nullptr, 0) == 0;
}

PyObject* number(double d) {
    return py.PyFloat_FromDouble(d);
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

/// An underscore name that is not _ref_*: a slot, as the interface's __setattr__ says.
bool internal(PyObject* name) {
    char const* const s = py.PyUnicode_AsUTF8AndSize(name, nullptr);
    if (!s) {
        py.PyErr_Clear();
        return false;
    }
    return s[0] == '_' && std::strncmp(s, "_ref_", 5) != 0;
}

/// The cached Mechanism of a held Segment, when the interface's own check would accept it.
PyObject* cached_mechanism(PyObject* self, PyObject* name) {
    auto* const cache = slot(self, SegmentSlot::mech_cache);
    if (!cache || type_of(cache) != py.PyDict_Type) {
        return nullptr;
    }
    auto* const mech = py.PyDict_GetItem(cache, name);
    auto* const epoch = current_epoch();
    if (!mech || !epoch || !is_instance(mech, types.mechanism) ||
        !same(slot(mech, MechanismSlot::epoch), epoch)) {
        return nullptr;
    }
    auto* const section_wrapper = slot(self, SegmentSlot::sec);
    if (!section_wrapper || !is_instance(section_wrapper, types.section)) {
        return nullptr;
    }
    auto* const g = generation(slot(section_wrapper, SectionSlot::sec));
    if (!g || !same(slot(mech, MechanismSlot::nseg), g)) {
        return nullptr;
    }
    return Ref::borrow(mech).release();
}

/// A new Mechanism for a mechanism name at a fresh Segment, made as the interface's _getattr makes
/// it, when its section's list of inserted mechanisms is current and has it, and the section's
/// nseg was read at this epoch. Cached in the Segment. Null otherwise, with no error.
PyObject* new_mechanism(PyObject* self, PyObject* name) {
    auto* const section_wrapper = slot(self, SegmentSlot::sec);
    auto* const position = slot(self, SegmentSlot::x);
    auto* const epoch = current_epoch();
    auto* const cls = attribute(t.mechanism_class, "_fast_mechanism_class");
    if (!epoch || !cls || !section_wrapper || !position || type_of(position) != py.PyFloat_Type ||
        !is_instance(section_wrapper, types.section) || !attribute(t.nseg_synced_at, "_fast_nseg_synced_at")) {
        return nullptr;
    }
    double const x = py.PyFloat_AsDouble(position);
    auto* const inserted = slot(section_wrapper, SectionSlot::inserted_mechs);
    auto* const address = slot(section_wrapper, SectionSlot::sec);
    if (!(0.0 < x && x < 1.0) || !inserted || type_of(inserted) != py.PyList_Type ||
        !same(slot(section_wrapper, SectionSlot::inserted_mechs_epoch), epoch) ||
        !same(py.PyDict_GetItem(t.nseg_synced_at, address), epoch)) {
        return nullptr;
    }
    int const has = py.PySequence_Contains(inserted, name);
    if (has != 1) {
        if (has < 0) {
            py.PyErr_Clear();
        }
        return nullptr;
    }
    auto* const g = generation(address);
    std::ptrdiff_t length;
    char const* const s = py.PyUnicode_AsUTF8AndSize(name, &length);
    if (!g || !s) {
        py.PyErr_Clear();
        return nullptr;
    }
    bool const ion = length >= 4 && !std::strcmp(s + length - 4, "_ion");
    // Its own Segment, not self: Segment -> cache -> Mechanism -> Segment would be a cycle.
    Ref const copy = segment(section_wrapper, x);
    Ref mech{py.PyType_GenericAlloc(cls, 0)};
    if (!copy || !mech) {
        py.PyErr_Clear();
        return nullptr;
    }
    PyObject* const values[MechanismSlot::count]{copy.get(), position, name,
        ion ? py._Py_TrueStruct : py._Py_FalseStruct, epoch, g, g};
    for (int i = 0; i < MechanismSlot::count; ++i) {
        reinterpret_cast<PyObject**>(reinterpret_cast<Head*>(mech.get()) + 1)[i] =
            Ref::borrow(values[i]).release();
    }
    auto* cache = slot(self, SegmentSlot::mech_cache);
    if (!cache || cache == py._Py_NoneStruct) {
        Ref fresh{py.PyDict_New()};
        if (!fresh || py.PyObject_GenericSetAttr(self, t.s_mech_cache, fresh.get())) {
            py.PyErr_Clear();
            return nullptr;
        }
        cache = slot(self, SegmentSlot::mech_cache);
    }
    if (type_of(cache) != py.PyDict_Type || py.PyDict_SetItem(cache, name, mech.get())) {
        py.PyErr_Clear();
        return nullptr;
    }
    return mech.release();
}

/// Section and interior x of a Mechanism whose validity the interface checked at this epoch.
bool mechanism_location(PyObject* self, Section*& sec, double& x) {
    auto* const segment = slot(self, MechanismSlot::seg);
    auto* const position = slot(self, MechanismSlot::x);
    auto* const epoch = current_epoch();
    if (!segment || !position || !epoch || type_of(position) != py.PyFloat_Type ||
        !is_instance(segment, types.segment) || !same(slot(self, MechanismSlot::epoch), epoch) ||
        !location(segment, sec, x)) {
        return false;
    }
    x = py.PyFloat_AsDouble(position);
    return 0.0 < x && x < 1.0;
}

PyObject* mechanism_symbol(PyObject* self, PyObject* name) {
    auto* const mech_name = slot(self, MechanismSlot::name);
    if (!mech_name || !is_str(mech_name) || !is_str(name)) {
        return nullptr;
    }
    auto* const variables = py.PyDict_GetItem(t.mechanisms, mech_name);
    return variables ? py.PyDict_GetItem(variables, name) : nullptr;
}

// ---- the segments of a section

struct SegmentIterator: Head {
    PyObject* section;
    std::ptrdiff_t i;
};
PyObject* iterator_type{};

PyObject* iterator_self(PyObject* self) {
    return Ref::borrow(self).release();
}

// As NEURON: nseg is read again at each step, and a deleted section ends the loop.
PyObject* iterator_next(PyObject* self) {
    auto* const it = static_cast<SegmentIterator*>(reinterpret_cast<Head*>(self));
    Section* const sec = section(it->section);
    if (!sec || !nrn_section_is_active(sec)) {
        return nullptr;
    }
    int const nseg = nrn_nseg_get(sec);
    if (it->i >= nseg) {
        return nullptr;
    }
    double const x = (it->i + 0.5) / nseg;
    ++it->i;
    return segment(it->section, x).release();
}

int iterator_traverse(PyObject* self, int (*visit)(PyObject*, void*), void* arg) {
    auto* const it = static_cast<SegmentIterator*>(reinterpret_cast<Head*>(self));
    int const r = visit(type_of(self), arg);
    return r || !it->section ? r : visit(it->section, arg);
}

int iterator_clear(PyObject* self) {
    auto* const it = static_cast<SegmentIterator*>(reinterpret_cast<Head*>(self));
    Ref{std::exchange(it->section, nullptr)};
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
        static_cast<SegmentIterator*>(reinterpret_cast<Head*>(it))->section =
            Ref::borrow(self).release();
    }
    return it;
}

// ---- Vector elements

cTemplate* vector_template{};

/// The data and index i of a Vector for an int key, or null.
double* element(PyObject* self, PyObject* key) {
    if (type_of(key) != py.PyLong_Type) {
        return nullptr;
    }
    Object* const o = hoc_object(self);
    if (!o) {
        return nullptr;
    }
    if (o->ctemplate != vector_template) {
        if (vector_template || std::strcmp(o->ctemplate->sym->name, "Vector")) {
            return nullptr;
        }
        vector_template = o->ctemplate;
    }
    std::ptrdiff_t i = py.PyLong_AsSsize_t(key);
    if (i == -1 && py.PyErr_Occurred()) {
        py.PyErr_Clear();
        return nullptr;
    }
    std::ptrdiff_t const size = nrn_vector_capacity(o);
    if (i < 0) {
        i += size;
    }
    return i >= 0 && i < size ? nrn_vector_data(o) + i : nullptr;
}

PyObject* vector_getitem(PyObject* self, PyObject* key) {
    if (double const* const pd = element(self, key)) {
        return number(*pd);
    }
    return py.PyObject_CallMethodObjArgs(self, t.s_getitem, key, nullptr);
}

int vector_setitem(PyObject* self, PyObject* key, PyObject* value) {
    if (value && (type_of(value) == py.PyFloat_Type || type_of(value) == py.PyLong_Type)) {
        if (double* const pd = element(self, key)) {
            double const d = type_of(value) == py.PyFloat_Type ? py.PyFloat_AsDouble(value)
                                                                : py.PyLong_AsDouble(value);
            if (!(d == -1.0 && py.PyErr_Occurred())) {
                *pd = d;
                return 0;
            }
            py.PyErr_Clear();
        }
    }
    if (!value) {
        py.PyErr_SetString(py.PyExc_TypeError, "'Vector' object doesn't support item deletion");
        return -1;
    }
    return status(py.PyObject_CallMethodObjArgs(self, t.s_setitem, key, value, nullptr));
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
    // The interface's own slots and methods (self._sec, self._x, ...) first: no table has them.
    if (is_str(name) && !internal(name)) {
        if (auto* const address = py.PyDict_GetItem(t.rangevars, name)) {
            Section* sec;
            double x;
            if (location(self, sec, x)) {
                double const v = read(address, sec, x);
                if (!std::isnan(v)) {
                    return number(v);
                }
            }
            return getattr_fallback(self, name);
        }
        if (py.PyDict_GetItem(t.mechanisms, name)) {
            if (auto* const mech = cached_mechanism(self, name)) {
                return mech;
            }
            if (auto* const mech = new_mechanism(self, name)) {
                return mech;
            }
            return getattr_fallback(self, name);
        }
        if (py.PyDict_GetItem(t.misses, name) &&
            type_of(self) == need(Hook::segment_class).get()) {
            return getattr_fallback(self, name);
        }
    }
    auto* const found = py.PyObject_GenericGetAttr(self, name);
    if (found || !py.PyErr_ExceptionMatches(py.PyExc_AttributeError)) {
        return found;
    }
    py.PyErr_Clear();
    // Generic lookup reads the class and the slots. A slot can be unset, so only names that
    // cannot be one are remembered.
    if (is_str(name) && !internal(name) && type_of(self) == need(Hook::segment_class).get()) {
        py.PyDict_SetItem(t.misses, name, py._Py_NoneStruct);
    }
    return getattr_fallback(self, name);
}

int segment_setattro(PyObject* self, PyObject* name, PyObject* value) {
    if (!value || !is_str(name)) {
        return py.PyObject_GenericSetAttr(self, name, value);
    }
    if (auto* const address = py.PyDict_GetItem(t.rangevars, name)) {
        Section* sec;
        double x;
        if (location(self, sec, x) && write(address, sec, x, value)) {
            return 0;
        }
    } else if (internal(name)) {
        return py.PyObject_GenericSetAttr(self, name, value);
    }
    return setattr_fallback(self, name, value);
}

PyObject* mechanism_getattro(PyObject* self, PyObject* name) {
    if (auto* const address = mechanism_symbol(self, name)) {
        Section* sec;
        double x;
        if (mechanism_location(self, sec, x)) {
            double const v = read(address, sec, x);
            if (!std::isnan(v)) {
                return number(v);
            }
        }
        return getattr_fallback(self, name);
    }
    auto* const found = py.PyObject_GenericGetAttr(self, name);
    if (found || !py.PyErr_ExceptionMatches(py.PyExc_AttributeError)) {
        return found;
    }
    py.PyErr_Clear();
    return getattr_fallback(self, name);
}

int mechanism_setattro(PyObject* self, PyObject* name, PyObject* value) {
    if (!value || !is_str(name)) {
        return py.PyObject_GenericSetAttr(self, name, value);
    }
    if (auto* const address = mechanism_symbol(self, name)) {
        Section* sec;
        double x;
        if (mechanism_location(self, sec, x) && write(address, sec, x, value)) {
            return 0;
        }
    } else if (internal(name)) {
        return py.PyObject_GenericSetAttr(self, name, value);
    }
    return setattr_fallback(self, name, value);
}

bool make_types() {
    using stable::Slot;
    t.s_getattr = py.PyUnicode_InternFromString("_getattr");
    t.s_setattr = py.PyUnicode_InternFromString("_setattr");
    t.s_getitem = py.PyUnicode_InternFromString("_getitem");
    t.s_setitem = py.PyUnicode_InternFromString("_setitem");
    t.s_mech_cache = py.PyUnicode_InternFromString("_mech_cache");
    t.zero = py.PyLong_FromLongLong(0);
    t.rangevars = table("_fast_rangevars");
    t.mechanisms = table("_fast_mechanisms");
    t.misses = table("_fast_misses");

    static Slot iterator_slots[]{
        {stable::tp_iter, reinterpret_cast<void*>(iterator_self)},
        {stable::tp_iternext, reinterpret_cast<void*>(iterator_next)},
        {stable::tp_traverse, reinterpret_cast<void*>(iterator_traverse)},
        {stable::tp_clear, reinterpret_cast<void*>(iterator_clear)},
        {stable::tp_dealloc, reinterpret_cast<void*>(iterator_dealloc)},
        {0, nullptr}};
    static stable::Spec iterator_spec{"nrn._SegmentIterator",
                                      sizeof(SegmentIterator),
                                      0,
                                      static_cast<unsigned>(stable::tpflags_have_gc),
                                      iterator_slots};
    iterator_type = py.PyType_FromSpec(&iterator_spec);

    // No fields of its own: the interface's Vector derives from it and from its Object class.
    static Slot vector_slots[]{{stable::mp_subscript, reinterpret_cast<void*>(vector_getitem)},
                               {stable::mp_ass_subscript, reinterpret_cast<void*>(vector_setitem)},
                               {0, nullptr}};
    static stable::Spec vector_spec{
        "hoc._Vector",
        0,
        0,
        // The collector's flag and functions are inherited from the base.
        static_cast<unsigned>(stable::tpflags_basetype),
        vector_slots};
    types.vector = py.PyType_FromSpecWithBases(&vector_spec, types.object);
    return t.s_getattr && t.s_setattr && t.s_getitem && t.s_setitem && t.zero && t.rangevars &&
           t.mechanisms && t.misses && iterator_type && types.vector;
}

}  // namespace neuron::python::fast
