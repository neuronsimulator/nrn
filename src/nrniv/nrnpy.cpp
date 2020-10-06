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

extern int nrn_nopython;
extern int nrnpy_nositeflag;
extern char* nrnpy_pyexe;
extern int nrn_is_python_extension;
int* nrnpy_site_problem_p;
extern void (*p_nrnpython_start)(int);
void nrnpython();
static void (*p_nrnpython_real)();
static void (*p_nrnpython_reg_real)();
char* hoc_back2forward(char* s);
char* hoc_forward2back(char* s);

// following is undefined or else has the value of sys.api_version
// at time of configure (using the python first in the PATH).
#if defined(NRNPYTHON_DYNAMICLOAD)

#if defined(NRNCMAKE)
// CMAKE installs libnrnpythonx.so not in <prefix>/x86_64/lib but <prefix>/lib
#undef NRNHOSTCPU
#define NRNHOSTCPU "."
#endif

#ifdef MINGW
#define RTLD_NOW 0
#define RTLD_GLOBAL 0
#define RTLD_NOLOAD 0
extern void* dlopen_noerr(const char* name, int mode);
#define dlopen dlopen_noerr
extern void* dlsym(void* handle, const char* name);
extern int dlclose(void* handle);
extern char* dlerror();
#else
//#define _GNU_SOURCE
#include <dlfcn.h>
#endif

extern char* neuron_home;

#if NRNPYTHON_DYNAMICLOAD >= 20 && NRNPYTHON_DYNAMICLOAD < 30

#ifdef MINGW
static const char* ver[] = {"2.7", 0};
#else
static const char* ver[] = {"2.7", "2.6", "2.5", 0};
#endif // !MINGW

#elif NRNPYTHON_DYNAMICLOAD >= 30

#ifdef MINGW
static const char* ver[] = {"3.5", 0};
#else
static const char* ver[] = {"3.6", "3.5", "3.4", 0};
#endif // !MINGW

#else //NRNPYTHON_DYNAMICLOAD < 20

static const char* ver[] = {0};

#endif //NRNPYTHON_DYNAMICLOAD < 20

static int iver; // which python is loaded?
static void* python_already_loaded();
static void* load_python();
static void load_nrnpython(int, const char*);
#else //!defined(NRNPYTHON_DYNAMICLOAD)
extern "C" void nrnpython_start(int);
extern "C" void nrnpython_reg_real();
extern "C" void nrnpython_real();
#endif //defined(NRNPYTHON_DYNAMICLOAD)

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
static void p_destruct(void* v) {
}
static Member_func p_members[] = {0,0};

#if NRNPYTHON_DYNAMICLOAD
static char* nrnpy_pylib;

static void siteprob(void) {
	if (nrnpy_site_problem_p && (*nrnpy_site_problem_p)) {
printf("Py_Initialize exited. PYTHONHOME probably needs to be set correctly.\n");
		if(nrnpy_pyhome) {
printf("The value of PYTHONHOME or our automatic guess based on the output of nrnpyenv.sh:\n    export PYTHONHOME=%s\ndid not work.\n", nrnpy_pyhome);
		}
printf("It will help to examine the output of:\nnrnpyenv.sh\n\
and set the indicated environment variables, or avoid python by adding\n\
nopython: on\n\
to %s/lib/nrn.defaults (or .nrn.defaults in your $HOME directory)\n",
neuron_home);
	}
}

