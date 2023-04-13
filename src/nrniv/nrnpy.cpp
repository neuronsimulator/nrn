#include <../../nrnconf.h>
// For Linux and Max OS X,
// Solve the problem of not knowing what version of Python the user has by
// possibly deferring linking to libnrnpython.so to run time using the proper
// Python interface

#include <../nrnpython/nrnpython_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <InterViews/resource.h>
#include "nrnoc2iv.h"
#include "classreg.h"
#include "nonvintblock.h"
#include "nrnmpi.h"

#include <algorithm>
#include <cctype>
#include <sstream>

// needed by nrnmusic.cpp but must exist if python is loaded.
#if USE_PYTHON
typedef struct _object PyObject;
PyObject* (*nrnpy_p_ho2po)(Object*);
Object* (*nrnpy_p_po2ho)(PyObject*);
#endif  // USE_PYTHON

extern int nrn_nopython;
extern char* nrnpy_pyexe;
extern int nrn_is_python_extension;
using nrnpython_real_t = void (*)();
using nrnpython_reg_real_t = void (*)();
using nrnpython_start_t = int (*)(int);
extern nrnpython_start_t p_nrnpython_start;
void nrnpython();
static nrnpython_real_t p_nrnpython_real{};
static nrnpython_reg_real_t p_nrnpython_reg_real{};
char* hoc_back2forward(char* s);
char* hoc_forward2back(char* s);
#if DARWIN
extern void nrn_possible_mismatched_arch(const char*);
#endif

// following is undefined or else has the value of sys.api_version
// at time of configure (using the python first in the PATH).
#ifdef NRNPYTHON_DYNAMICLOAD
#include "nrnwrap_dlfcn.h"
extern char* neuron_home;
static void load_nrnpython();
#else
extern "C" int nrnpython_start(int);
extern "C" void nrnpython_reg_real();
extern "C" void nrnpython_real();
#endif

char* nrnpy_pyhome;

void nrnpython() {
#if USE_PYTHON
    if (p_nrnpython_real) {
        p_nrnpython_real();
        return;
    }
#endif
    hoc_retpushx(0.);
}

// Stub class for when Python does not exist
static void* p_cons(Object*) {
    return nullptr;
}
static void p_destruct(void*) {}
static Member_func p_members[] = {{nullptr, nullptr}};

#ifdef NRNPYTHON_DYNAMICLOAD
static char *nrnpy_pylib{}, *nrnpy_pyversion{};
static void set_nrnpylib() {
    nrnpy_pylib = getenv("NRN_PYLIB");
    nrnpy_pyhome = getenv("NRN_PYTHONHOME");
    nrnpy_pyversion = getenv("NRN_PYTHONVERSION");
    if (nrnpy_pylib && nrnpy_pyhome && nrnpy_pyversion) {
        return;
    }
    // copy allows free of the copy if needed
    if (nrnpy_pylib) {
        nrnpy_pylib = strdup(nrnpy_pylib);
    }
    if (nrnpy_pyhome) {
        nrnpy_pyhome = strdup(nrnpy_pyhome);
    }
    if (nrnpy_pyversion) {
        nrnpy_pyversion = strdup(nrnpy_pyversion);
    }

    if (nrnmpi_myid_world == 0) {
        int linesz = 1024 + (nrnpy_pyexe ? strlen(nrnpy_pyexe) : 0);
#ifdef MINGW
        linesz += 3 * strlen(neuron_home);
        char* line = new char[linesz + 1];
        char* bnrnhome = strdup(neuron_home);
        char* fnrnhome = strdup(neuron_home);
        hoc_forward2back(bnrnhome);
        hoc_back2forward(fnrnhome);
        std::snprintf(line,
                      linesz + 1,
                      "%s\\mingw\\usr\\bin\\bash %s/bin/nrnpyenv.sh %s --NEURON_HOME=%s",
                      bnrnhome,
                      fnrnhome,
                      (nrnpy_pyexe && strlen(nrnpy_pyexe) > 0) ? nrnpy_pyexe : "",
                      fnrnhome);
        free(fnrnhome);
        free(bnrnhome);
#else
        char* line = new char[linesz + 1];
        std::snprintf(line,
                      linesz + 1,
                      "bash %s/../../bin/nrnpyenv.sh %s",
                      neuron_home,
                      (nrnpy_pyexe && strlen(nrnpy_pyexe) > 0) ? nrnpy_pyexe : "");
#endif
        FILE* p = popen(line, "r");
        if (!p) {
            printf("could not popen '%s'\n", line);
        } else {
            while (fgets(line, linesz, p)) {
                char* cp;
                // must get rid of beginning '"' and trailing '"\n'
                if (!nrnpy_pyhome && (cp = strstr(line, "export NRN_PYTHONHOME="))) {
                    cp += strlen("export NRN_PYTHONHOME=") + 1;
                    cp[strlen(cp) - 2] = '\0';
                    nrnpy_pyhome = strdup(cp);
                } else if (!nrnpy_pylib && (cp = strstr(line, "export NRN_PYLIB="))) {
                    cp += strlen("export NRN_PYLIB=") + 1;
                    cp[strlen(cp) - 2] = '\0';
                    nrnpy_pylib = strdup(cp);
                } else if (!nrnpy_pyexe && (cp = strstr(line, "export NRN_PYTHONEXE="))) {
                    cp += strlen("export NRN_PYTHONEXE=") + 1;
                    cp[strlen(cp) - 2] = '\0';
                    nrnpy_pyexe = strdup(cp);
                } else if (!nrnpy_pyexe && (cp = strstr(line, "export NRN_PYTHONVERSION="))) {
                    cp += strlen("export NRN_PYTHONVERSION=") + 1;
                    cp[strlen(cp) - 2] = '\0';
                    nrnpy_pyversion = strdup(cp);
                }
            }
            if (std::ferror(p)) {
                // see if we terminated for some non-EOF reason
                std::ostringstream oss;
                oss << "reading from: " << line << " failed with error: " << std::strerror(errno)
                    << std::endl;
                throw std::runtime_error(oss.str());
            }
            pclose(p);
        }
        delete[] line;
    }
#if NRNMPI
    if (nrnmpi_numprocs_world > 1) {  // 0 broadcasts to everyone else.
        nrnmpi_char_broadcast_world(&nrnpy_pyexe, 0);
        nrnmpi_char_broadcast_world(&nrnpy_pylib, 0);
        nrnmpi_char_broadcast_world(&nrnpy_pyhome, 0);
        nrnmpi_char_broadcast_world(&nrnpy_pyversion, 0);
    }
#endif
}
#endif

