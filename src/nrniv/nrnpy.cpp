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

extern int nrn_nopython;
extern int nrnpy_nositeflag;
extern char* nrnpy_pyexe;
extern int nrn_is_python_extension;
int* nrnpy_site_problem_p;
extern int (*p_nrnpython_start)(int);
void nrnpython();
static void (*p_nrnpython_real)();
static void (*p_nrnpython_reg_real)();
char* hoc_back2forward(char* s);
char* hoc_forward2back(char* s);
#if DARWIN
extern void nrn_possible_mismatched_arch(const char*);
#endif

// following is undefined or else has the value of sys.api_version
// at time of configure (using the python first in the PATH).
#if defined(NRNPYTHON_DYNAMICLOAD)

#include "nrnwrap_dlfcn.h"
#if !defined(RTLD_NOLOAD)
#define RTLD_NOLOAD 0
#endif  // RTLD_NOLOAD

extern char* neuron_home;

#if NRNPYTHON_DYNAMICLOAD >= 30

#ifdef MINGW
static const char* ver[] = {"3.7", 0};
#else
static const char* ver[] = {"3.10", "3.9", "3.8", "3.7", 0};
#endif  // !MINGW

#else

static const char* ver[] = {0};

#endif

static int iver;  // which python is loaded?
static void* python_already_loaded();
static void* load_python();
static void load_nrnpython(int, const char*);
#else   //! defined(NRNPYTHON_DYNAMICLOAD)
extern "C" int nrnpython_start(int);
extern "C" void nrnpython_reg_real();
extern "C" void nrnpython_real();
#endif  // defined(NRNPYTHON_DYNAMICLOAD)

char* nrnpy_pyhome;

void nrnpython() {
#if USE_PYTHON
    if (p_nrnpython_real) {
        (*p_nrnpython_real)();
        return;
    }
#endif
    hoc_retpushx(0.);
}

// Stub class for when Python does not exist
static void* p_cons(Object*) {
    return 0;
}
static void p_destruct(void* v) {}
static Member_func p_members[] = {{0, 0}};

#if NRNPYTHON_DYNAMICLOAD
static char* nrnpy_pylib;

static void siteprob(void) {
    if (nrnpy_site_problem_p && (*nrnpy_site_problem_p)) {
        printf("Py_Initialize exited. PYTHONHOME probably needs to be set correctly.\n");
        if (nrnpy_pyhome) {
            printf(
                "The value of PYTHONHOME or our automatic guess based on the output of "
                "nrnpyenv.sh:\n    export PYTHONHOME=%s\ndid not work.\n",
                nrnpy_pyhome);
        }
        printf(
            "It will help to examine the output of:\nnrnpyenv.sh\n\
and set the indicated environment variables, or avoid python by adding\n\
nopython: on\n\
to %s/lib/nrn.defaults (or .nrn.defaults in your $HOME directory)\n",
            neuron_home);
    }
}

