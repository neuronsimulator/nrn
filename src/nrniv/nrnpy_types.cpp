// The base types of the Python classes of HOC objects, sections and segments. They have the
// instances' slots, so that core reads and makes instances itself; the interface derives its
// classes from them (nrn_python_type) and registers those (nrn_python_hook_set).
#include <../../nrnconf.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <utility>

#include "neuronapi.h"
#include "nrn_ansi.h"
#include "ocjump.h"
#include "nrnpy_capi.h"
#include "nrnpy_object.h"
#include "nrnpy_types.h"
#include "ocfunc.h"
#include "section.h"

extern bool hoc_unref_is_deferred();

namespace neuron::python {
namespace {

template <int N>
struct Instance: Head {
    PyObject* slots[N];
};

template <int N>
int traverse(PyObject* self, int (*visit)(PyObject*, void*), void* arg) {
    int r = visit(type_of(self), arg);
    for (auto* const slot: reinterpret_cast<Instance<N>*>(self)->slots) {
        r = r || !slot ? r : visit(slot, arg);
    }
    return r;
}

/// The slots from first on are None.
template <int N, int first>
PyObject* make_instance(PyObject* type, PyObject*, PyObject*) {
    auto* const self = py.PyType_GenericAlloc(type, 0);
    for (int i = first; self && i < N; ++i) {
        reinterpret_cast<Instance<N>*>(self)->slots[i] = Ref::borrow(py._Py_NoneStruct).release();
    }
    return self;
}

/// Clear the slots from first on.
template <int N, int first>
int clear(PyObject* self) {
    auto* const slots = reinterpret_cast<Instance<N>*>(self)->slots;
    for (int i = first; i < N; ++i) {
        Ref{std::exchange(slots[i], nullptr)};
    }
    return 0;
}

// After a subclass's __del__, and when nothing can reach the object any more: no Python code gets
// self.
template <int N, void (*release)(PyObject*)>
void dealloc(PyObject* self) {
    auto* const type = type_of(self);
    py.PyObject_GC_UnTrack(self);
    release(self);
    clear<N, 0>(self);
    py.PyObject_GC_Del(self);
    py.Py_DecRef(type);
}

void release_nothing(PyObject*) {}

/// While HOC releases what an instance holds, an exception being handled waits.
class KeepError {
    PyObject *type_, *value_, *traceback_;

