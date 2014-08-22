#include <../../nrnconf.h>
#include <stdio.h>
#include <stdlib.h>
#include "hocdec.h"

void _modl_set_dt(newdt) double newdt; {
	/*ARGSUSED*/
	printf("ssimplic.c :: _modl_set_dt can't be called\n");
	nrn_exit(1);
}