static void set_nrnpylib() {
    nrnpy_pylib = getenv("NRN_PYLIB");
    nrnpy_pyhome = getenv("NRN_PYTHONHOME");
    if (nrnpy_pylib && nrnpy_pyhome) {
        return;
    }
    // copy allows free of the copy if needed
    if (nrnpy_pylib) {
        nrnpy_pylib = strdup(nrnpy_pylib);
    }
    if (nrnpy_pyhome) {
        nrnpy_pyhome = strdup(nrnpy_pyhome);
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
        sprintf(line,
                "%s\\mingw\\usr\\bin\\bash %s/bin/nrnpyenv.sh %s --NEURON_HOME=%s",
                bnrnhome,
                fnrnhome,
                (nrnpy_pyexe && strlen(nrnpy_pyexe) > 0) ? nrnpy_pyexe : "",
                fnrnhome);
        free(fnrnhome);
        free(bnrnhome);
#else
        char* line = new char[linesz + 1];
        sprintf(line,
                "bash %s/../../bin/nrnpyenv.sh %s",
                neuron_home,
                (nrnpy_pyexe && strlen(nrnpy_pyexe) > 0) ? nrnpy_pyexe : "");
#endif
        FILE* p = popen(line, "r");
        if (!p) {
            printf("could not popen '%s'\n", line);
        } else {
            if (!fgets(line, linesz, p)) {
                printf("failed: %s\n", line);
            }
            while (fgets(line, linesz, p)) {
                char* cp;
                // must get rid of beginning '"' and trailing '"\n'
                if (!nrnpy_pyhome && (cp = strstr(line, "export NRN_PYTHONHOME="))) {
                    cp += strlen("export NRN_PYTHONHOME=") + 1;
                    cp[strlen(cp) - 2] = '\0';
                    if (nrnpy_pyhome) {
                        free(nrnpy_pyhome);
                    }
                    nrnpy_pyhome = strdup(cp);
                } else if (!nrnpy_pylib && (cp = strstr(line, "export NRN_PYLIB="))) {
                    cp += strlen("export NRN_PYLIB=") + 1;
                    cp[strlen(cp) - 2] = '\0';
                    if (nrnpy_pylib) {
                        free(nrnpy_pylib);
                    }
                    nrnpy_pylib = strdup(cp);
                }
            }
            pclose(p);
        }
        delete[] line;
    }
#if NRNMPI
    if (nrnmpi_numprocs_world > 1) {  // 0 broadcasts to everyone else.
        nrnmpi_char_broadcast_world(&nrnpy_pylib, 0);
        nrnmpi_char_broadcast_world(&nrnpy_pyhome, 0);
    }
#endif
}

#if 0
static void set_pythonhome(void* handle){
	if (nrnmpi_myid == 0) {atexit(siteprob);}
#ifdef MINGW
#else
	if (getenv("PYTHONHOME") || nrnpy_nositeflag) { return; }
	if (nrnpy_pyhome) {
		int res = setenv("PYTHONHOME", nrnpy_pyhome, 1);
		assert(res == 0);
		return;
	}

	Dl_info dl_info;
	void* s = dlsym(handle, "Py_Initialize");
        assert(s != NULL);
	int success = dladdr(s, &dl_info);
	if (success) {
		//printf("%s\n", dl_info.dli_fname);
		nrnpy_pyhome = strdup(dl_info.dli_fname);
		char* p = nrnpy_pyhome;
		int n = strlen(p);
		int seen = 0;
		for (int i = n-1; i > 0; --i) {
			if (p[i] == '/') {
				if (++seen >= 2) {
					p[i] = '\0' ;
					break;
				}
			}
		}
		int res = setenv("PYTHONHOME", p, 1);
		assert(res == 0);
	}
#endif
}
#endif  // if 0
#endif

void nrnpython_reg() {
    // printf("nrnpython_reg in nrnpy.cpp\n");
#if USE_PYTHON
    if (nrn_nopython) {
        p_nrnpython_start = 0;
        p_nrnpython_real = 0;
        p_nrnpython_reg_real = 0;
    } else {
#if NRNPYTHON_DYNAMICLOAD
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
            if (!handle) {
                python_already_loaded();
            }
            if (!handle) {  // embed python
                handle = load_python();
            }
#if 0
	// No longer do this as Py_SetPythonHome is used
	if (handle) {
		// need to worry about the site.py problem
		// can fix with a proper PYTHONHOME but need to know
		// what path was used to load the python library.
		set_pythonhome(handle);
	}
#endif
        } else {
            // printf("nrn_is_python_extension = %d\n", nrn_is_python_extension);
        }
        // for some mysterious reason on max osx 10.12
        // (perhaps due to System Integrity Protection?) when python is
        // launched, python_already_loaded() returns a NULL handle unless
        // the full path to the dylib is used. Since we know it is loaded
        // in these circumstances, it is sufficient to go ahead and dlopen
        // the nrnpython interface library
        if (handle || nrn_is_python_extension) {
            load_nrnpython(nrn_is_python_extension, nrnpy_pylib);
        }
#else
        p_nrnpython_start = nrnpython_start;
        p_nrnpython_real = nrnpython_real;
        p_nrnpython_reg_real = nrnpython_reg_real;
#endif
    }
    if (p_nrnpython_reg_real) {
        (*p_nrnpython_reg_real)();
        if (nrnpy_site_problem_p) {
            *nrnpy_site_problem_p = 1;
        }
        return;
    }
