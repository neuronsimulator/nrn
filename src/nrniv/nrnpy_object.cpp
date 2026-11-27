// PythonObject, and everything else in HOC and the simulator that calls Python: callbacks,
// locations given as Python objects, Vector.from_python, SectionList of Python sections, pickled
// jobs.
//
// Python is called directly, through the stable ABI (nrnpy_capi.h). The classes of sections,
// segments and HOC objects derive from types made here (nrnpy_types.h), and the interface registers
// them with nrn_python_hook_set. A Python exception comes back here as the error indicator, so
// nothing is thrown through a Python frame; it becomes a HOC error, and the Python code that called
// HOC, if any, can still get the exception itself (nrn_python_error_take).
#include <../../nrnconf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <InterViews/resource.h>
#include "classreg.h"
#include "ivocvect.h"
#include "neuronapi.h"
#include "hocstr.h"
#include "nrnmpi.h"
#include "nrnoc2iv.h"
#include "nrnpy_capi.h"
#include "nrnpy_object.h"
#include "nrnpy_types.h"
#include "ocjump.h"
#include "ocpointer.h"
#include "oc_ansi.h"
#include "parse.hpp"
#include "seclist.h"
#include "section.h"

extern Object** hoc_temp_objptr(Object*);
extern "C" NRN_EXPORT int nrnpy_set_gui_callback(PyObject*);
extern void oc_restore_hoc_oop(Object**, Objectdata**, int*, Symlist**);
extern void oc_save_hoc_oop(Object**, Objectdata**, int*, Symlist**);

namespace neuron::python {
namespace {

// ---- what the interface registers

PyObject* hooks[static_cast<int>(Hook::count)];

PyObject*& registered(Hook hook) {
    return hooks[static_cast<int>(hook)];
}

// ---- errors

thread_local ErrorScope* innermost_scope{};
// The exception of the last HOC error that arrived in an ErrorScope, until it is taken.
thread_local PyObject* pending_exception{};

/// A null-safe text of a Python object.
std::string text(PyObject* o) {
    Ref const str{o ? py.PyObject_Str(o) : nullptr};
    Ref const bytes{str ? py.PyUnicode_AsUTF8String(str.get()) : nullptr};
    char* chars{};
    if (!bytes || py.PyBytes_AsStringAndSize(bytes.get(), &chars, nullptr)) {
        py.PyErr_Clear();
        return {};
    }
    return chars;
}

/// The Python error, taken from the indicator.
struct Error {
    Ref type, value, traceback;
    Error() {
        PyObject *t{}, *v{}, *tb{};
        py.PyErr_Fetch(&t, &v, &tb);
        py.PyErr_NormalizeException(&t, &v, &tb);
        if (tb) {
            py.PyException_SetTraceback(v, tb);
        }
        type = Ref{t};
        value = Ref{v};
        traceback = Ref{tb};
    }
    bool is(PyObject* exception) const {
        return type && py.PyErr_GivenExceptionMatches(type.get(), exception);
    }
    /// As sys.excepthook. SystemExit ends the process.
    void print() {
        py.PyErr_Restore(type.release(), value.release(), traceback.release());
        py.PyErr_Print();
    }
    std::string formatted() const {
        Ref const module{py.PyImport_ImportModule("traceback")};
        Ref const format{module ? py.PyObject_GetAttrString(module.get(), "format_exception")
                                : nullptr};
        Ref const none = Ref::borrow(py._Py_NoneStruct);
        Ref const lines = format ? call(format.get(), type, value, traceback ? traceback : none)
                                 : Ref{};
        Ref const empty{py.PyUnicode_FromString("")};
        Ref const joined{lines ? py.PyUnicode_Join(empty.get(), lines.get()) : nullptr};
        return text(joined.get());
    }
};

/// Takes over a reference, or null.
void print_exception(PyObject* exception) {
    if (exception && started()) {
        Gil const gil{};
        Ref const value{exception};
        py.PyErr_SetObject(type_of(exception), exception);
        Error{}.print();
    }
}

/// What the Python code that called HOC, if any, gets: the exception itself, or the HOC error it
/// became, as NEURON's Python module raised it: a RuntimeError of the message, without the
/// exception as its cause (always: also for SystemExit).
enum class Raise { exception, hoc_error, always };

/// Make a Python error a HOC error. If no Python code can get the exception it is printed (unless
/// the message tells it), and SystemExit ends the process.
[[noreturn]] void fail(Error error, const std::string& message, Raise what, bool print = true) {
    bool const exit = error.is(py.PyExc_SystemExit);
    auto* const scope = ErrorScope::innermost();
    if (scope && scope->can_be_taken && py.PyEval_GetFrame()) {
        if (what == Raise::always || (what == Raise::hoc_error && !exit)) {
            if (print) {
                Fprintf(stderr, "%s", error.formatted().c_str());
            }
            Ref const full{
                py.PyUnicode_FromString(("hocobj_call error: hoc_execerror: " + message).c_str())};
            error.value = call(py.PyExc_RuntimeError, full);
        }
        scope->keep(error.value.release());
    } else if (print || (exit && what != Raise::always)) {
        error.print();
    }
    hoc_execerror(message.c_str(), nullptr);
}

/// The Python error as NEURON's Python module threw it (nanobind's python_error): a C++ exception
/// "Python exception". A method's caller makes it the HOC error "Class[i]::method: Python
/// exception", and the exception is lost. If keep, the Python code that called the HOC function
/// raises the exception itself, as NEURON's module restored it. Only C++ frames are between here
/// and that caller.
[[noreturn]] void throw_python_error(bool keep = false) {
    {
        Error error{};
        auto* const scope = ErrorScope::innermost();
        if (keep && scope && scope->can_be_taken && py.PyEval_GetFrame()) {
            scope->keep(error.value.release());
        }
    }
    throw std::runtime_error("Python exception");
}

/// A failed call of Python, as NEURON's Python module reported it: an Exception is printed with
/// its traceback (neuron.format_exception), anything else as PyErr_Print does (SystemExit ends the
/// process); then the HOC error message.
[[noreturn]] void hoc_error_traceback(const std::string& message, Error error = {}) {
    {
        if (error.is(py.PyExc_Exception)) {
            Fprintf(stderr, "%s\n", error.formatted().c_str());
        } else {
            error.print();
        }
    }
    hoc_execerror(message.c_str(), nullptr);
}

/// PyErr_Print, then the HOC error message, as NEURON's Python module did.
[[noreturn]] void hoc_error_printed(const std::string& message) {
    Error{}.print();
    hoc_execerror(message.c_str(), nullptr);
}

[[noreturn]] void refuse(PyObject* exception, const char* message) {
    py.PyErr_SetString(exception, message);
    fail(Error{}, message, Raise::exception, false);
}

/// A hook failed. Its exceptions are for the user, so the text is enough.
[[noreturn]] void fail_hook() {
    Error error{};
    auto message = text(error.value.get());
    if (message.empty()) {
        message = text(error.type.get());
    }
    fail(std::move(error), message, Raise::hoc_error, false);
}

/// A callback failed. KeyboardInterrupt and the like do not stop NEURON: they are printed.
void fail_callback(const char* message, bool message_has_traceback = false) {
    Error error{};
    if (!error.is(py.PyExc_Exception) && !error.is(py.PyExc_SystemExit)) {
        error.print();
        return;
    }
    if (message_has_traceback) {
        auto const full = std::string{message} + "\n" + error.formatted();
        fail(std::move(error), full, Raise::hoc_error, false);
    }
    fail(std::move(error), message, Raise::hoc_error);
}

// ---- values

/// Entering Python: hold the GIL, with HOC at its top level (Python code that a template's method
/// runs calls HOC as any other), and what the interface keeps about the model is out of date (HOC
/// may have edited it since Python ran).
class Top {
    Gil gil_;
    Object* object_{hoc_thisobject};
    int exceptions_;
    Objectdata* data_;
    Symlist* symbols_;
    int local_;

