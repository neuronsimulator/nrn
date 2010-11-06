#include "nrnmpiuse.h"
#include <stdio.h>
#include "nrnmpi.h"
#include "nrnpython_config.h"
#include <Python.h>
#include <stdlib.h>


extern "C" {

//int nrn_global_argc;
extern char** nrn_global_argv;

extern void nrnpy_augment_path();

#if PY_MAJOR_VERSION >= 3
extern PyObject* nrnpy_hoc();
#else
extern void nrnpy_hoc();
#endif

extern int nrn_is_python_extension;
extern int ivocmain(int, char**, char**);

#ifdef NRNMPI

static char* argv_mpi[] = {"NEURON", "-mpi","-dll", 0};
static int argc_mpi = 2;
extern int nrn_wrap_mpi_init(int*);

#endif

static char* argv_nompi[] = {"NEURON", "-dll", 0};
static int argc_nompi = 1;

static char* env[] = {0};
#if PY_MAJOR_VERSION >= 3
PyObject* PyInit_hoc() {
#else
void inithoc() {
#endif
	char buf[200];
	
	int argc = argc_nompi;
	char** argv = argv_nompi;

	if (nrn_global_argv) { //ivocmain was already called so already loaded
#if PY_MAJOR_VERSION >= 3
	return nrnpy_hoc();
#else
		nrnpy_hoc();
		return;
#endif
	}
#ifdef NRNMPI

	int flag;

	// avoid having to include the c++ version of mpi.h
	nrn_wrap_mpi_init(&flag);
	//MPI_Initialized(&flag);

	if (flag) {
	  printf("MPI_Initialized==true, enabling MPI functionality.\n");

	  argc = argc_mpi;
	  argv = argv_mpi;
	}
	else {
	  printf("MPI_Initialized==false, disabling MPI functionality.\n");
	}

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
#if NRNMPI
	nrnmpi_init(1, &argc, &argv); // may change argc and argv
#endif		
	ivocmain(argc, argv, env);
	nrnpy_augment_path();
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