#endif
    class2oc("PythonObject", p_cons, p_destruct, p_members, NULL, NULL, NULL);
}

#if NRNPYTHON_DYNAMICLOAD  // to end of file

// important dlopen flags :
// RTLD_NOLOAD returns NULL if not open, or handle if it is resident.

static void* ver_dlo(int flag) {
    for (int i = 0; ver[i]; ++i) {
        char name[100];
#ifdef MINGW
        sprintf(name, "python%c%c.dll", ver[i][0], ver[i][2]);
#else
#if DARWIN
        sprintf(name, "libpython%s.dylib", ver[i]);
#else
        sprintf(name, "libpython%s.so", ver[i]);
#endif
#endif
        void* handle = dlopen(name, flag);
        iver = i;
        if (handle) {
            return handle;
        }
    }
    iver = -1;
    return NULL;
}

static void* python_already_loaded() {
    void* handle = ver_dlo(RTLD_NOW | RTLD_GLOBAL | RTLD_NOLOAD);
    // printf("python_already_loaded %d\n", iver);
    return handle;
}

static void* load_python() {
    void* handle = ver_dlo(RTLD_NOW | RTLD_GLOBAL);
    // printf("load_python %d\n", iver);
    return handle;
}

static void* load_sym(void* handle, const char* name) {
    void* p = dlsym(handle, name);
    if (!p) {
        printf("Could not load %s\n", name);
        exit(1);
    }
    return p;
}

static void* load_nrnpython_helper(const char* npylib) {
    char name[2048];
#ifdef MINGW
    sprintf(name, "%s.dll", npylib);
#else  // !MINGW
#if DARWIN
    sprintf(name, "%s/../../lib/%s.dylib", neuron_home, npylib);
#else   // !DARWIN
    sprintf(name, "%s/../../lib/%s.so", neuron_home, npylib);
#endif  // DARWIN
#endif  // MINGW
    void* handle = dlopen(name, RTLD_NOW);
    return handle;
}

// Get python version as integer from pythonlib path
static int pylib2pyver10(std::string pylib) {
    // skip past last \ or /
    const auto pos = pylib.find_last_of("/\\");
    if (pos != std::string::npos) {
        pylib = pylib.substr(pos + 1);
    }

    // erase nondigits
    pylib.erase(std::remove_if(pylib.begin(), pylib.end(), [](char c) { return !std::isdigit(c); }),
                pylib.end());

    // parse number. 0 is fine to return as error (no need for stoi)
    return std::atoi(pylib.c_str());
}

static void load_nrnpython(int pyver10, const char* pylib) {
    void* handle = NULL;
#if (defined(__MINGW32__) || \
     (defined(USE_LIBNRNPYTHON_MAJORMINOR) && USE_LIBNRNPYTHON_MAJORMINOR == 1))
    char name[256];
    int pv10 = pyver10;
    if (pyver10 < 1 && pylib) {
        pv10 = pylib2pyver10(pylib);
    }
    sprintf(name, "libnrnpython%d", pv10);
    handle = load_nrnpython_helper(name);
    if (!handle) {
        printf("Could not load %s\n", name);
        printf("pyver10=%d pylib=%s\n", pyver10, pylib ? pylib : "NULL");
        return;
    }
#else
    handle = load_nrnpython_helper("libnrnpython3");
    if (!handle) {
        printf("Could not load libnrnpython3\n");
        printf("pyver10=%d pylib=%s\n", pyver10, pylib ? pylib : "NULL");
        return;
    }
#endif
    p_nrnpython_start = (int (*)(int)) load_sym(handle, "nrnpython_start");
    p_nrnpython_real = (void (*)()) load_sym(handle, "nrnpython_real");
    p_nrnpython_reg_real = (void (*)()) load_sym(handle, "nrnpython_reg_real");
}

#endif
