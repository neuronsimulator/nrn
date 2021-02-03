#include <../../nrnconf.h>
#include <stdio.h>
#include <stdlib.h>
#include "hocdec.h"

extern "C" void _modl_set_dt(double newdt){
	/*ARGSUSED*/
	printf("ssimplic.cpp :: _modl_set_dt can't be called\n");
	nrn_exit(1);
}

