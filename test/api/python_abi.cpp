// libnrniv does not include Python.h: it copies what it needs of CPython's stable ABI
// (src/nrniv/nrnpy_capi.h). This checks that copy against the real headers of each Python the
// build knows, with the limited API of the oldest version NEURON supports. It fails to compile if
// they differ; running it checks nothing more.
#define Py_LIMITED_API 0x030B0000
#include <Python.h>
#include <structmember.h>

#include <cstddef>
#include <type_traits>
#include <utility>

#include "nrnpy_capi.h"
#include "nrnpy_types.h"

namespace stable = neuron::python::stable;

// Every function libnrniv looks up is declared in the limited API, with the table's return type,
// number of arguments and, for the result and each argument, kind (integer, floating point,
// pointer) and size. The table writes a few pointers as void* or PyObject* (PyType_Spec*,
// PyFrameObject*) and an enum as int (PyGILState_STATE), so the exact types are not compared.
// Unevaluated, so nothing links.
template <typename T>
constexpr int kind() {
    if constexpr (std::is_void_v<T>) {
        return 0;
    } else
        return (std::is_pointer_v<T>          ? 3
                : std::is_floating_point_v<T> ? 2
                                              : 1) *
                   100 +
               static_cast<int>(sizeof(T));
}
template <typename F>
struct Signature;
template <typename R, typename... A>
struct Signature<R (*)(A...)> {
    using result = R;
    using arguments = std::integer_sequence<int, kind<A>()...>;
    static constexpr bool variadic = false;
};
template <typename R, typename... A>
struct Signature<R (*)(A..., ...)> {
    using result = R;
    using arguments = std::integer_sequence<int, kind<A>()...>;
    static constexpr bool variadic = true;
};
template <typename Theirs, typename Ours>
constexpr bool same_signature =
    kind<typename Signature<Theirs>::result>() == kind<typename Signature<Ours>::result>() &&
    std::is_same_v<typename Signature<Theirs>::arguments,
                   typename Signature<Ours>::arguments>&& Signature<Theirs>::variadic
        == Signature<Ours>::variadic;
#define X(ret, name, args) static_assert(same_signature<decltype(&::name), ret(*) args>, #name);
NRNPY_FUNCTIONS(X)
#undef X
#define X(name) static_assert(sizeof(&::name) == sizeof(void*), #name);
NRNPY_TYPES(X)
#undef X
#define X(name) static_assert(sizeof(::name) == sizeof(PyObject*), #name);
NRNPY_EXCEPTIONS(X)
#undef X

static_assert(stable::tp_clear == Py_tp_clear && stable::tp_dealloc == Py_tp_dealloc &&
              stable::tp_init == Py_tp_init && stable::tp_new == Py_tp_new &&
              stable::tp_traverse == Py_tp_traverse && stable::tp_members == Py_tp_members);
static_assert(stable::tp_getattro == Py_tp_getattro && stable::tp_setattro == Py_tp_setattro &&
              stable::tp_iter == Py_tp_iter && stable::tp_iternext == Py_tp_iternext &&
              stable::tp_methods == Py_tp_methods && stable::mp_subscript == Py_mp_subscript &&
              stable::mp_ass_subscript == Py_mp_ass_subscript && stable::sq_item == Py_sq_item &&
              stable::meth_noargs == METH_NOARGS);
static_assert(stable::tpflags_disallow_instantiation == Py_TPFLAGS_DISALLOW_INSTANTIATION);
static_assert(stable::t_object_ex == T_OBJECT_EX);
static_assert(stable::tpflags_basetype == Py_TPFLAGS_BASETYPE &&
              stable::tpflags_have_gc == Py_TPFLAGS_HAVE_GC &&
              stable::tpflags_bytes_subclass == Py_TPFLAGS_BYTES_SUBCLASS &&
              stable::tpflags_unicode_subclass == Py_TPFLAGS_UNICODE_SUBCLASS);
static_assert(stable::buf_write == PyBUF_WRITE && stable::file_input == Py_file_input);

#define SAME(ours, field, theirs, their_field)                              \
    static_assert(offsetof(ours, field) == offsetof(theirs, their_field) && \
                      sizeof(ours::field) == sizeof(theirs::their_field),   \
                  #ours "::" #field)
static_assert(sizeof(stable::Member) == sizeof(PyMemberDef));
SAME(stable::Member, name, PyMemberDef, name);
SAME(stable::Member, type, PyMemberDef, type);
SAME(stable::Member, offset, PyMemberDef, offset);
SAME(stable::Member, flags, PyMemberDef, flags);
SAME(stable::Member, doc, PyMemberDef, doc);
static_assert(sizeof(stable::Slot) == sizeof(PyType_Slot));
SAME(stable::Slot, id, PyType_Slot, slot);
SAME(stable::Slot, function, PyType_Slot, pfunc);
static_assert(sizeof(stable::Method) == sizeof(PyMethodDef));
SAME(stable::Method, name, PyMethodDef, ml_name);
SAME(stable::Method, function, PyMethodDef, ml_meth);
SAME(stable::Method, flags, PyMethodDef, ml_flags);
SAME(stable::Method, doc, PyMethodDef, ml_doc);
static_assert(sizeof(stable::Spec) == sizeof(PyType_Spec));
SAME(stable::Spec, name, PyType_Spec, name);
SAME(stable::Spec, basicsize, PyType_Spec, basicsize);
SAME(stable::Spec, itemsize, PyType_Spec, itemsize);
SAME(stable::Spec, flags, PyType_Spec, flags);
SAME(stable::Spec, slots, PyType_Spec, slots);
// The start of every object, for a build with the GIL (nrnpy_types.h). Since 3.14 the reference
// count is part of a union, but it still takes the first word, which libnrniv never reads.
static_assert(sizeof(neuron::python::Head) == sizeof(PyObject));
static_assert(offsetof(neuron::python::Head, refcnt) == 0 &&
              offsetof(PyObject, ob_type) == sizeof(std::ptrdiff_t));
SAME(neuron::python::Head, type, PyObject, ob_type);
#undef SAME

int main() {
    return 0;
}
