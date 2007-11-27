#include <../../nrnconf.h>
#include <stdio.h>
#include <stdlib.h>
_modl_set_dt(newdt) double newdt; {
	/*ARGSUSED*/
	printf("ssimplic.c :: _modl_set_dt can't be called\n");
	nrn_exit(1);
}

