#include "nrnpython_config.h"
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {

//int nrn_global_argc;
//char** nrn_global_argv;

extern void nrnpy_hoc();

extern int nrn_is_python_extension;
extern int ivocmain(int, char**, char**);
static char* argv[] = {"NEURON", "-dll", 0};
static char* env[] = {0};
void inithoc() {
	int argc = 1;
	char buf[200];
#if !defined(__CYGWIN__)
	sprintf(buf, "%s/.libs/libnrnmech.so", NRNHOSTCPU);
	//printf("buf = |%s|\n", buf);
	FILE* f;
	if ((f = fopen(buf, "r")) != 0) {
		fclose(f);
		argc = 3;
		argv[2] = new char[strlen(buf)+1];
		strcpy(argv[2], buf);
	}
#endif
	nrn_is_python_extension = 1;
	ivocmain(argc, argv, env);
	nrnpy_hoc();
}

void modl_reg() {}

}

