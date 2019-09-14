#include <../../nrnconf.h>
#include "nrnmpiuse.h"
#include "nrnpthread.h"
#include <stdio.h>
#include <stdint.h>
#include "nrnmpi.h"
#include "nrnpython_config.h"
#if defined(__MINGW32__)
#define _hypot hypot
#endif
#include <Python.h>
#include <stdlib.h>

extern "C" {

// int nrn_global_argc;
extern char** nrn_global_argv;

extern void nrnpy_augment_path();
extern void (*p_nrnpython_finalize)();

#if PY_MAJOR_VERSION >= 3
extern PyObject* nrnpy_hoc();
#else
extern void nrnpy_hoc();
#endif

#if NRNMPI_DYNAMICLOAD
extern void nrnmpi_stubs();
extern char* nrnmpi_load(int is_python);
#endif
#if NRNPYTHON_DYNAMICLOAD
extern int nrnpy_site_problem;
#define HOCMOD(a, b) HOCMOD_(a, b)
#define HOCMOD_(a, b) a ## b
#else
#define HOCMOD(a, b) a
#endif

extern int nrn_is_python_extension;
extern int nrn_nobanner_;
extern int ivocmain(int, char**, char**);
extern int nrn_main_launch;

#if NRNMPI_DYNAMICLOAD
#include <dlfcn.h>
/**
 * Check for existance of MPI symbol in process memory space
 *
 * In case of dynamic mpi build we want to avoid loading MPI library
 * if program is being executed serially. is_mpi_loaded() function
 * helps to find out if MPI library is already loaded in process
 * memory space.
 */
#ifdef DARWIN
#include <mach-o/dyld.h>
/**
 * On OSX there is dl_iterate_phdr and hence we have to iterate
 * through all libraries loaded in process memory space and then
 * check if MPI symbol exist in each library.
 */
bool is_mpi_loaded() {
  bool result = false;
  uint32_t count = _dyld_image_count();
  for(uint32_t i = 0; i < count; i++) {
    void* handle = dlopen (_dyld_get_image_name(i), RTLD_LAZY);
    if (handle) {
      void* symbol = dlsym(handle, "MPI_Init");
      dlclose(handle);
      if (symbol != NULL)  {
        result = true;
        break;
      }
    }
  }
  return result;
}
#else
#include <link.h>
/**
 * On linix we can use dl_iterate_phdr by providing handle to function
 * that can check the existance of symbol using dlopen + dlsym. The
 * approach is same as OSX.
 */
static int is_mpi_loaded_impl(struct dl_phdr_info *info, size_t size, void *data) {
  int result = 0;
  void* handle = dlopen (info->dlpi_name, RTLD_LAZY);
  if (handle) {
    void* symbol = dlsym(handle, "MPI_Init");
    dlclose(handle);
    if (symbol != NULL)  {
      result = 1;
    }
  }
  return result;
}

bool is_mpi_loaded() {
    return dl_iterate_phdr(is_mpi_loaded_impl, NULL);
}
#endif //Darwin
#endif // NRNMPI_DYNAMICLOAD

#ifdef NRNMPI

static const char* argv_mpi[] = {"NEURON", "-mpi", "-dll", 0};
static int argc_mpi = 2;

#endif

static const char* argv_nompi[] = {"NEURON", "-dll", 0};
static int argc_nompi = 1;

#if USE_PTHREAD
#include <pthread.h>
static pthread_t main_thread_;
#endif

static void nrnpython_finalize() {
#if USE_PTHREAD
  pthread_t now = pthread_self();
  if (pthread_equal(main_thread_, now)) {
#else
  {
#endif
    Py_Finalize();
  }
#if linux
  if (system("stty sane > /dev/null 2>&1")){} // 'if' to avoid ignoring return value warning
#endif
}

static char* env[] = {0};

#if defined(__MINGW32__)

// The problem is that the hoc.dll name is the same for python2 and 3.
// The work around is to name them hoc27.dll and hoc36.dll when manually
// created by nrncygso.sh and use if version clauses in the neuron
// module to import the correct one as hoc.  Generalizable in the
// future to 27, 34, 35, 36 with try except clauses.
// It seems that since these dlls refer to python3x.dll explicitly,
// it is not possible for different minor versions of python3x to share
// hoc module dlls.
// It is conceivable that this strategy will work for linux and mac as well,
// but for now setup.py names them differently anyway.
#if PY_MAJOR_VERSION >= 3
PyObject* HOCMOD(PyInit_hoc, NRNPYTHON_DYNAMICLOAD)() {
#else //!PY_MAJOR_VERSION >= 3
void HOCMOD(inithoc, NRNPYTHON_DYNAMICLOAD)() {
#endif //!PY_MAJOR_VERSION >= 3

#else // ! defined __MINGW32__

#if PY_MAJOR_VERSION >= 3
PyObject* PyInit_hoc() {
#else //!PY_MAJOR_VERSION >= 3
void inithoc() {
#endif //!PY_MAJOR_VERSION >= 3

#endif // ! defined __MINGW32__

  char buf[200];

  int argc = argc_nompi;
  char** argv = (char**)argv_nompi;
#if USE_PTHREAD
  main_thread_ = pthread_self();
#endif

  if (nrn_global_argv) {  // ivocmain was already called so already loaded
#if PY_MAJOR_VERSION >= 3
    return nrnpy_hoc();
#else //!PY_MAJOR_VERSION >= 3
    nrnpy_hoc();
    return;
#endif //!PY_MAJOR_VERSION >= 3
  }
#ifdef NRNMPI

  int flag = 0;
  int mpi_mes = 0;  // for printing an mpi message only once.
  char* pmes = 0;

  /**
   * We decide to load MPI based on following condition:
   * - NEURON_INIT_MPI takes precedence
   *   - if set to 1 then initialize MPI (init_mpi == 2 means force mpi initialization)
   *   - if set to 0 then do not initialize MPI (init_mpi == 0 disable mpi initialization)
   * - if NEURON_INIT_MPI is not set then
   *   - initialize MPI if mpi library is loaded externally (e.g. from mpi4py)
   */
  char* env_mpi = getenv("NEURON_INIT_MPI");
  int init_mpi = 1;
  if (env_mpi != NULL) {
    if(strcmp(env_mpi, "0") == 0) {
      init_mpi = 0;
    } else if(strcmp(env_mpi, "1") == 0) {
      init_mpi = 2;
    }
  }

#if NRNMPI_DYNAMICLOAD
  nrnmpi_stubs();
  /**
   * In dynamic mpi build we load mpi if it is forced or if mpi
   * library has been already loaded.
   */
  if (init_mpi == 2 ||  (init_mpi == 1 && is_mpi_loaded())) {
    // if nrnmpi_load succeeds (MPI available), pmes is nil.
    pmes = nrnmpi_load(1);
    if (pmes) {
      printf(
        "NEURON_INIT_MPI exists in env but NEURON cannot initialize MPI "
        "because:\n%s\n",
        pmes);
      exit(1);
    } else {
      mpi_mes = 2;
      argc = argc_mpi;
      argv = (char**)argv_mpi;
    }
  } else {
    // no mpi
    mpi_mes = 3;
  }
#else
  // avoid having to include the c++ version of mpi.h
  if (!pmes) {
    nrnmpi_wrap_mpi_init(&flag);
  }
  /**
   * In case of normal non-dynamic mpi build
   *  - initialize mpi if user has not requested disabling (init_mpi != 0) and mpi is linked (flag = 1)
   *  - initialize mpi if user has forced initialization (init_mpi = 2)
   */
  if (flag && init_mpi != 0) {
    mpi_mes = 1;
    argc = argc_mpi;
    argv = (char**)argv_mpi;
  } else if (init_mpi == 2) {
    mpi_mes = 2;
    argc = argc_mpi;
    argv = (char**)argv_mpi;
  } else {
    //no mpi
    mpi_mes = 3;
  }
#endif

  if (pmes && mpi_mes == 2){exit(1);}  // avoid unused variable warning

#endif //NRNMPI

#if !defined(__CYGWIN__)
  sprintf(buf, "%s/.libs/libnrnmech.so", NRNHOSTCPU);
  // printf("buf = |%s|\n", buf);
  FILE* f;
  if ((f = fopen(buf, "r")) != 0) {
    fclose(f);
    argc += 2;
    argv[argc - 1] = new char[strlen(buf) + 1];
    strcpy(argv[argc - 1], buf);
  }
#endif // !defined(__CYGWIN__)
  nrn_is_python_extension = 1;
  nrn_nobanner_ = 1;
  const char* pyver = Py_GetVersion();
  nrn_is_python_extension = (pyver[0]-'0')*10 + (pyver[2] - '0');
  p_nrnpython_finalize = nrnpython_finalize;
#if NRNMPI
  nrnmpi_init(1, &argc, &argv);  // may change argc and argv
#if 0 && !defined(NRNMPI_DYNAMICLOAD)
	if (nrnmpi_myid == 0) {
		switch(mpi_mes) {
			case 0:
				break;
			case 1:
  printf("MPI_Initialized==true, MPI functionality enabled by Python.\n");
				break;
			case 2:
  printf("MPI functionality enabled by NEURON.\n");
				break;
			case 3:
  printf("MPI_Initialized==false, MPI functionality not enabled.\n");
				break;
		}
	}
#endif // 0 && !defined(NRNMPI_DYNAMICLOAD)
  if (pmes) {
    free(pmes);
  }
#endif // NRNMPI
  nrn_main_launch = 2;
  ivocmain(argc, argv, env);
//	nrnpy_augment_path();
#if NRNPYTHON_DYNAMICLOAD
  nrnpy_site_problem = 0;
#endif // NRNPYTHON_DYNAMICLOAD
#if PY_MAJOR_VERSION >= 3
  return nrnpy_hoc();
#else // ! PY_MAJOR_VERSION >= 3
  nrnpy_hoc();
#endif // ! PY_MAJOR_VERSION >= 3
}

#if !defined(CYGWIN)
void modl_reg() {}
#endif // !defined(CYGWIN)
}