  public:
    Top() {
        model_changed();
        if (object_) {
            exceptions_ = std::uncaught_exceptions();
            oc_save_hoc_oop(&object_, &data_, &local_, &symbols_);
            hoc_thisobject = nullptr;
            hoc_objectdata = hoc_top_level_data;
            hoc_symlist = hoc_top_level_symlist;
        }
    }
    Top(const Top&) = delete;
    Top& operator=(const Top&) = delete;
    ~Top() {
        // A HOC error has put HOC where it must be.
        if (object_ && std::uncaught_exceptions() == exceptions_) {
            oc_restore_hoc_oop(&object_, &data_, &local_, &symbols_);
        }
    }
};

/// Python code can drop the last other reference to an object.
class Hold {
    Object* o_;

  public:
    explicit Hold(Object* o)
        : o_{o} {
        hoc_obj_ref(o);
    }
    Hold(const Hold&) = delete;
    ~Hold() {
        hoc_obj_unref(o_);
    }
};

bool is_python_object(Object* o) {
    return o && o->ctemplate->sym == nrnpy_pyobj_sym_;
}

/// A segment that HOC made and Python has not seen yet: there is no object for it.
struct Location {
    Section* sec;
    double x;
    Ref python_section;

    Location(Section* sec, double x)
        : sec{sec}
        , x{x}
        , python_section{neuron::python::python_section(sec)} {
        section_ref(sec);
    }
    ~Location() {
        section_unref(sec);
    }
};

/// A PythonObject's this_pointer is a PyObject*, or a Location* with the lowest bit set.
Location* location(void* p) {
    auto const bits = reinterpret_cast<std::uintptr_t>(p);
    return bits & 1 ? reinterpret_cast<Location*>(bits - 1) : nullptr;
}

/// Null with an error set on failure.
Ref held_by(Object* o) {
    auto*& p = o->u.this_pointer;
    if (auto* const l = location(p)) {
        Ref segment = wrap(l->sec, l->x);
        if (!segment) {
            return {};
        }
        p = segment.release();
        delete l;
    }
    auto* const object = static_cast<PyObject*>(p);
    return Ref::borrow(object ? object : py.PyImport_AddModule("__main__"));
}

/// What a PythonObject holds. `new PythonObject()` in HOC stands for __main__.
Ref payload(Object* o) {
    Ref held = held_by(o);
    if (!held) {
        fail_hook();
    }
    return held;
}

/// Start Python for an object argument that must be a PythonObject.
void require_python_object(Object* o, const char* message) {
    if (!is_python_object(o)) {
        hoc_execerror(message, nullptr);
    }
    require();
}

/// The Python value of a HOC object.
Ref to_python(Object* o) {
    if (!o) {
        return Ref::borrow(py._Py_NoneStruct);
    }
    if (is_python_object(o)) {
        return payload(o);
    }
    Ref value = wrap(o);
    if (!value) {
        fail_hook();
    }
    return value;
}

/// A PythonObject with one reference.
Object* new_python_object_of(void* p) {
    Object* const o = hoc_new_object(nrnpy_pyobj_sym_, p);
    hoc_obj_ref(o);
    return o;
}

Object* new_python_object(PyObject* p) {
    py.Py_IncRef(p);
    return new_python_object_of(p);
}

/// The HOC object for a Python value, with a reference for the caller; null for None.
Object* to_hoc(PyObject* p) {
    if (p == py._Py_NoneStruct) {
        return nullptr;
    }
    if (is_instance(p, types.object)) {
        auto* const o = hoc_object(p);
        if (o) {
            hoc_obj_ref(o);
        }
        return o;
    }
    return new_python_object(p);
}

/// The same, with the reference given up, for a caller that passes it to hoc_temp_objptr.
Object* to_hoc_temp(PyObject* p) {
    Object* const o = to_hoc(p);
    if (o) {
        --o->refcount;
    }
    return o;
}

/// Does HOC take p as a number? Not a sequence, nor what float() refuses (complex).
bool is_number(PyObject* p, double& value) {
    if (!py.PyNumber_Check(p) || py.PySequence_Check(p)) {
        return false;
    }
    value = py.PyFloat_AsDouble(p);
    if (value == -1. && py.PyErr_Occurred()) {
        py.PyErr_Clear();
        return false;
    }
    return true;
}

/// A HOC string, if p is a str or bytes. The slots outlive the expression that uses the string, as
/// hoc_temp_charptr's do.
char** temp_string(PyObject* p) {
    unsigned long constexpr is_bytes = stable::tpflags_bytes_subclass,
                            is_str = stable::tpflags_unicode_subclass;
    auto const flags = py.PyType_GetFlags(type_of(p));
    if (!(flags & (is_bytes | is_str))) {
        return nullptr;
    }
    Ref const bytes = flags & is_str ? Ref{py.PyUnicode_AsUTF8String(p)} : Ref::borrow(p);
    if (!bytes) {
        fail(Error{}, "not a HOC string", Raise::exception);
    }
    static std::array<std::string, 128> slots;
    static std::size_t next{};
    auto& slot = slots[next++ % slots.size()];
    slot = py.PyBytes_AsString(bytes.get());  // up to the first NUL
    char** const temp = hoc_temp_charptr();
    *temp = slot.data();
    return temp;
}

/// Push the result of a call or an attribute on the HOC stack.
void push(PyObject* p) {
    if (double value; is_number(p, value)) {
        hoc_pop_defer();
        hoc_pushx(value);
        return;
    }
    if (char** const string = temp_string(p)) {
        hoc_pop_defer();
        hoc_pushstr(string);
        return;
    }
    Object* const o = to_hoc(p);
    hoc_pop_defer();
    hoc_push_object(o);
    if (o) {
        --o->refcount;
    }
}

/// Pop a value from the HOC stack.
Ref pop() {
    switch (hoc_stack_type()) {
    case STRING:
        if (Ref s{py.PyUnicode_FromString(*hoc_strpop())}) {
            return s;
        }
        fail(Error{}, "not a Python string", Raise::exception);
    case VAR: {
        auto const handle = hoc_pop_handle<double>();
        if (!handle) {
            hoc_execerror("Invalid pointer argument", nullptr);
        }
        return Ref{py.PyFloat_FromDouble(*handle)};
    }
    case NUMBER:
        return Ref{py.PyFloat_FromDouble(hoc_xpop())};
    case OBJECTVAR:
    case OBJECTTMP: {
        Object** const o = hoc_objpop();
        std::unique_ptr<Object*, void (*)(Object**)> const unref{o, hoc_tobj_unref};
        return to_python(*o);
    }
    default:
        hoc_execerror("This type of argument can not be passed to Python", nullptr);
    }
}

/// Pop n arguments, the last one first, into a tuple.
Ref pop_tuple(int n) {
    Ref args{py.PyTuple_New(n)};
    while (n--) {
        py.PyTuple_SetItem(args.get(), n, pop().release());
    }
    return args;
}

template <typename... Args>
Ref make_tuple(Args&&... args) {
    Ref tuple{py.PyTuple_New(sizeof...(args))};
    int i{};
    (py.PyTuple_SetItem(tuple.get(), i++, args.release()), ...);
    return tuple;
}

// ---- PythonObject

void* construct(Object*) {
    return nullptr;
}

void destruct(void* p) {
    if (p && start()) {
        Top const top{};
        if (auto* const l = location(p)) {
            delete l;
        } else {
            py.Py_DecRef(static_cast<PyObject*>(p));
        }
    }
}

/// The name of a component, as Python keeps names. Never released.
PyObject* interned(const Symbol* sym) {
    struct Name {
        std::string name;
        PyObject* object{};
    };
    static std::unordered_map<const Symbol*, Name> names;
    auto& known = names[sym];
    if (!known.object || known.name != sym->name) {
        known = {sym->name, py.PyUnicode_InternFromString(sym->name)};
    }
    return known.object;
}

/// The attribute that HOC writes as o.name. Null with an error set if there is none.
Ref attribute(Object* o, const Symbol* sym) {
    auto* const name = interned(sym);
    if (!o->u.this_pointer) {
        auto* const vars = main_namespace();
        auto* value = vars ? py.PyDict_GetItem(vars, name) : nullptr;
        if (!value) {
            value = py.PyDict_GetItem(py.PyEval_GetBuiltins(), name);
        }
        if (value) {
            return Ref::borrow(value);
        }
        return Ref{py.PyObject_GetAttr(py.PyImport_AddModule("__main__"), name)};
    }
    if (std::strcmp(sym->name, "_") == 0) {
        return payload(o);
    }
    return Ref{py.PyObject_GetAttr(payload(o).get(), name)};
}

void check_dimension(int n) {
    if (n > 1) {
        hoc_execerr_ext(
            "%d dimensional python objects "
            "can't be accessed from hoc with var._[i1][i2]... syntax. "
            "Must use var._[i1]._[i2]... hoc syntax.",
            n);
    }
}

const char* component(Object* o, Symbol* sym, int nindex, int isfunc) {
    require();
    Top const top{};
    Ref const value = attribute(o, sym);
    if (!value) {
        hoc_error_printed(std::string{"No attribute: "} + sym->name);
    }
    Ref result;
    if (isfunc) {
        Ref const args = pop_tuple(nindex);
        result = Ref{py.PyObject_CallObject(value.get(), args.get())};
        if (!result) {
            Error error{};
            if (!error.is(py.PyExc_Exception)) {
                // As NEURON's Python module: printed, and nothing is pushed, which HOC then
                // reports as its own error.
                error.print();
                return nullptr;
            }
            hoc_error_traceback(std::string{"PyObject method call failed: "} + sym->name,
                                std::move(error));
        }
    } else if (nindex) {
        Hold const hold{o};
        check_dimension(hoc_pop_ndim());
        // HOC numbers are doubles; as an index they are truncated.
        Ref const key = hoc_stack_type() == NUMBER ? Ref{py.PyLong_FromDouble(hoc_xpop())} : pop();
        result = key ? Ref{py.PyObject_GetItem(value.get(), key.get())} : Ref{};
        if (!result) {
            hoc_error_printed(std::string{"Python get item failed: "} + hoc_object_name(o));
        }
    }
    push(result ? result.get() : value.get());
    return nullptr;
}

const char* assign_component(Object* o) {
    require();
    Top const top{};
    Ref const value = pop();
    auto const target = hoc_pop_object();
    assert(o == target.get());
    auto const* const sym = hoc_spop();
    int const nindex = hoc_ipop();
    int err{};
    check_dimension(nindex);
    if (nindex) {
        check_dimension(hoc_pop_ndim());
        Ref const key = hoc_stack_type() == NUMBER ? Ref{py.PyLong_FromDouble(hoc_xpop())} : pop();
        Ref const sequence = key ? attribute(o, sym) : Ref{};
        err = sequence ? py.PyObject_SetItem(sequence.get(), key.get(), value.get()) : -1;
    } else {
        auto* held = static_cast<PyObject*>(o->u.this_pointer);  // target keeps it
        if (!held || location(held)) {
            held = payload(o).get();
        }
        err = py.PyObject_SetAttr(held, interned(sym), value.get());
    }
    if (err) {
        hoc_error_printed("Assignment to PythonObject failed");
    }
    return nullptr;
}

// ---- callbacks

/// Call a callable, or the first element of a tuple with the second, or its elements, as arguments.
Ref call_command(PyObject* command) {
    if (!is_instance(command, py.PyTuple_Type)) {
        return Ref{py.PyObject_CallObject(command, nullptr)};
    }
    auto* const callable = py.PyTuple_GetItem(command, 0);
    auto* const args = callable ? py.PyTuple_GetItem(command, 1) : nullptr;
    if (!args) {
        return {};
    }
    if (is_instance(args, py.PyTuple_Type)) {
        return Ref{py.PyObject_CallObject(callable, args)};
    }
    return call(callable, Ref::borrow(args));
}

// Windows: this thread waits for the GUI's, which calls Python and HOC. The interface's lock is
// released as often as this thread holds it, and taken as often again. Only the lock's own C
// functions are called, so no other Python code runs here; a signal handler can, in acquire().
struct Waiting {
    PyObject* lock{};
    int count{};
};
thread_local Waiting waiting;

PyThreadState* save_thread() {
    if (!started()) {
        return nullptr;
    }
    waiting = {optional(Hook::hoc_lock).release(), 0};
    if (waiting.lock) {
        Ref const release{py.PyObject_GetAttrString(waiting.lock, "release")};
        while (call(release.get())) {
            ++waiting.count;
        }
        py.PyErr_Clear();  // it does not hold the lock any more
    }
    return py.PyEval_SaveThread();
}

void restore_thread(PyThreadState* state) {
    if (!state) {
        return;
    }
    py.PyEval_RestoreThread(state);
    auto const [lock, count] = std::exchange(waiting, {});
    if (Ref const held{lock}) {
        Ref const acquire{py.PyObject_GetAttrString(lock, "acquire")};
        for (int i = 0; i < count;) {
            if (call(acquire.get())) {
                ++i;
            } else {  // a signal handler raised; there is nobody here to raise it in
                py.PyErr_WriteUnraisable(lock);
            }
        }
    }
}

int hoccommand_exec(Object* o) {
    require_python_object(o, "not a Python callback");
    Top const top{};
    Ref const held = payload(o);
    if (call_command(held.get())) {
        return 1;
    }
    fail_callback("Python Callback failed [hoccommand_exec]:", true);
    return 0;
}

int hoccommand_exec_strret(Object* o, char* buf, int size) {
    require_python_object(o, "not a Python callback");
    Top const top{};
    Ref const held = payload(o);
    if (Ref const result = call_command(held.get())) {
        std::strncpy(buf, text(result.get()).c_str(), size);
        buf[size - 1] = '\0';
        return 1;
    }
    fail_callback("Python Callback failed");
    return 0;
}

// err is 0 if a failure is only to be reported in it.
double call_func(Object* o, int narg, int* err) {
    require_python_object(o, "not a Python callback");
    Top const top{};
    Ref const held = payload(o);
    Ref const args = pop_tuple(narg);
    Ref const result{py.PyObject_CallObject(held.get(), args.get())};
    if (!result) {
        if (!err || *err) {
            fail(Error{}, "func_call failed", Raise::hoc_error);
        }
        py.PyErr_Clear();
        *err = 1;
        return 0.;
    }
    if (err) {
        *err = 0;
    }
    double value{};
    return is_number(result.get(), value) ? value : 0.;
}

void vec_apply(IvocVect* vec, Object* o, int start, int end) {
    require_python_object(o, "apply: first argument must be a HOC string or a Python callable");
    Top const top{};
    Ref const f = payload(o);
    if (!py.PyCallable_Check(f.get())) {
        hoc_execerror("Object is not callable", nullptr);
    }
    // As NEURON's nrnpy_call_func: any exception, an interrupt and SystemExit too, is only the HOC
    // error; nothing is printed.
    for (int i = start; i <= end; ++i) {
        Ref const arg{py.PyFloat_FromDouble(vec->elem(i))};
        Ref const result = call(f.get(), arg);
        if (!result) {
            fail(Error{}, "Python function call raised exception", Raise::always, false);
        }
        if (!py.PyNumber_Check(result.get())) {
            hoc_execerror("Expected a numeric result from Python function", nullptr);
        }
        double const value = py.PyFloat_AsDouble(result.get());
        if (value == -1. && py.PyErr_Occurred()) {
            fail(Error{}, "Failed to convert result to float", Raise::always, false);
        }
        vec->elem(i) = value;
    }
}

double praxis_efun(Object* f, Object* vec) {
    require_python_object(f, "not a Python callback");
    Top const top{};
    Ref const arg = to_python(vec);
    Ref const held = payload(f);
    Ref const result = call(held.get(), arg);
    if (!result) {
        Error error{};
        if (!error.is(py.PyExc_Exception) && !error.is(py.PyExc_SystemExit)) {
            error.print();
            return 1e9;
        }
        fail(std::move(error), "Call of Python Callable failed in praxis_efun", Raise::hoc_error);
    }
    // float(result), as NEURON's nb::float_: its failure is not a HOC error.
    Ref const number{py.PyNumber_Float(result.get())};
    if (!number) {
        throw_python_error(true);
    }
    return py.PyFloat_AsDouble(number.get());
}

Object* callable_with_args(Object* o, int narg) {
    require_python_object(o, "not a Python callback");
    Top const top{};
    Ref const command = make_tuple(payload(o), pop_tuple(narg));
    return new_python_object(command.get());
}

void call_with_section(Object* o, Section* sec) {
    require_python_object(o, "not a Python callback");
    Top const top{};
    Ref const section = wrap(sec);
    if (!section) {
        fail_hook();
    }
    Ref const held = payload(o);
    if (!call(held.get(), section)) {
        fail_callback("Call of Python Callable failed");
    }
}

void command_tool(Object* o, int type, double x, double y, int key) {
    require_python_object(o, "not a Python callback");
    Top const top{};
    Ref const held = payload(o);
    if (!call(held.get(),
              Ref{py.PyLong_FromLongLong(type)},
              Ref{py.PyFloat_FromDouble(x)},
              Ref{py.PyFloat_FromDouble(y)},
              Ref{py.PyLong_FromLongLong(key)})) {
        fail_callback("Python Callback failed");
    }
}

// HOC's operators try a Python object's method first. 0 leaves the operator to HOC.
int call_method(Object* o, const char* method, Ref const& arg) {
    Ref const f{py.PyObject_GetAttrString(payload(o).get(), method)};
    Ref const result = f && py.PyCallable_Check(f.get()) ? call(f.get(), arg) : Ref{};
    double value{};
    if (!result || (py.PyNumber_Check(result.get()) &&
                    (value = py.PyFloat_AsDouble(result.get())) == -1. && py.PyErr_Occurred())) {
        py.PyErr_Clear();
        return 0;
    }
    if (py.PyNumber_Check(result.get())) {
        hoc_pushx(value);
    } else {
        Object* const r = to_hoc(result.get());
        hoc_push_object(r);
        if (r) {
            --r->refcount;
        }
    }
    return 1;
}

int call_obj_method(Object* o, const char* method, Object* other) {
    if (!is_python_object(o) || !start()) {
        return 0;
    }
    Top const top{};
    return call_method(o, method, to_python(other));
}

int call_obj_method_double(Object* o, const char* method, double x) {
    if (!is_python_object(o) || !start()) {
        return 0;
    }
    Top const top{};
    // A whole number is an int: rxd's coefficients tell them apart.
    bool const whole = x == std::floor(x) && std::abs(x) < 9e18;
    return call_method(o,
                       method,
                       Ref{whole ? py.PyLong_FromLongLong(static_cast<long long>(x))
                                 : py.PyFloat_FromDouble(x)});
}

// ---- GUI variables: a (target, key) tuple

struct Variable {
    Ref held;
    PyObject *target, *key;
    explicit Variable(Object* o)
        : held{payload(o)}
        , target{py.PyTuple_GetItem(held.get(), 0)}
        , key{target ? py.PyTuple_GetItem(held.get(), 1) : nullptr} {}
    bool is_item() const {
        // A HOC object's names are attributes, though it has __getitem__.
        return (py.PySequence_Check(target) || py.PyMapping_Check(target)) &&
               !(is_instance(target, types.object) && is_instance(key, py.PyUnicode_Type));
    }
};

double gui_get(Object* o) {
    require_python_object(o, "not a Python variable");
    Top const top{};
    Variable const v{o};
    Ref const value{!v.key        ? nullptr
                    : v.is_item() ? py.PyObject_GetItem(v.target, v.key)
                                  : py.PyObject_GetAttr(v.target, v.key)};
    double const x = value ? py.PyFloat_AsDouble(value.get()) : -1.;
    if (x == -1. && py.PyErr_Occurred()) {
        fail_callback("Python GUI get failed");
    }
    return x;
}

void gui_set(Object* o, double x) {
    require_python_object(o, "not a Python variable");
    Top const top{};
    Variable const v{o};
    Ref const value{py.PyFloat_FromDouble(x)};
    if (!v.key || (v.is_item() ? py.PyObject_SetItem(v.target, v.key, value.get())
                               : py.PyObject_SetAttr(v.target, v.key, value.get()))) {
        fail_callback("Python GUI set failed");
    }
}

// Returns whether the string changed.
int gui_get_str(Object* o, char** cpp) {
    require_python_object(o, "not a Python variable");
    Top const top{};
    Variable const v{o};
    Ref const value{v.key ? py.PyObject_GetAttr(v.target, v.key) : nullptr};
    if (!value) {
        fail_callback("Python GUI get str failed");
    }
    auto const string = text(value.get());
    if (*cpp && string == *cpp) {
        return 0;
    }
    delete[] * cpp;
    *cpp = new char[string.size() + 1];
    std::strcpy(*cpp, string.c_str());
    return 1;
}

// ---- the GUI in Python: gui_callback(name, the object of a method, HOC's object, arguments...)

/// The argument of the HOC call in progress.
Ref argument(int i, bool reference) {
    if (hoc_is_object_arg(i)) {
        return to_python(*hoc_objgetarg(i));
    }
    if (hoc_is_pdouble_arg(i)) {
        static Symbol* const pointer = hoc_table_lookup("Pointer", hoc_built_in_symlist);
        Object* const o = hoc_new_object(pointer, new OcPointer("", hoc_hgetarg<double>(i)));
        Ref const target = to_python(o);
        return call(need(Hook::reference).get(), target);
    }
    if (!hoc_is_str_arg(i)) {
        return Ref{py.PyFloat_FromDouble(*getarg(i))};
    }
    return reference
               ? call(need(Hook::reference).get(), Ref{py.PyLong_FromVoidPtr(hoc_pgargstr(i))})
               : Ref{py.PyUnicode_FromString(gargstr(i))};
}

/// Null if there is no callback.
Ref call_gui(const char* name, Object* obj, bool references) {
    if (!registered(Hook::gui_callback) || !start()) {
        return {};
    }
    // A destructor runs when the object is freed, maybe outside any HOC call: no arguments.
    bool const destructor = name[0] == '~';
    Object* const self = destructor ? nullptr : hoc_thisobject;
    Top const top{};
    int n{};
    while (!destructor && ifarg(n + 1)) {
        ++n;
    }
    Ref const args{py.PyTuple_New(n + 3)};
    py.PyTuple_SetItem(args.get(), 0, py.PyUnicode_FromString(name));
    py.PyTuple_SetItem(args.get(), 1, to_python(obj).release());
    py.PyTuple_SetItem(args.get(), 2, to_python(self).release());
    for (int i = 1; i <= n; ++i) {
        Ref value = argument(i, references);
        if (!value) {
            fail_hook();  // the reference hook raised
        }
        py.PyTuple_SetItem(args.get(), i + 2, value.release());
    }
    Ref result{py.PyObject_CallObject(optional(Hook::gui_callback).get(), args.get())};
    if (!result) {
        py.PyErr_Print();
        result = Ref{py.PyLong_FromLongLong(0)};
    }
    return result;
}

Object** gui_helper3(const char* name, Object* obj, int references) {
    Ref const result = call_gui(name, obj, references);
    return result ? hoc_temp_objptr(to_hoc_temp(result.get())) : nullptr;
}

char** gui_helper3_str(const char* name, Object* obj, int references) {
    Ref const result = call_gui(name, obj, references);
    return result ? temp_string(result.get()) : nullptr;
}

// ---- sections and locations

Section* o2sec(Object* o) {
    require_python_object(o, "not a Python nrn.Section");
    Top const top{};
    Ref const held = payload(o);
    auto* const sec = is_instance(held.get(), types.section) ? section(held.get()) : nullptr;
    if (!sec) {
        hoc_execerror("not a Python nrn.Section", nullptr);
    }
    return sec;
}

constexpr auto not_segment = "not a Python nrn.Segment";
constexpr auto not_location =
    "not a Python nrn.Segment, rxd.node, or other with a segment property";

/// What HOC made and still has itself.
bool locate(Object* o, Section** sec, double* x) {
    auto* const l = location(o->u.this_pointer);
    if (l && l->sec->prop) {
        *sec = l->sec;
        *x = l->x;
    }
    return *sec;
}

void locate(PyObject* segment, Section** sec, double* x) {
    auto* const wrapper = slot(segment, SegmentSlot::sec);
    auto* const position = slot(segment, SegmentSlot::x);
    *sec = is_instance(wrapper, types.section) ? section(wrapper) : nullptr;
    if (!*sec || !(*sec)->prop) {
        hoc_execerror("nrn.Segment associated with deleted internal Section", nullptr);
    }
    *x = py.PyFloat_AsDouble(position);
    if (*x == -1. && py.PyErr_Occurred()) {
        fail_hook();
    }
}

void o2loc(Object* o, Section** sec, double* x) {
    require_python_object(o, not_segment);
    if (locate(o, sec, x)) {
        return;
    }
    Top const top{};
    Ref const held = payload(o);
    if (!is_instance(held.get(), types.segment)) {
        hoc_execerror(not_segment, nullptr);
    }
    locate(held.get(), sec, x);
}

// Also an object with a segment attribute, such as an rxd node, or a list of one.
void o2loc2(Object* o, Section** sec, double* x) {
    require_python_object(o, not_location);
    if (locate(o, sec, x)) {
        return;
    }
    Top const top{};
    Ref held = payload(o);
    if (!is_instance(held.get(), types.segment)) {
        if (is_instance(held.get(), py.PyList_Type)) {
            if (py.PyList_Size(held.get()) != 1) {
                hoc_execerror("If a list is supplied, it must be of length 1", nullptr);
            }
            held = Ref::borrow(py.PyList_GetItem(held.get(), 0));
        }
        held = Ref{py.PyObject_GetAttrString(held.get(), "segment")};
        if (!held &&
            !py.PyErr_GivenExceptionMatches(py.PyErr_Occurred(), py.PyExc_AttributeError)) {
            fail_hook();
        }
        py.PyErr_Clear();
        if (!held || !is_instance(held.get(), types.segment)) {
            hoc_execerror(not_location, nullptr);
        }
    }
    locate(held.get(), sec, x);
}

int is_segment(Object* o) {
    if (!is_python_object(o) || !types.segment || !start()) {
        return 0;
    }
    if (location(o->u.this_pointer)) {
        return 1;
    }
    Gil const gil{};
    return is_instance(payload(o).get(), types.segment);
}

Object* new_segment(Section* sec, double x) {
    require();
    Gil const gil{};
    auto* const l = new Location{sec, x};
    return new_python_object_of(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(l) | 1));
}

void append_sections(void* list, Object* o) {
    require_python_object(o, "argument must be a Python iterable");
    Top const top{};
    Ref const iterator{py.PyObject_GetIter(payload(o).get())};
    if (!iterator) {
        py.PyErr_Clear();
        hoc_execerror("argument must be an iterable", nullptr);
    }
    while (Ref const item{py.PyIter_Next(iterator.get())}) {
        auto* const sec = is_instance(item.get(), types.section) ? section(item.get()) : nullptr;
        if (!sec) {
            hoc_execerror("iterable must contain only Section objects", nullptr);
        }
        lvappendsec_and_ref(list, sec);
    }
    if (py.PyErr_Occurred()) {
        fail(Error{}, "argument must be a Python iterable", Raise::exception);
    }
}

// ---- Vector

/// The doubles of an object with __array_interface__, such as a numpy array, and their stride.
char* array_interface(PyObject* p, std::ptrdiff_t& stride) {
    if (!py.PyObject_HasAttrString(p, "__array_interface__")) {
        return nullptr;
    }
    Ref const interface{py.PyObject_GetAttrString(p, "__array_interface__")};
    auto* const type = interface ? py.PyDict_GetItemString(interface.get(), "typestr") : nullptr;
    int const one = 1;
    auto const native = reinterpret_cast<char const&>(one) ? "<f8" : ">f8";
    if (!type || text(type) != native) {
        py.PyErr_Clear();
        return nullptr;
    }
    auto* const buffer = py.PyDict_GetItemString(interface.get(), "data");
    auto* const data = buffer && is_instance(buffer, py.PyTuple_Type)
                           ? py.PyTuple_GetItem(buffer, 0)
                           : nullptr;
    auto* const address = data ? static_cast<char*>(py.PyLong_AsVoidPtr(data)) : nullptr;
    auto* const strides = py.PyDict_GetItemString(interface.get(), "strides");
    stride = sizeof(double);
    if (strides && strides != py._Py_NoneStruct) {
        stride = py.PyLong_AsLong(py.PyTuple_GetItem(strides, 0));
    }
    if (py.PyErr_Occurred()) {
        py.PyErr_Clear();
        return nullptr;
    }
    return address;
}

double item_as_double(PyObject* item, std::ptrdiff_t i) {
    double const x = py.PyFloat_AsDouble(item);
    if (x == -1. && py.PyErr_Occurred()) {
        py.PyErr_Clear();
        hoc_execerror(fmt::format("item {} is not a valid number", i).c_str(), nullptr);
    }
    return x;
}

IvocVect* vec_from_python(void* v) {
    auto* const vec = static_cast<IvocVect*>(v);
    Object* const o = *hoc_objgetarg(1);
    require_python_object(o, "the argument of Vector.from_python is not a PythonObject");
    Top const top{};
    Hold const hold{o};
    Ref const held = payload(o);
    auto* const source = held.get();
    Ref list;
    if (!py.PySequence_Check(source)) {
        list = Ref{py.PySequence_List(source)};
        if (!list) {
            fail(Error{},
                 std::string{hoc_object_name(o)} +
                     " does not support the Python Sequence or Iterator protocol",
                 Raise::hoc_error,
                 false);
        }
    } else if (is_instance(source, py.PyList_Type)) {
        list = Ref::borrow(source);
    }
    auto const size = py.PySequence_Size(list ? list.get() : source);
    if (size < 0) {
        fail_hook();
    }
    vec->resize(size);
    if (std::ptrdiff_t stride; !list) {
        if (auto const* const data = array_interface(source, stride)) {
            for (std::ptrdiff_t i = 0; i < size; ++i) {
                vec->elem(i) = *reinterpret_cast<double const*>(data + i * stride);
            }
            return vec;
        }
    }
    for (std::ptrdiff_t i = 0; i < size; ++i) {
        auto* const borrowed = list ? py.PyList_GetItem(list.get(), i) : nullptr;
        if (borrowed && type_of(borrowed) == py.PyFloat_Type) {
            vec->elem(i) = py.PyFloat_AsDouble(borrowed);
            continue;
        }
        // Converting anything else can run Python code, which can change a list.
        Ref const item = borrowed ? Ref::borrow(borrowed)
                                  : Ref{list ? nullptr : py.PySequence_GetItem(source, i)};
        if (!item) {
            fail_hook();
        }
        vec->elem(i) = item_as_double(item.get(), i);
    }
    return vec;
}

Object** vec_to_python(void* v) {
    auto* const vec = static_cast<IvocVect*>(v);
    require();
    Top const top{};
    std::ptrdiff_t const size = vec->size();
    if (!ifarg(1)) {
        Ref const list{py.PyList_New(size)};
        for (std::ptrdiff_t i = 0; i < size; ++i) {
            py.PyList_SetItem(list.get(), i, py.PyFloat_FromDouble(vec->elem(i)));
        }
        return hoc_temp_objptr(to_hoc_temp(list.get()));
    }
    Object* const o = *hoc_objgetarg(1);
    require_python_object(o, "the argument of Vector.to_python is not a PythonObject");
    Hold const hold{o};
    Ref const held = payload(o);
    auto* const sequence = held.get();
    if (!py.PySequence_Check(sequence)) {
        hoc_execerror(hoc_object_name(o), " is not a Python Sequence");
    }
    if (py.PySequence_Size(sequence) != size) {
        py.PyErr_Clear();
        hoc_execerror(hoc_object_name(o), "Python Sequence not same size as Vector");
    }
    std::ptrdiff_t stride;
    if (auto* const data = array_interface(sequence, stride)) {
        for (std::ptrdiff_t i = 0; i < size; ++i) {
            *reinterpret_cast<double*>(data + i * stride) = vec->elem(i);
        }
    } else {
        for (std::ptrdiff_t i = 0; i < size; ++i) {
            Ref const x{py.PyFloat_FromDouble(vec->elem(i))};
            if (py.PySequence_SetItem(sequence, i, x.get())) {
                fail(Error{},
                     fmt::format("Could not set a Python Sequence item {} of {}", i, size),
                     Raise::hoc_error);
            }
        }
    }
    return hoc_temp_objptr(o);
}

// The array shares the Vector's data.
Object** vec_as_numpy(int size, double* data) {
    require();
    Top const top{};
    Ref const numpy{py.PyImport_ImportModule("numpy")};
    Ref array;
    if (numpy) {
        int const writable = stable::buf_write;
        Ref const make{py.PyObject_GetAttrString(numpy.get(), size ? "frombuffer" : "empty")};
        Ref const arg{size ? py.PyMemoryView_FromMemory(reinterpret_cast<char*>(data),
                                                        size * sizeof(double),
                                                        writable)
                           : py.PyLong_FromLongLong(0)};
        array = make && arg ? call(make.get(), arg) : Ref{};
    }
    if (!array) {
        fail(Error{}, "Vector.as_numpy() error", Raise::exception);
    }
    return hoc_temp_objptr(to_hoc_temp(array.get()));
}

// ---- ParallelContext

/// pickle.dumps or pickle.loads. Not released: Python may already be gone at exit.
PyObject* pickle_function(const char* name) {
    Ref const pickle{py.PyImport_ImportModule("pickle")};
    Ref function{pickle ? py.PyObject_GetAttrString(pickle.get(), name) : nullptr};
    if (!function) {
        hoc_error_from_python("Python's pickle is not available");
    }
    return function.release();
}

std::vector<char> dumps(PyObject* p) {
    static PyObject* function;
    if (!function) {
        function = pickle_function("dumps");
    }
    Ref const bytes = call(function, Ref::borrow(p));
    char* data{};
    std::ptrdiff_t size{};
    if (!bytes || py.PyBytes_AsStringAndSize(bytes.get(), &data, &size)) {
        throw_python_error();
    }
    return {data, data + size};
}

Ref loads(const char* data, std::size_t size) {
    static PyObject* function;
    if (!function) {
        function = pickle_function("loads");
    }
    Ref const bytes{py.PyBytes_FromStringAndSize(data, size)};
    Ref value = call(function, bytes);
    if (!value) {
        throw_python_error();
    }
    return value;
}

std::vector<char> po2pickle(Object* o) {
    if (!is_python_object(o)) {
        return {};
    }
    require();
    Top const top{};
    return dumps(payload(o).get());
}

Object* pickle2po(const std::vector<char>& pickled) {
    require();
    Top const top{};
    return new_python_object(loads(pickled.data(), pickled.size()).get());
}

std::vector<char> call_picklef(const std::vector<char>& pickled, int narg) {
    require();
    Top const top{};
    Ref const f = loads(pickled.data(), pickled.size());
    Ref const args = pop_tuple(narg);
    Ref const result{py.PyObject_CallObject(f.get(), args.get())};
    if (!result) {
        Error error{};
        if (!error.is(py.PyExc_Exception)) {
            // As NEURON's Python module: printed, then pickling no result fails.
            error.print();
            throw std::bad_cast{};
        }
        hoc_error_traceback("PyObject method call failed:", std::move(error));
    }
    return dumps(result.get());
}

#if NRNMPI
std::vector<int> displacements(const std::vector<int>& counts) {
    std::vector<int> d(counts.size() + 1);
    for (std::size_t i = 0; i < counts.size(); ++i) {
        d[i + 1] = d[i] + counts[i];
    }
    return d;
}

/// A list of the pickled values in data. A rank that sent nothing sent None.
Ref unpickle_list(const std::vector<char>& data,
                  const std::vector<int>& counts,
                  const std::vector<int>& displs) {
    Ref list{py.PyList_New(0)};
    for (std::size_t i = 0; i < counts.size(); ++i) {
        Ref const item = counts[i] ? loads(data.data() + displs[i], counts[i])
                                   : Ref::borrow(py._Py_NoneStruct);
        py.PyList_Append(list.get(), item.get());
    }
    return list;
}

Ref allgather(PyObject* source) {
    auto send = dumps(source);
    std::vector<int> counts(nrnmpi_numprocs);
    counts[nrnmpi_myid] = static_cast<int>(send.size());
    nrnmpi_int_allgather_inplace(counts.data(), 1);
    auto displs = displacements(counts);
    std::vector<char> data(displs.back());
    nrnmpi_char_allgatherv(send.data(), data.data(), counts.data(), displs.data());
    return unpickle_list(data, counts, displs);
}

Ref gather(PyObject* source, int root) {
    auto send = dumps(source);
    bool const is_root = nrnmpi_myid == root;
    int size = static_cast<int>(send.size());
    std::vector<int> counts(is_root ? nrnmpi_numprocs : 0);
    nrnmpi_int_gather(&size, counts.data(), 1, root);
    auto displs = displacements(counts);
    std::vector<char> data(displs.back());
    nrnmpi_char_gatherv(send.data(), size, data.data(), counts.data(), displs.data(), root);
    return is_root ? unpickle_list(data, counts, displs) : Ref::borrow(py._Py_NoneStruct);
}

Ref broadcast(PyObject* source, int root) {
    bool const is_root = nrnmpi_myid == root;
    auto data = is_root ? dumps(source) : std::vector<char>{};
    int size = static_cast<int>(data.size());
    nrnmpi_int_broadcast(&size, 1, root);
    data.resize(size);
    nrnmpi_char_broadcast(data.data(), size, root);
    return is_root ? Ref::borrow(source) : loads(data.data(), data.size());
}

/// The pickled items of a list, one per rank, None as nothing.
std::vector<char> pickle_items(PyObject* list, std::vector<int>& counts, bool keep) {
    std::vector<char> data;
    for (int i = 0; i < nrnmpi_numprocs; ++i) {
        auto* const item = py.PyList_GetItem(list, i);
        auto const pickled = item == py._Py_NoneStruct ? std::vector<char>{} : dumps(item);
        counts.push_back(static_cast<int>(pickled.size()));
        if (keep) {
            data.insert(data.end(), pickled.begin(), pickled.end());
        }
    }
    return data;
}

Ref scatter(PyObject* source, int root) {
    std::vector<int> counts;
    auto send = nrnmpi_myid == root ? pickle_items(source, counts, true) : std::vector<char>{};
    auto displs = displacements(counts);
    int size{};
    nrnmpi_int_scatter(counts.data(), &size, 1, root);
    std::vector<char> data(size + 1);  // not empty
    nrnmpi_char_scatterv(send.data(), counts.data(), displs.data(), data.data(), size, root);
    return size ? loads(data.data(), size) : Ref::borrow(py._Py_NoneStruct);
}

// A negative size asks only for the numbers of bytes to be sent and received.
Ref alltoall(PyObject* source, int size) {
    std::vector<int> send_counts;
    auto send = pickle_items(source, send_counts, size >= 0);
    std::vector<int> ones(nrnmpi_numprocs, 1);
    auto unit = displacements(ones);
    std::vector<int> counts(nrnmpi_numprocs);
    nrnmpi_int_alltoallv(
        send_counts.data(), ones.data(), unit.data(), counts.data(), ones.data(), unit.data());
    auto send_displs = displacements(send_counts);
    auto displs = displacements(counts);
    if (size < 0) {
        return make_tuple(Ref{py.PyLong_FromLongLong(send_displs.back())},
                          Ref{py.PyLong_FromLongLong(displs.back())});
    }
    std::vector<char> data(displs.back() + 1);  // not empty
    nrnmpi_char_alltoallv(send.data(),
                          send_counts.data(),
                          send_displs.data(),
                          data.data(),
                          counts.data(),
                          displs.data());
    return unpickle_list(data, counts, displs);
}
#endif

/// type is 1 alltoall, 2 allgather, 3 gather, 4 broadcast, 5 scatter. For 3, 4 and 5 size is the
/// root rank.
Object* collective(int size, int type) {
    require();
    Top const top{};
    enum { ALLTOALL = 1, ALLGATHER, GATHER, BROADCAST, SCATTER };
    int const nhost = nrnmpi_numprocs;
    if (type == ALLTOALL || type == SCATTER) {
        hoc_objgetarg(1);  // a HOC error if it is not an object, as NEURON's
    }
    Ref const source = argument(1, false);
    if (type == ALLTOALL || (type == SCATTER && nrnmpi_myid == size)) {
        if (!is_instance(source.get(), py.PyList_Type)) {
            hoc_execerror("Argument must be a Python list", nullptr);
        }
        if (py.PyList_Size(source.get()) != nhost) {
            hoc_execerror(type == ALLTOALL ? "py_alltoall list size must be nhost"
                                           : "py_scatter list size must be nhost",
                          nullptr);
        }
    }
    if (nhost == 1) {
        switch (type) {
        case SCATTER:
            return to_hoc_temp(py.PyList_GetItem(source.get(), 0));
        case ALLGATHER:
        case GATHER: {
            Ref const list{py.PyList_New(0)};
            py.PyList_Append(list.get(), source.get());
            return to_hoc_temp(list.get());
        }
        default:
            return to_hoc_temp(source.get());
        }
    }
#if NRNMPI
    if (type != ALLTOALL && type != ALLGATHER && (size < 0 || size >= nhost)) {
        hoc_execerror("root rank must be >= 0 and < nhost", nullptr);
    }
    Ref result;
    switch (type) {
    case ALLTOALL:
        result = alltoall(source.get(), size);
        break;
    case ALLGATHER:
        result = allgather(source.get());
        break;
    case GATHER:
        result = gather(source.get(), size);
        break;
    case BROADCAST:
        result = broadcast(source.get(), size);
        break;
    default:
        result = scatter(source.get(), size);
    }
    Object* const o = to_hoc(result.get());
    result = Ref{};  // before the reference is given up: it may hold the object's last other one
    if (o) {
        --o->refcount;
    }
    return o;
#else
    return nullptr;
#endif
}

// ---- other hooks

void* opaque_object(Object* o) {
    if (!is_python_object(o)) {
        return nullptr;
    }
    if (location(o->u.this_pointer)) {
        Gil const gil{};
        payload(o);
    }
    return o->u.this_pointer;
}

int same_object(Object* a, Object* b) {
    return is_python_object(a) && is_python_object(b) && a->u.this_pointer == b->u.this_pointer;
}

// SaveState keeps the bytes the interface's store function returns, and gives them back to restore.
void savestate_store(char** data, uint64_t* size) {
    *size = 0;
    if (!registered(Hook::savestate_store) || !start()) {
        return;
    }
    Top const top{};
    Ref const state = call(optional(Hook::savestate_store).get());
    Ref const bytes{state ? py.PyBytes_FromObject(state.get()) : nullptr};
    char* chars{};
    std::ptrdiff_t n{};
    if (!bytes || py.PyBytes_AsStringAndSize(bytes.get(), &chars, &n)) {
        fail(Error{}, "SaveState: Data store failure.", Raise::hoc_error);
    }
    delete[] * data;
    *data = new char[n];
    std::memcpy(*data, chars, n);
    *size = n;
}

void savestate_restore(int64_t size, char* data) {
    if (!registered(Hook::savestate_restore) || !start()) {
        if (size) {
            hoc_execerror("SaveState:", "Missing data restore function.");
        }
        return;
    }
    Top const top{};
    Ref const bytes{py.PyByteArray_FromStringAndSize(data, size)};
    if (!call(optional(Hook::savestate_restore).get(), bytes)) {
        fail(Error{}, "SaveState: Data restore failure.", Raise::hoc_error);
    }
}

// CoreNEURON's options are read while the simulator is set up: an error is printed, not raised.
int coreneuron_option(Hook hook) {
    if (!registered(hook) || !start()) {
        return 0;
    }
    Top const top{};
    Ref const value = call(optional(hook).get());
    int const option = value ? static_cast<int>(py.PyLong_AsLong(value.get())) : -1;
    if (py.PyErr_Occurred()) {
        py.PyErr_Print();
        return -1;
    }
    return option;
}

int coreneuron_enable() {
    return coreneuron_option(Hook::coreneuron_enable);
}

int coreneuron_file_mode() {
    return coreneuron_option(Hook::coreneuron_file_mode);
}

char* coreneuron_arg(double tstop) {
    if (!registered(Hook::coreneuron_arg) || !start()) {
        return nullptr;
    }
    Top const top{};
    Ref const arg{py.PyFloat_FromDouble(tstop)};
    Ref const value = call(optional(Hook::coreneuron_arg).get(), arg);
    if (!value) {
        py.PyErr_Print();
        return nullptr;
    }
    return value.get() == py._Py_NoneStruct ? nullptr : strdup(text(value.get()).c_str());
}

// What to plot for the Python object a RangeVarPlot was given: an rxd species becomes a function.
Object* rvp_callable(Object* o) {
    if (!is_python_object(o) || !registered(Hook::rvp_callable) || !start()) {
        return o;
    }
    Top const top{};
    Ref const value = call(optional(Hook::rvp_callable).get(), payload(o));
    if (!value) {
        Error{}.print();
        return o;
    }
    return value.get() == py._Py_NoneStruct ? o : to_hoc_temp(value.get());
}

// PlotShape's Python variable.
void* plotshape_object(Object* o) {
    if (!is_python_object(o) || !start()) {
        return nullptr;
    }
    Gil const gil{};
    return payload(o).release();
}

void plotshape_release(void* p) {
    if (p && start()) {
        Top const top{};
        py.Py_DecRef(static_cast<PyObject*>(p));
    }
}

void nrnpython_statement() {
    bool ok{};
    if (start()) {
        Top const top{};
        ok = static_cast<bool>(run(gargstr(1)));
        if (!ok) {
            Error error{};
            if (error.is(py.PyExc_SystemExit)) {
                fail(std::move(error), "SystemExit", Raise::exception);
            }
            error.print();
        }
    }
    hoc_retpushx(ok);
}
}  // namespace