static void set_nrnpylib() {
  nrnpy_pylib = getenv("NRN_PYLIB");
  nrnpy_pyhome = getenv("PYTHONHOME");
  if (nrnpy_pylib && nrnpy_pyhome) { return; }
  // copy allows free of the copy if needed
  if (nrnpy_pylib) { nrnpy_pylib = strdup(nrnpy_pylib); }
  if (nrnpy_pyhome) { nrnpy_pyhome = strdup(nrnpy_pyhome); }

  if (nrnmpi_myid_world == 0) {
    int linesz = 1024 + (nrnpy_pyexe ? strlen(nrnpy_pyexe) : 0);
    #ifdef MINGW
    linesz += 3*strlen(neuron_home);
    char* line = new char[linesz+1];
    char* bnrnhome = strdup(neuron_home);
    char* fnrnhome = strdup(neuron_home);
    hoc_forward2back(bnrnhome);
    hoc_back2forward(fnrnhome);
    sprintf(line, "%s\\mingw\\usr\\bin\\bash %s/bin/nrnpyenv.sh %s --NEURON_HOME=%s",
      bnrnhome,
      fnrnhome,
      (nrnpy_pyexe && strlen(nrnpy_pyexe) > 0) ? nrnpy_pyexe : "",
      fnrnhome);
    free(fnrnhome);
    free(bnrnhome);
    #else
    char* line = new char[linesz+1];
#if defined(NRNCMAKE)
    sprintf(line, "bash %s/../../bin/nrnpyenv.sh %s",
     neuron_home,
#else
    sprintf(line, "bash %s/../../%s/bin/nrnpyenv.sh %s",
     neuron_home, NRNHOSTCPU,
#endif
      (nrnpy_pyexe && strlen(nrnpy_pyexe) > 0) ? nrnpy_pyexe : "");
   #endif
    FILE* p = popen(line, "r");
    if (!p) {
      printf("could not popen '%s'\n", line);
    }else{
      if (!fgets(line, linesz, p)) {
        printf("failed: %s\n", line);
      }
      while(fgets(line, linesz, p)) {
        char* cp;
        // must get rid of beginning '"' and trailing '"\n'
        if (!nrnpy_pyhome && (cp = strstr(line, "export PYTHONHOME="))) {
          cp += 19;
          cp[strlen(cp) - 2] = '\0';
          if (nrnpy_pyhome) { free(nrnpy_pyhome); }
          nrnpy_pyhome = strdup(cp);
        }else if (!nrnpy_pylib && (cp = strstr(line, "export NRN_PYLIB="))) {
          cp += 18;
          cp[strlen(cp) - 2] = '\0';
          if (nrnpy_pylib) { free(nrnpy_pylib); }
          nrnpy_pylib = strdup(cp);
        }
      }
      pclose(p);
    }
    delete [] line;
  }
#if NRNMPI
  if (nrnmpi_numprocs_world > 1) { // 0 broadcasts to everyone else.
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
#endif // if 0
#endif

void nrnpython_reg() {
	//printf("nrnpython_reg in nrnpy.cpp\n");
#if USE_PYTHON
    if (nrn_nopython) {
	p_nrnpython_start = 0;
	p_nrnpython_real = 0;
	p_nrnpython_reg_real = 0;
    }else{
#if NRNPYTHON_DYNAMICLOAD
	void* handle = NULL;

      if (!nrn_is_python_extension) {
	// As last resort (or for python3) load $NRN_PYLIB
	set_nrnpylib();
	//printf("nrnpy_pylib %s\n", nrnpy_pylib);
	//printf("nrnpy_pyhome %s\n", nrnpy_pyhome);
	if (nrnpy_pylib) {
		handle = dlopen(nrnpy_pylib, RTLD_NOW|RTLD_GLOBAL);
		if (!handle) {
			fprintf(stderr, "Could not dlopen NRN_PYLIB: %s\n", nrnpy_pylib);
			exit(1);
		}
	}
	if (!handle) { python_already_loaded();}
	if (!handle) { // embed python
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
      }else{
        //printf("nrn_is_python_extension = %d\n", nrn_is_python_extension);
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

#if NRNPYTHON_DYNAMICLOAD // to end of file

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
	void* handle = ver_dlo(RTLD_NOW|RTLD_GLOBAL|RTLD_NOLOAD);
	//printf("python_already_loaded %d\n", iver);
	return handle;
}

static void* load_python() {
	void* handle = ver_dlo(RTLD_NOW|RTLD_GLOBAL);
	//printf("load_python %d\n", iver);
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
#else // !MINGW
#if DARWIN
#if defined(NRNCMAKE)
	sprintf(name, "%s/../../lib/%s.dylib", neuron_home, npylib);
#else // !NRNCMAKE
	sprintf(name, "%s/../../%s/lib/%s.dylib", neuron_home, NRNHOSTCPU, npylib);
#endif // NRNCMAKE
#else // !DARWIN
#if defined(NRNCMAKE)
	sprintf(name, "%s/../../lib/%s.so", neuron_home, npylib);
#else // !NRNCMAKE
	sprintf(name, "%s/../../%s/lib/%s.so", neuron_home, NRNHOSTCPU, npylib);
#endif // NRNCMAKE
#endif // DARWIN
#endif // MINGW
	void* handle = dlopen(name, RTLD_NOW);
	return handle;
}

int digit_to_int(char ch) {
  int d = ch - '0';
  if ((unsigned) d < 10) {
    return d;
  }
  d = ch - 'a';
  if ((unsigned) d < 6) {
    return d + 10;
  }
  d = ch - 'A';
  if ((unsigned) d < 6) {
    return d + 10;
  }
  return -1;
}

static int pylib2pyver10(const char* pylib) {
  // check backwards for N.N or NN // obvious limitations
  int n1 = -1; int n2 = -1;
  for (const char* cp = pylib + strlen(pylib) -1 ; cp > pylib; --cp) {
    if (isdigit(*cp)) {
      if (n2 < 0) {
        n2 = digit_to_int(*cp);
      } else {
        n1 = digit_to_int(*cp);
        return n1*10 + n2;
      }
    }else if (*cp == '.') {
      // skip
    }else{ //
      // start over
      n2 = -1;
    }
  }
  return 0;
}

static void load_nrnpython(int pyver10, const char* pylib) {
	void* handle = NULL;
#if (defined(__MINGW32__) || (defined(USE_LIBNRNPYTHON_MAJORMINOR) && USE_LIBNRNPYTHON_MAJORMINOR == 1))
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
        handle = load_nrnpython_helper("libnrnpython2");
        if (!handle) {
            printf("Could not load either libnrnpython3 or libnrnpython2\n");
            printf("pyver10=%d pylib=%s\n", pyver10, pylib ? pylib : "NULL");
            return;
        }
    }
#endif
	p_nrnpython_start = (void(*)(int))load_sym(handle, "nrnpython_start");
	p_nrnpython_real = (void(*)())load_sym(handle, "nrnpython_real");
	p_nrnpython_reg_real = (void(*)())load_sym(handle, "nrnpython_reg_real");
}

#endif