void nrnpython_reg() {
    // printf("nrnpython_reg in nrnpy.cpp\n");
#if USE_PYTHON
    if (nrn_nopython) {
        p_nrnpython_start = nullptr;
        p_nrnpython_real = nullptr;
        p_nrnpython_reg_real = nullptr;
    } else {
#ifdef NRNPYTHON_DYNAMICLOAD
        void* handle = NULL;
        if (!nrn_is_python_extension) {
            // As last resort (or for python3) load $NRN_PYLIB
            set_nrnpylib();
            // printf("nrnpy_pylib %s\n", nrnpy_pylib);
            // printf("nrnpy_pyhome %s\n", nrnpy_pyhome);
            if (nrnpy_pylib) {
                handle = dlopen(nrnpy_pylib, RTLD_NOW | RTLD_GLOBAL);
                if (!handle) {
                    fprintf(stderr, "Could not dlopen NRN_PYLIB: %s\n", nrnpy_pylib);
#if DARWIN
                    nrn_possible_mismatched_arch(nrnpy_pylib);
#endif
                    exit(1);
                }
            }
        }
        if (handle || nrn_is_python_extension) {
            load_nrnpython();
        }
#else
        p_nrnpython_start = nrnpython_start;
        p_nrnpython_real = nrnpython_real;
        p_nrnpython_reg_real = nrnpython_reg_real;
#endif
    }
    if (p_nrnpython_reg_real) {
        p_nrnpython_reg_real();
        return;
    }
#endif
    class2oc("PythonObject", p_cons, p_destruct, p_members, nullptr, nullptr, nullptr);
}

#ifdef NRNPYTHON_DYNAMICLOAD  // to end of file
static void* load_sym(void* handle, const char* name) {
    void* p = dlsym(handle, name);
    if (!p) {
        printf("Could not load %s\n", name);
        exit(1);
    }
    return p;
}

static void load_nrnpython() {
    std::string pyversion{};
    if (int pv10 = nrn_is_python_extension; pv10 > 0) {
        // pv10 is one of the packed integers like 310 (3.10) or 38 (3.8)
        auto const factor = (pv10 >= 100) ? 100 : 10;
        pyversion = std::to_string(pv10 / factor) + "." + std::to_string(pv10 % factor);
    } else {
        if (!nrnpy_pylib || !nrnpy_pyversion) {
            std::cerr << "Do not know what Python to load [nrnpy_pylib="
                      << (nrnpy_pylib ? nrnpy_pylib : "nullptr")
                      << " nrnpy_pyversion=" << (nrnpy_pyversion ? nrnpy_pyversion : "nullptr")
                      << ']' << std::endl;
            return;
        }
        pyversion = nrnpy_pyversion;
    }
    std::string const name{neuron::config::shared_library_prefix + "nrnpython" + pyversion +
                           neuron::config::shared_library_suffix};
    auto const& supported_versions = neuron::config::supported_python_versions;
    auto const iter = std::find(supported_versions.begin(), supported_versions.end(), pyversion);
    if (iter == supported_versions.end()) {
        std::cerr << "This NEURON installation does not support the current Python version ("
                  << pyversion << "). Either re-build NEURON with support for this version, use "
                  << "a supported version of Python (";
        for (auto i = supported_versions.begin(); i != supported_versions.end(); ++i) {
            std::cerr << *i;
            if (std::next(i) != supported_versions.end()) {
                std::cerr << ", ";
            }
        }
        std::cerr << "), or try using nrniv -python so that NEURON can suggest a compatible "
                     "version for you. [pyver10="
                  << pyver10 << " pv10=" << pv10 << " pylib=" << (pylib ? pylib : "nullptr") << ']'
                  << std::endl;
        return;
    }
#ifndef MINGW
    // Build a path from neuron_home on macOS and Linux
    name = neuron_home + ("/../../lib/" + name);
#endif
    auto* const handle = dlopen(name.c_str(), RTLD_NOW);
    if (!handle) {
        std::cout << "Could not load " << name << std::endl;
        std::cout << "pyver10=" << pyver10 << " pylib=" << (pylib ? pylib : "(null)") << std::endl;
        return;
    }
    p_nrnpython_start = reinterpret_cast<nrnpython_start_t>(load_sym(handle, "nrnpython_start"));
    p_nrnpython_real = reinterpret_cast<nrnpython_real_t>(load_sym(handle, "nrnpython_real"));
    p_nrnpython_reg_real = reinterpret_cast<nrnpython_reg_real_t>(
        load_sym(handle, "nrnpython_reg_real"));
}

#endif