Ref optional(Hook hook) {
    auto* const p = registered(hook);
    return p ? Ref::borrow(p) : Ref{};
}

Ref need(Hook hook) {
    if (!registered(hook)) {
        // Importing it reads HOC files, and HOC is in a line.
        std::string const line{hoc_cbufstr->buf};
        auto const position = hoc_ctp - hoc_cbufstr->buf;
        Ref const neuron{py.PyImport_ImportModule("neuron")};
        std::strcpy(hoc_cbufstr->buf, line.c_str());
        hoc_ctp = hoc_cbufstr->buf + position;
        if (!neuron) {
            hoc_error_from_python("could not import neuron");
        }
        if (!registered(hook)) {
            hoc_execerror("The neuron package did not register its Python objects", nullptr);
        }
    }
    return optional(hook);
}

void model_changed() {
    // The interface keeps the epoch in a list. None stands for one that nobody has asked for yet.
    if (auto* const epoch = registered(Hook::model_epoch);
        epoch && py.PyList_GetItem(epoch, 0) != py._Py_NoneStruct) {
        py.PyList_SetItem(epoch, 0, Ref::borrow(py._Py_NoneStruct).release());
    }
}

ErrorScope::ErrorScope(bool can_be_taken)
    : outer_{std::exchange(innermost_scope, this)}
    , can_be_taken{can_be_taken} {}

