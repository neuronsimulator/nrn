#include "../oc/nrnmpiuse.h"
#include "../oc/nrnmpi.h"
#include "nrnpython_config.h"
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {

//int nrn_global_argc;
//char** nrn_global_argv;

extern void nrnpy_augment_path();
extern void nrnpy_hoc();

extern int nrn_is_python_extension;
extern int ivocmain(int, char**, char**);
#if NRNMPI
static char* argv[] = {"NEURON", "-mpi","-dll", 0};
static int argc = 2;
#else
static char* argv[] = {"NEURON", "-dll", 0};
static int argc = 1;
#endif
static char* env[] = {0};
void inithoc() {
	char buf[200];
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
	char **myargv = argv;
	nrnmpi_init(1, &argc, &myargv); // may change argc and argv
#endif		
	ivocmain(argc, argv, env);
	nrnpy_augment_path();
	nrnpy_hoc();
}

#if !defined(CYGWIN)
void modl_reg() {}
#endif
}