  public:
    KeepError() {
        py.PyErr_Fetch(&type_, &value_, &traceback_);
    }
    ~KeepError() {
        py.PyErr_Restore(type_, value_, traceback_);
    }
};

void release_object(PyObject* self) {
    // At exit Python is already gone for HOC: the object stays.
    if (auto* const o = hoc_object(self); o && py.Py_IsInitialized()) {
        if (o->refcount > 1 && !o->ctemplate->unref && !hoc_unref_is_deferred()) {
            --o->refcount;  // nothing happens
            return;
        }
        KeepError const keep{};
        nrn_object_unref(o);
    }
}

// A section that Python made goes with its object. A wrapper only refers to one that HOC made.
void release_section(PyObject* self) {
    Section* const sec = section(self);
    if (!sec || !py.Py_IsInitialized()) {
        return;
    }
    if (slot(self, SectionSlot::pynative) == py._Py_FalseStruct) {
        section_unref(sec);
        return;
    }
    KeepError const keep{};
    nrn_section_owned_release(sec);
}

void set(PyObject* self, int i, PyObject* value) {
    Ref{std::exchange(reinterpret_cast<Instance<1>*>(self)->slots[i],
                      Ref::borrow(value).release())};
}

/// The two arguments of a constructor.
bool arguments(PyObject* args,
               PyObject* keywords,
               std::array<const char*, 3> names,
               PyObject*& a,
               PyObject*& b) {
    return py.PyArg_ParseTupleAndKeywords(
        args, keywords, "OO", const_cast<char**>(names.data()), &a, &b);
}

// Segment(section, x)
int init_segment(PyObject* self, PyObject* args, PyObject* keywords) {
    PyObject *section, *x;
    if (!arguments(args, keywords, {"sec", "x", nullptr}, section, x)) {
        return -1;
    }
    set(self, SegmentSlot::sec, section);
    set(self, SegmentSlot::x, x);
    return 0;
}

// Section(name, cell): the name with the cell's, as bytes, or None; a weak reference to the cell,
// or None. Makes the section.
int init_section(PyObject* self, PyObject* args, PyObject* keywords) {
    PyObject *name, *cell;
    if (section(self)) {
        return 0;  // made before
    }
    if (!arguments(args, keywords, {"name", "cell", nullptr}, name, cell)) {
        return -1;
    }
    if (name != py._Py_NoneStruct && !is_instance(name, py.PyBytes_Type)) {
        py.PyErr_SetString(py.PyExc_TypeError, "a section's name is bytes or None");
        return -1;
    }
    set(self, SectionSlot::name, name);
    set(self, SectionSlot::cell, cell);
    Section* sec{};
    std::array<char, 1024> error;
    if (!nrn_nothrow([&] { sec = nrn_section_new_owned(self); }, error.data(), error.size()) ||
        !sec) {
        py.PyErr_SetString(py.PyExc_RuntimeError,
                           error[0] ? error.data() : "the section owner hooks are not Python's");
        return -1;
    }
    set(self, SectionSlot::sec, Ref{py.PyLong_FromVoidPtr(sec)}.get());
    set(self, SectionSlot::pynative, py._Py_TrueStruct);
    return 0;
}

// What HOC asks about sections. The object of one that Python made is the owner that core stores.

char* section_name(Section*);

/// The object of a section that Python made, or null. Not a reference: it may be in dealloc.
PyObject* owner(Section* sec) {
    return nrnpy_pysec_name_p_ == section_name ? static_cast<PyObject*>(nrn_section_owner_get(sec))
                                               : nullptr;
}

char* section_name(Section* sec) {
    static std::string known;
    known.clear();
    if (started()) {  // NEURON still names sections when Python has been finalized
        Gil const gil{};
        auto* const name = slot(owner(sec), SectionSlot::name);
        known = name && name != py._Py_NoneStruct ? py.PyBytes_AsString(name)
                                                  : fmt::format("__nrnsec_{}", fmt::ptr(sec));
    }
    return known.data();
}

/// The cell of a section, or None; null if Python did not make the section. The GIL is held.
Ref section_cell(Section* sec) {
    auto* const object = owner(sec);
    if (!object) {
        return {};
    }
    Ref const weak = Ref::borrow(slot(object, SectionSlot::cell));
    Ref cell = weak.get() == py._Py_NoneStruct ? Ref{} : call(weak.get());
    py.PyErr_Clear();
    return cell ? std::move(cell) : Ref::borrow(py._Py_NoneStruct);
}

Object* section_cell_object(Section* sec) {
    if (!started()) {
        return nullptr;
    }
    Gil const gil{};
    Ref const cell = section_cell(sec);
    return cell ? methods.po2ho(cell.get()) : nullptr;
}

int section_cell_equals(Section* sec, Object* o) {
    if (!started()) {
        return 0;
    }
    Gil const gil{};
    Ref const cell = section_cell(sec);
    if (!cell) {
        return 0;
    }
    if (is_instance(cell.get(), types.object)) {
        return hoc_object(cell.get()) == o;
    }
    return o ? methods.opaque_obj2pyobj(o) == cell.get() : cell.get() == py._Py_NoneStruct;
}

using stable::Member;
using stable::Slot;
using stable::Spec;

/// The slots from optional on are None at first, and the garbage collector clears them: the others
/// are not in a cycle without one of them, and say what dealloc releases.
template <int N, void (*release)(PyObject*), int optional>
PyObject* make_type(const char* name,
                    std::array<const char*, N> const& names,
                    std::array<Slot, 4> extra = {}) {
    static Member members[N + 1]{};
    auto* member = members;
    for (int i = 0; i < N; ++i) {
        // The interpreter reads a Py_T_OBJECT_EX member as a __slots__ entry.
        if (names[i]) {  // otherwise it is core's own
            *member++ = {names[i],
                         stable::t_object_ex,
                         static_cast<std::ptrdiff_t>(sizeof(Head) + i * sizeof(PyObject*)),
                         0,
                         nullptr};
        }
    }
    static Slot slots[]{{stable::tp_clear, reinterpret_cast<void*>(clear<N, optional>)},
                        {stable::tp_new, reinterpret_cast<void*>(make_instance<N, optional>)},
                        {stable::tp_dealloc, reinterpret_cast<void*>(dealloc<N, release>)},
                        {stable::tp_traverse, reinterpret_cast<void*>(traverse<N>)},
                        {stable::tp_members, members},
                        extra[0],
                        extra[1],
                        extra[2],
                        extra[3],
                        {0, nullptr}};
    // Unused extra slots are {0, nullptr}, which ends the list there.
    std::stable_partition(std::begin(slots), std::end(slots), [](Slot const& s) { return s.id; });
    static Spec spec{name,
                     sizeof(Instance<N>),
                     0,
                     static_cast<unsigned>(stable::tpflags_basetype | stable::tpflags_have_gc),
                     slots};
    return py.PyType_FromSpec(&spec);
}

template <int N>
Ref make(PyObject* type, std::array<PyObject*, N> const& values) {
    Ref instance{py.PyType_GenericAlloc(type, 0)};
    for (int i = 0; instance && i < N; ++i) {
        auto* const value = values[i] ? values[i] : py._Py_NoneStruct;
        reinterpret_cast<Instance<N>*>(instance.get())->slots[i] = Ref::borrow(value).release();
    }
    return instance;
}

struct Class {
    std::string name;
    PyObject *type{}, *name_object{};
};
std::unordered_map<cTemplate*, Class> classes;

}  // namespace

Types types;

bool make_types() {
    if (types.object) {
        return true;
    }
    int constexpr init = stable::tp_init;
    bool const fast = fast::enabled();
    Slot const none{};
    types.segment = make_type<SegmentSlot::count, release_nothing, SegmentSlot::mech_cache>(
        "nrn._Segment",
        {"_sec", "_x", "_mech_cache"},
        {{{init, reinterpret_cast<void*>(init_segment)},
          fast ? Slot{stable::tp_getattro, reinterpret_cast<void*>(fast::segment_getattro)} : none,
          fast ? Slot{stable::tp_setattro, reinterpret_cast<void*>(fast::segment_setattro)}
               : none}});
    types.section = make_type<SectionSlot::count, release_section, SectionSlot::inserted_mechs>(
        "nrn._Section",
        {"_sec", "_pynative", nullptr, "_cell", "_inserted_mechs", "_inserted_mechs_epoch"},
        {{{init, reinterpret_cast<void*>(init_section)},
          fast ? Slot{stable::tp_methods, fast::section_methods} : none}});
    types.object = make_type<ObjectSlot::count, release_object, ObjectSlot::methods>(
        "hoc._Object", {"_obj", "_class_name", "_methods", "_section_cache", "_plotshape_var"});
    if (!types.object || !types.section || !types.segment) {
        return false;
    }
    if (!fast) {  // the interface's classes are then as without the fast path
        return true;
    }
    types.mechanism = make_type<MechanismSlot::count, release_nothing, 0>(
        "nrn._Mechanism",
        {"_seg", "_x", "_name", "_is_ion", "_epoch", "_nseg", "_x_nseg"},
        {{{stable::tp_getattro, reinterpret_cast<void*>(fast::mechanism_getattro)},
          {stable::tp_setattro, reinterpret_cast<void*>(fast::mechanism_setattro)}}});
    return types.mechanism && fast::make_types();
}

PyObject* slot(PyObject* instance, int i) {
    return reinterpret_cast<Instance<1>*>(instance)->slots[i];
}

Object* hoc_object(PyObject* wrapper) {
    auto* const address = slot(wrapper, ObjectSlot::obj);
    return address ? static_cast<Object*>(py.PyLong_AsVoidPtr(address)) : nullptr;
}

Section* section(PyObject* wrapper) {
    auto* const address = slot(wrapper, SectionSlot::sec);
    return address ? static_cast<Section*>(py.PyLong_AsVoidPtr(address)) : nullptr;
}

void own_sections() {
    nrn_section_owner_hooks_set(section_name, section_cell_object, section_cell_equals, nullptr, 0);
}

void forget_classes() {
    for (auto const& [_, known]: std::exchange(classes, {})) {
        Ref const type{known.type}, name{known.name_object};
    }
}

Ref wrap(Object* o) {
    auto const* known = &classes[o->ctemplate];
    // KSChan renames its class.
    if (auto const* const name = o->ctemplate->sym->name; !known->type || known->name != name) {
        Ref class_name{py.PyUnicode_FromString(name)};
        Ref type = call(need(Hook::object_class).get(), class_name);
        if (!type) {
            return {};
        }
        if (!is_instance(type.get(), py.PyType_Type) ||
            !py.PyType_IsSubtype(type.get(), types.object)) {
            py.PyErr_SetString(py.PyExc_TypeError, "object_class returned no class of objects");
            return {};
        }
        // Not through known: importing neuron, or the hook, can register an object_class.
        known = &(classes[o->ctemplate] = {name, type.release(), class_name.release()});
    }
    Ref const address{py.PyLong_FromVoidPtr(o)};
    Ref wrapper = make<ObjectSlot::count>(known->type, {address.get(), known->name_object});
    if (wrapper) {
        hoc_obj_ref(o);
    }
    return wrapper;
}

Ref segment(PyObject* section, double x) {
    Ref const position{py.PyFloat_FromDouble(x)};
    return position ? make<SegmentSlot::count>(need(Hook::segment_class).get(),
                                               {section, position.get()})
                    : Ref{};
}

Ref python_section(Section* sec) {
    auto* const object = owner(sec);
    return object ? Ref::borrow(object) : Ref{};
}

Ref wrap(Section* sec) {
    if (Ref object = python_section(sec)) {
        return object;
    }
    Ref const address{py.PyLong_FromVoidPtr(sec)};
    Ref wrapper = make<SectionSlot::count>(need(Hook::section_class).get(),
                                           {address.get(), py._Py_FalseStruct});
    if (wrapper) {
        section_ref(sec);
    }
    return wrapper;
}

Ref wrap(Section* sec, double x) {
    Ref const section = wrap(sec);
    if (!section) {
        return {};
    }
    Ref const position{py.PyFloat_FromDouble(x)};
    return make<SegmentSlot::count>(need(Hook::segment_class).get(),
                                    {section.get(), position.get()});
}

}  // namespace neuron::python
