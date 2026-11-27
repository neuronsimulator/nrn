#pragma once
// The CPython functions libnrniv calls. Python.h is not included and libpython is not linked when
// Python is dynamic: the functions are looked up in the running interpreter. All of them are in the
// stable ABI, so one libnrniv serves every CPython 3.11 and later, with any compiler.
#include <cstddef>
#include <string>
#include <utility>

#include "nrnpy.h"

namespace neuron::python {

// clang-format off
#define NRNPY_FUNCTIONS(X)                                                        \
    X(int, Py_IsInitialized, (void))                                              \
    X(void, Py_Initialize, (void))                                                \
    X(int, Py_FinalizeEx, (void))                                                 \
    X(int, Py_AtExit, (void (*)()))                                               \
    X(void, Py_SetProgramName, (const wchar_t*))                                  \
    X(wchar_t*, Py_DecodeLocale, (const char*, std::size_t*))                     \
    X(int, PyGILState_Ensure, (void))                                             \
    X(void, PyGILState_Release, (int))                                            \
    X(PyThreadState*, PyEval_SaveThread, (void))                                  \
    X(void, PyEval_RestoreThread, (PyThreadState*))                               \
    X(PyObject*, PyEval_GetFrame, (void))                                         \
    X(PyObject*, Py_CompileString, (const char*, const char*, int))               \
    X(PyObject*, PyEval_EvalCode, (PyObject*, PyObject*, PyObject*))              \
    X(PyObject*, PyEval_GetBuiltins, (void))                                      \
    X(PyObject*, PyImport_AddModule, (const char*))                               \
    X(PyObject*, PyImport_ImportModule, (const char*))                            \
    X(PyObject*, PyModule_GetDict, (PyObject*))                                   \
    X(void, Py_IncRef, (PyObject*))                                               \
    X(void, Py_DecRef, (PyObject*))                                               \
    X(void, PyErr_Fetch, (PyObject**, PyObject**, PyObject**))                    \
    X(void, PyErr_NormalizeException, (PyObject**, PyObject**, PyObject**))       \
    X(void, PyErr_Restore, (PyObject*, PyObject*, PyObject*))                     \
    X(int, PyErr_GivenExceptionMatches, (PyObject*, PyObject*))                   \
    X(PyObject*, PyErr_Occurred, (void))                                          \
    X(void, PyErr_SetString, (PyObject*, const char*))                            \
    X(void, PyErr_SetObject, (PyObject*, PyObject*))                              \
    X(void, PyErr_Print, (void))                                                  \
    X(void, PyErr_WriteUnraisable, (PyObject*))                                   \
    X(void, PyErr_Clear, (void))                                                  \
    X(int, PyException_SetTraceback, (PyObject*, PyObject*))                      \
    X(void, PyException_SetCause, (PyObject*, PyObject*))                         \
    X(PyObject*, PyObject_CallObject, (PyObject*, PyObject*))                     \
    X(PyObject*, PyObject_CallFunctionObjArgs, (PyObject*, ...))                  \
    X(PyObject*, PyObject_GetAttr, (PyObject*, PyObject*))                        \
    X(int, PyObject_SetAttr, (PyObject*, PyObject*, PyObject*))                   \
    X(PyObject*, PyObject_GetAttrString, (PyObject*, const char*))                \
    X(int, PyObject_SetAttrString, (PyObject*, const char*, PyObject*))           \
    X(int, PyObject_HasAttrString, (PyObject*, const char*))                      \
    X(PyObject*, PyObject_GetItem, (PyObject*, PyObject*))                        \
    X(int, PyObject_SetItem, (PyObject*, PyObject*, PyObject*))                   \
    X(PyObject*, PyObject_GetIter, (PyObject*))                                   \
    X(PyObject*, PyObject_Str, (PyObject*))                                       \
    X(int, PyType_IsSubtype, (PyObject*, PyObject*))                              \
    X(unsigned long, PyType_GetFlags, (PyObject*))                                \
    X(PyObject*, PyType_FromSpec, (void*))                                        \
    X(PyObject*, PyType_GenericAlloc, (PyObject*, std::ptrdiff_t))                \
    X(void, PyObject_GC_UnTrack, (void*))                                         \
    X(void, PyObject_GC_Del, (void*))                                             \
    X(int, PyCallable_Check, (PyObject*))                                         \
    X(PyObject*, PyIter_Next, (PyObject*))                                        \
    X(int, PyNumber_Check, (PyObject*))                                           \
    X(PyObject*, PyNumber_Float, (PyObject*))                                     \
    X(int, PySequence_Check, (PyObject*))                                         \
    X(int, PyMapping_Check, (PyObject*))                                          \
    X(std::ptrdiff_t, PySequence_Size, (PyObject*))                               \
    X(PyObject*, PySequence_GetItem, (PyObject*, std::ptrdiff_t))                 \
    X(int, PySequence_SetItem, (PyObject*, std::ptrdiff_t, PyObject*))            \
    X(PyObject*, PySequence_List, (PyObject*))                                    \
    X(PyObject*, PyFloat_FromDouble, (double))                                    \
    X(double, PyFloat_AsDouble, (PyObject*))                                      \
    X(PyObject*, PyLong_FromLongLong, (long long))                                \
    X(PyObject*, PyLong_FromDouble, (double))                                     \
    X(long, PyLong_AsLong, (PyObject*))                                           \
    X(PyObject*, PyLong_FromVoidPtr, (void*))                                     \
    X(void*, PyLong_AsVoidPtr, (PyObject*))                                       \
    X(PyObject*, PyUnicode_FromString, (const char*))                             \
    X(PyObject*, PyUnicode_InternFromString, (const char*))                       \
    X(PyObject*, PyUnicode_DecodeFSDefault, (const char*))                        \
    X(PyObject*, PyUnicode_DecodeLocale, (const char*, const char*))              \
    X(PyObject*, PyUnicode_AsUTF8String, (PyObject*))                             \
    X(PyObject*, PyUnicode_Join, (PyObject*, PyObject*))                          \
    X(PyObject*, PyBytes_FromStringAndSize, (const char*, std::ptrdiff_t))        \
    X(PyObject*, PyBytes_FromObject, (PyObject*))                                 \
    X(char*, PyBytes_AsString, (PyObject*))                                       \
    X(int, PyBytes_AsStringAndSize, (PyObject*, char**, std::ptrdiff_t*))         \
    X(PyObject*, PyByteArray_FromStringAndSize, (const char*, std::ptrdiff_t))    \
    X(PyObject*, PyMemoryView_FromMemory, (char*, std::ptrdiff_t, int))           \
    X(int, PyArg_ParseTupleAndKeywords,                                           \
      (PyObject*, PyObject*, const char*, char**, ...))                           \
    X(PyObject*, PyTuple_New, (std::ptrdiff_t))                                   \
    X(PyObject*, PyTuple_GetItem, (PyObject*, std::ptrdiff_t))                    \
    X(int, PyTuple_SetItem, (PyObject*, std::ptrdiff_t, PyObject*))               \
    X(PyObject*, PyList_New, (std::ptrdiff_t))                                    \
    X(std::ptrdiff_t, PyList_Size, (PyObject*))                                   \
    X(PyObject*, PyList_GetItem, (PyObject*, std::ptrdiff_t))                     \
    X(int, PyList_SetItem, (PyObject*, std::ptrdiff_t, PyObject*))                \
    X(int, PyList_Append, (PyObject*, PyObject*))                                 \
    X(PyObject*, PyDict_New, (void))                                              \
    X(PyObject*, PyDict_GetItem, (PyObject*, PyObject*))                          \
    X(PyObject*, PyDict_GetItemString, (PyObject*, const char*))                  \
    X(int, PyDict_SetItemString, (PyObject*, const char*, PyObject*))            \
    X(int, PyDict_SetItem, (PyObject*, PyObject*, PyObject*))                     \
    X(PyObject*, PyObject_GenericGetAttr, (PyObject*, PyObject*))                 \
    X(int, PyObject_GenericSetAttr, (PyObject*, PyObject*, PyObject*))            \
    X(PyObject*, PyObject_CallMethodObjArgs, (PyObject*, PyObject*, ...))         \
    X(int, PyErr_ExceptionMatches, (PyObject*))                                   \
    X(double, PyLong_AsDouble, (PyObject*))                                       \
    X(std::ptrdiff_t, PyLong_AsSsize_t, (PyObject*))                              \
    X(PyObject*, PyType_FromSpecWithBases, (void*, PyObject*))                    \
    X(const char*, PyUnicode_AsUTF8AndSize, (PyObject*, std::ptrdiff_t*))         \
    X(void, PyDict_Clear, (PyObject*))                                            \
    X(void*, PyType_GetSlot, (PyObject*, int))

// Types and exceptions, data in the stable ABI. PyExc_* are pointers to the objects.
#define NRNPY_TYPES(X)                                                            \
    X(_Py_NoneStruct) X(_Py_FalseStruct) X(_Py_TrueStruct) X(PyUnicode_Type)      \
    X(PyBytes_Type) X(PyTuple_Type) X(PyList_Type) X(PyType_Type) X(PyFloat_Type)      \
    X(PyLong_Type) X(PyDict_Type)
#define NRNPY_EXCEPTIONS(X)                                                       \
    X(PyExc_AttributeError) X(PyExc_Exception) X(PyExc_RuntimeError)              \
    X(PyExc_SystemExit) X(PyExc_TypeError) X(PyExc_ValueError)
// clang-format on

struct Capi {
#define X(ret, name, args) ret(*name) args{};
    NRNPY_FUNCTIONS(X)
#undef X
#define X(name) PyObject* name{};
    NRNPY_TYPES(X)
    NRNPY_EXCEPTIONS(X)
#undef X
    /// Fill the table from the libpython that library is a dlopen handle of or, if it is null, from
    /// the one this process has already loaded. False, with a message in missing, if there is none.
    bool bind(void* library, std::string& missing);
    /// Data of the interpreter that is not in the stable ABI. Null if there is none.
    void* optional(const char* name) const;

