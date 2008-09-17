#include "../oc/nrnmpiuse.h"
#include "../oc/nrnmpi.h"
#include "nrnpython_config.h"
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef NRNMPI

#undef SEEK_SET
#undef SEEK_END
#undef SEEK_CUR

#include <mpi.h>

#endif


extern "C" {

//int nrn_global_argc;
//char** nrn_global_argv;

extern void nrnpy_augment_path();
extern void nrnpy_hoc();

extern int nrn_is_python_extension;
extern int ivocmain(int, char**, char**);

#ifdef NRNMPI

static char* argv_mpi[] = {"NEURON", "-mpi","-dll", 0};
static int argc_mpi = 2;

#endif

static char* argv_nompi[] = {"NEURON", "-dll", 0};
static int argc_nompi = 1;

static char* env[] = {0};
void inithoc() {
	char buf[200];
	
	int argc = argc_nompi;
	char** argv = argv_nompi;

#ifdef NRNMPI

	int flag;

	MPI_Initialized(&flag);

	if (flag) {
	  printf("MPI_Initialized==true, enabling MPI funtionality.\n");

	  argc = argc_mpi;
	  argv = argv_mpi;
	}
	else {
	  printf("MPI_Initialized==false, disabling MPI funtionality.\n");
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
	nrnpy_hoc();
}

#if !defined(CYGWIN)
void modl_reg() {}
#endif
}
