#include "nrnmpiuse.h"
#include <stdio.h>
#include <stdint.h>
#include "nrnmpi.h"
#include "nrnpython_config.h"
#include <Python.h>
#include <stdlib.h>


extern "C" {

//int nrn_global_argc;
extern char** nrn_global_argv;

extern void nrnpy_augment_path();
extern void(*p_nrnpython_finalize)();

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
#endif

extern int nrn_is_python_extension;
extern int ivocmain(int, char**, char**);
extern int nrn_main_launch;

#ifdef NRNMPI

static const char* argv_mpi[] = {"NEURON", "-mpi","-dll", 0};
static int argc_mpi = 2;

#endif

static const char* argv_nompi[] = {"NEURON", "-dll", 0};
static int argc_nompi = 1;

static void nrnpython_finalize() {
  Py_Finalize();
#if linux
  system("stty sane");
#endif
}

static char* env[] = {0};
#if PY_MAJOR_VERSION >= 3
PyObject* PyInit_hoc() {
#else
void inithoc() {
#endif
	char buf[200];
	
	int argc = argc_nompi;
	char** argv = (char**)argv_nompi;

	if (nrn_global_argv) { //ivocmain was already called so already loaded
#if PY_MAJOR_VERSION >= 3
	return nrnpy_hoc();
#else
		nrnpy_hoc();
		return;
#endif
	}
#ifdef NRNMPI

	int flag = 0;
	int mpi_mes = 0; // for printing an mpi message only once.
	char* pmes = 0;

#if NRNMPI_DYNAMICLOAD
	nrnmpi_stubs();
	// if nrnmpi_load succeeds (MPI available), pmes is nil.
	pmes = nrnmpi_load(1);
#endif

	// avoid having to include the c++ version of mpi.h
	if (!pmes) {nrnmpi_wrap_mpi_init(&flag);}
	//MPI_Initialized(&flag);

	if (flag) {
	  mpi_mes = 1;

	  argc = argc_mpi;
	  argv = (char**)argv_mpi;
	} else if (getenv ("NEURON_INIT_MPI")) {
	  // force NEURON to initialize MPI
	  mpi_mes = 2;
	 if (pmes) {
printf("NEURON_INIT_MPI exists in env but NEURON cannot initialize MPI because:\n%s\n", pmes);
		exit(1);
	 }else{
	  argc = argc_mpi;
	  argv = (char**)argv_mpi;
	 }
	} else {
	  mpi_mes = 3;
	}
	assert(mpi_mes != -1); //avoid unused variable warning

#endif

#if !defined(__CYGWIN__)
	sprintf(buf, "%s/.libs/libnrnmech.so", NRNHOSTCPU);
	//printf("buf = |%s|\n", buf);
	FILE* f;
	if ((f = fopen(buf, "r")) != 0) {
		fclose(f);
		argc += 2;
		argv[argc-1] = new char[strlen(buf)+1];
		strcpy(argv[argc-1], buf);
	}
#endif
	nrn_is_python_extension = 1;
	p_nrnpython_finalize = nrnpython_finalize;
#if NRNMPI
	nrnmpi_init(1, &argc, &argv); // may change argc and argv
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
#endif
	if (pmes) { free(pmes); }
#endif		
	nrn_main_launch = 2;
	ivocmain(argc, argv, env);
//	nrnpy_augment_path();
#if NRNPYTHON_DYNAMICLOAD
	nrnpy_site_problem = 0;
#endif
#if PY_MAJOR_VERSION >= 3
	return nrnpy_hoc();
#else
	nrnpy_hoc();
#endif
}

#if !defined(CYGWIN)
void modl_reg() {}
#endif
}