ErrorScope::~ErrorScope() {
    innermost_scope = outer_;
    // HOC code went on after the error.
    print_exception(exception_);
}

ErrorScope* ErrorScope::innermost() {
    return innermost_scope;
}

void ErrorScope::keep(PyObject* exception) {
    print_exception(std::exchange(exception_, exception));
}

void ErrorScope::caught() {
    print_exception(std::exchange(pending_exception, std::exchange(exception_, nullptr)));
}

void hoc_error_from_python(const std::string& message) {
    fail(Error{}, message, Raise::exception);
}

void require() {
    if (!start()) {
        hoc_execerror("Python not available", nullptr);
    }
}

PyObject* main_namespace() {
    auto* const main = py.PyImport_AddModule("__main__");
    return main ? py.PyModule_GetDict(main) : nullptr;
}

Ref run(const char* statements) {
    Ref const code{py.Py_CompileString(statements, "<string>", file_input)};
    auto* const vars = code ? main_namespace() : nullptr;
    return Ref{vars ? py.PyEval_EvalCode(code.get(), vars, vars) : nullptr};
}

void register_python_object() {
    class2oc("PythonObject", construct, destruct, nullptr, nullptr, nullptr);
    nrnpy_pyobj_sym_ = hoc_lookup("PythonObject");
    nrn_template_set_component_hooks(nrnpy_pyobj_sym_, component, assign_component, nullptr, 0);

    methods.callable_with_args = callable_with_args;
    methods.call_func = call_func;
    methods.call_picklef = call_picklef;
    methods.call_python_with_section = call_with_section;
    methods.cmdtool = command_tool;
    methods.guigetstr = gui_get_str;
    methods.guigetval = gui_get;
    methods.guisetval = gui_set;
    methods.gui_helper = [](const char* name, Object* obj) { return gui_helper3(name, obj, 0); };
    methods.gui_helper3 = gui_helper3;
    methods.gui_helper3_str = gui_helper3_str;
    methods.object_to_double = [](Object* o) {
        if (!started()) {
            return 0.;
        }
        Gil const gil{};
        double const value = py.PyFloat_AsDouble(to_python(o).get());
        if (value == -1. && py.PyErr_Occurred()) {
            py.PyErr_Clear();  // as -1, which the caller cannot tell from a value
        }
        return value;
    };
    methods.hoccommand_exec = hoccommand_exec;
    methods.hoccommand_exec_strret = hoccommand_exec_strret;
    methods.hoc_nrnpython = nrnpython_statement;
    methods.ho2po = [](Object* o) { return to_python(o).release(); };
    methods.mpi_alltoall_type = collective;
    methods.opaque_obj2pyobj = opaque_object;
    methods.pickle2po = pickle2po;
    methods.po2ho = to_hoc;
    methods.po2pickle = po2pickle;
    methods.praxis_efun = praxis_efun;
    methods.pysame = same_object;
    methods.restore_thread = restore_thread;
    methods.save_thread = save_thread;
    nrnpy_call_obj_method = call_obj_method;
    nrnpy_call_obj_method_double = call_obj_method_double;
    nrnpy_decref = plotshape_release;
    nrnpy_get_pyobj = plotshape_object;
    nrnpy_hoccommand_exec = hoccommand_exec;
    nrnpy_nrncore_arg_p_ = coreneuron_arg;
    nrnpy_nrncore_enable_value_p_ = coreneuron_enable;
    nrnpy_nrncore_file_mode_value_p_ = coreneuron_file_mode;
    nrnpy_o2loc_p_ = o2loc;
    nrnpy_o2loc2_p_ = o2loc2;
    nrnpy_o2sec_p_ = o2sec;
    nrnpy_ob_is_seg = is_segment;
    nrnpy_restore_savestate = savestate_restore;
    nrnpy_rvp_rxd_to_callable = rvp_callable;
    nrnpy_sectionlist_helper_ = append_sections;
    nrnpy_tree_changed_p_ = [] {
        if (started()) {
            Gil const gil{};
            model_changed();
        }
    };
    nrnpy_seg_from_sec_x = new_segment;
    nrnpy_store_savestate = savestate_store;
    nrnpy_vec_apply_p_ = vec_apply;
    nrnpy_vec_as_numpy_helper_ = vec_as_numpy;
    nrnpy_vec_from_python_p_ = vec_from_python;
    nrnpy_vec_to_python_p_ = vec_to_python;
}

}  // namespace neuron::python

