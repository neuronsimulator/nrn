#pragma once
#include "nrnpy_capi.h"

struct Object;
struct Section;

namespace neuron::python {

/// The start of every Python object: PyObject of a build with the GIL, which the stable ABI fixes.
struct Head {
    std::ptrdiff_t refcnt;
    PyObject* type;
};

inline PyObject* type_of(PyObject* o) {
    return reinterpret_cast<Head*>(o)->type;
}

inline bool is_instance(PyObject* o, PyObject* type) {
    return type && (type_of(o) == type || py.PyType_IsSubtype(type_of(o), type));
}

/// The base types of the interface's classes of HOC objects, sections and segments.
struct Types {
    PyObject* object{};
    PyObject* section{};
    PyObject* segment{};
};
extern Types types;
/// Make them. False with a Python error set on failure.
bool make_types();

struct ObjectSlot {
    enum { obj, class_name, methods, section_cache, plotshape_var, count };
};
struct SectionSlot {
    enum { sec, pynative, name, cell, inserted_mechs, inserted_mechs_epoch, count };
};
struct SegmentSlot {
    enum { sec, x, mech_cache, count };
};

/// Slot i of an instance of one of the types, borrowed. Null if it is not set.
PyObject* slot(PyObject* instance, int i);
/// What a wrapper stands for. Null for one whose construction failed.
Object* hoc_object(PyObject* wrapper);
Section* section(PyObject* wrapper);
/// Python's sections are owned by their objects, and HOC asks those for a section's name and cell.
void own_sections();
/// object_class gives other classes now.
void forget_classes();
/// The object of a section that Python made; null for one that HOC made.
Ref python_section(Section* sec);
/// A new wrapper, or for a section that Python made its object. Null with an error set on failure;
/// a HOC error if there is no interface.
Ref wrap(Object* o);
Ref wrap(Section* sec);
Ref wrap(Section* sec, double x);

}  // namespace neuron::python