  private:
    void* library_{};
};
extern Capi py;

/// What else libnrniv copies of the stable ABI. test/api/python_abi.cpp checks it against Python.h.
namespace stable {
struct Member {  // PyMemberDef
    const char* name;
    int type;
    std::ptrdiff_t offset;
    int flags;
    const char* doc;
};
struct Slot {  // PyType_Slot
    int id;
    void* function;
};
struct Method {  // PyMethodDef
    const char* name;
    void* function;
    int flags;
    const char* doc;
};
struct Spec {  // PyType_Spec
    const char* name;
    int basicsize;
    int itemsize;
    unsigned int flags;
    Slot* slots;
};
// Py_tp_*
constexpr int tp_clear = 51, tp_dealloc = 52, tp_init = 60, tp_new = 65, tp_traverse = 71,
              tp_members = 72;
// The fast path (nrnpy_fast.cpp): Py_tp_*, Py_mp_*, METH_*
constexpr int tp_getattro = 58, tp_setattro = 69, tp_iter = 62, tp_iternext = 63, tp_methods = 64,
              mp_subscript = 5, mp_ass_subscript = 3, sq_item = 44;
constexpr int meth_noargs = 0x0004;
constexpr int t_object_ex = 16;  // Py_T_OBJECT_EX
// Py_TPFLAGS_*
constexpr unsigned long tpflags_disallow_instantiation = 1ul << 7,  // 3.10+
    tpflags_basetype = 1ul << 10, tpflags_have_gc = 1ul << 14, tpflags_bytes_subclass = 1ul << 27,
                        tpflags_unicode_subclass = 1ul << 28;
constexpr int buf_write = 0x200;  // PyBUF_WRITE
constexpr int file_input = 257;   // Py_file_input
}  // namespace stable
using stable::file_input;

/// Starts Python and binds py. The launcher sets it when NEURON, not Python, is the main program.
extern bool (*launch)();
/// Start Python or find the running one, the first time.
bool start_first();
extern bool bound;
/// Is Python running, or can it be started now? The GIL need not be held. Once Python has been
/// finalized the answer is false.
inline bool start() {
    return bound ? py.Py_IsInitialized() : start_first();
}
/// Is Python running? Does not start it.
inline bool started() {
    return bound && py.Py_IsInitialized();
}
/// HOC's quit(): finalize Python, in the thread that brought NEURON and Python together. Python
/// waits for the others.
void finalize();
/// start(), or a HOC error.
void require();

/// Hold the GIL. Works in any thread.
class Gil {
    int state_{py.PyGILState_Ensure()};