using namespace neuron::python;

Object* nrn_python_object_new(PyObject* value) {
    return start() ? new_python_object(value) : nullptr;
}

PyObject* nrn_python_object_get(Object* obj) {
    if (!start()) {
        return nullptr;
    }
    if (!obj) {
        return Ref::borrow(py._Py_NoneStruct).release();
    }
    bool const python_object = is_python_object(obj);
    if (!registered(python_object ? Hook::segment_class : Hook::object_class) &&
        (!python_object || location(obj->u.this_pointer))) {
        py.PyErr_SetString(py.PyExc_RuntimeError, "no class is registered");
        return nullptr;
    }
    return (python_object ? held_by(obj) : wrap(obj)).release();
}

PyObject* nrn_python_type(const char* name) {
    if (!start() || !make_types()) {
        return nullptr;
    }
    auto* const type = !name                                                ? py._Py_NoneStruct
                       : !std::strcmp(name, "object")                       ? types.object
                       : !std::strcmp(name, "section")                      ? types.section
                       : !std::strcmp(name, "segment")                      ? types.segment
                       : !std::strcmp(name, "mechanism") && types.mechanism ? types.mechanism
                       : !std::strcmp(name, "vector") && types.vector       ? types.vector
                                                                            : py._Py_NoneStruct;
    return Ref::borrow(type).release();
}

