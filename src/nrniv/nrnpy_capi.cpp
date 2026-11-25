#include <../../nrnconf.h>

#include "nrnpy_capi.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <thread>

#include "oc_ansi.h"

extern NRN_DLLSYM void (*p_nrnpython_finalize)();
extern "C" NRN_DLLSYM void nrnpy_set_pr_etal(int (*cbpr_stdoe)(int, char*));

#ifndef NRNPYTHON_DYNAMICLOAD
// Taking the addresses makes libnrniv need libpython.
extern "C" {
#define X(ret, name, args) ret name args;
NRNPY_FUNCTIONS(X)
#undef X
}
#endif

namespace neuron::python {

Capi py;

namespace {
void* lookup(void* library, const char* name) {
#if defined(_WIN32)
    if (library) {
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), name));
    }
    // A process that runs Python has python3XY.dll. It exports the stable ABI, and what python3.dll
    // leaves out.
    for (int minor = 11; minor <= 30; ++minor) {
        auto const dll = "python3" + std::to_string(minor) + ".dll";
        if (auto const module = GetModuleHandleA(dll.c_str())) {
            return reinterpret_cast<void*>(GetProcAddress(module, name));
        }
    }
    return nullptr;
#else
    return dlsym(library ? library : RTLD_DEFAULT, name);
#endif
}
}  // namespace

void* Capi::optional(const char* name) const {
    return lookup(library_, name);
}

bool Capi::bind(void* library, std::string& missing) {
    library_ = library;
    auto const required = [&](const char* name) {
        auto* const p = optional(name);
        if (!p && missing.empty()) {
            missing = name;
        }
        return p;
    };
#ifdef NRNPYTHON_DYNAMICLOAD
#define X(ret, name, args) name = reinterpret_cast<ret(*) args>(required(#name));
#else
#define X(ret, name, args) name = ::name;
#endif
    NRNPY_FUNCTIONS(X)
#undef X
#define X(name) name = static_cast<PyObject*>(required(#name));
    NRNPY_TYPES(X)
#undef X
#define X(name)                                                     \
    if (auto* const p = static_cast<PyObject**>(required(#name))) { \
        name = *p;                                                  \
    }
    NRNPY_EXCEPTIONS(X)
#undef X
    return missing.empty();
}

bool (*launch)();

bool bound{};

namespace {
std::thread::id host_thread;


// Python has been finalized. The interface's output function is gone with it, but NEURON still
// prints on its way out.
void forget_python() {
    nrnpy_set_pr_etal(nullptr);
}

// Python is the main program: it loaded libnrniv, or a program that embeds it did.
bool attach() {
    std::string missing;
    if (!py.bind(nullptr, missing) || !py.Py_IsInitialized()) {
        return false;
    }
    p_nrnpython_finalize = finalize;
    return true;
}
}  // namespace

void finalize() {
    if (std::this_thread::get_id() == host_thread) {
        py.PyGILState_Ensure();  // nothing is left to release it to
        py.PyErr_Clear();        // quit() in Python code that is handling an error
        py.Py_FinalizeEx();
    }
}

bool start_first() {
    host_thread = std::this_thread::get_id();
    bound = attach() || (launch && launch());
    if (bound) {
        py.Py_AtExit(forget_python);
    }
    return bound;
}

}  // namespace neuron::python