  public:
    Gil() = default;
    Gil(const Gil&) = delete;
    Gil& operator=(const Gil&) = delete;
    ~Gil() {
        py.PyGILState_Release(state_);
    }
};

/// An owned reference, or null. The GIL must be held.
class Ref {
    PyObject* p_{};

  public:
    Ref() = default;
    /// Takes over a new reference.
    explicit Ref(PyObject* p)
        : p_{p} {}
    static Ref borrow(PyObject* p) {
        py.Py_IncRef(p);
        return Ref{p};
    }
    Ref(Ref&& other) noexcept
        : p_{std::exchange(other.p_, nullptr)} {}
    Ref& operator=(Ref&& other) noexcept {
        std::swap(p_, other.p_);
        return *this;
    }
    ~Ref() {
        if (p_) {
            py.Py_DecRef(p_);
        }
    }
    PyObject* get() const {
        return p_;
    }
    PyObject* release() {
        return std::exchange(p_, nullptr);
    }
    explicit operator bool() const {
        return p_;
    }
};

template <typename... Args>
Ref call(PyObject* f, Args const&... args) {
    return Ref{py.PyObject_CallFunctionObjArgs(f, args.get()..., nullptr)};
}

/// vars(__main__), borrowed. Null with an error set on failure.
PyObject* main_namespace();
/// Run statements in __main__, as nrnpython() does. Null with an error set on failure.
Ref run(const char* statements);
/// The Python error is set: make it a HOC error. If HOC was not called from Python the error is
/// printed, and SystemExit ends the process; otherwise the Python caller gets the exception.
[[noreturn]] void hoc_error_from_python(const std::string& message);

}  // namespace neuron::python