int nrn_python_hook_set(const char* name, PyObject* object) {
    static char const* const names[]{
#define X(name) #name,
        NRNPY_HOOKS(X)
#undef X
    };
    if (!start() || !make_types()) {
        return 3;
    }
    auto const* const known = std::find_if(std::begin(names), std::end(names), [&](auto* n) {
        return name && !std::strcmp(n, name);
    });
    if (known == std::end(names)) {
        return 1;
    }
    auto const hook = static_cast<Hook>(known - names);
    auto const is_subtype = [&](PyObject* base) {
        return is_instance(object, py.PyType_Type) && py.PyType_IsSubtype(object, base);
    };
    if (!object || object == py._Py_NoneStruct) {
        object = nullptr;
    } else if (hook == Hook::model_epoch
                   ? !is_instance(object, py.PyList_Type) || py.PyList_Size(object) != 1
               : hook == Hook::hoc_lock      ? !py.PyObject_HasAttrString(object, "release")
               : hook == Hook::section_class ? !is_subtype(types.section)
               : hook == Hook::segment_class ? !is_subtype(types.segment)
                                             : !py.PyCallable_Check(object)) {
        return 2;
    }
    if (hook == Hook::object_class) {
        forget_classes();
    } else if (hook == Hook::section_class) {
        own_sections();
    }
    Ref{std::exchange(registered(hook), object ? Ref::borrow(object).release() : nullptr)};
    return 0;
}

int nrnpy_set_gui_callback(PyObject* callback) {
    return nrn_python_hook_set("gui_callback", callback);
}

PyObject* nrn_python_error_take() {
    if (!started()) {  // no Python, no exception
        return nullptr;
    }
    return pending_exception ? std::exchange(pending_exception, nullptr)
                             : Ref::borrow(py._Py_NoneStruct).release();
}
