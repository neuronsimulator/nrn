#include <../../nrnconf.h>
#include <stdlib.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <nrnisaac.h>
#include <isaac64.h>
#include "hocdec.h"

typedef struct isaac64_state Rng;

void* nrnisaac_new(void) {
	Rng* rng;
	rng = (Rng*)hoc_Emalloc(sizeof(Rng)); hoc_malchk();
	return (void*)rng;
}

void nrnisaac_delete(void* v) {
	free(v);
}

void nrnisaac_init(void* v, unsigned long int seed) {
	isaac64_init((Rng*)v, seed);
}

double nrnisaac_dbl_pick(void* v) {
	Rng* rng = (Rng*)v;
	double x = isaac64_dbl32(rng);
/*printf("dbl %d %d %d %d %g\n", sizeof(ub8), sizeof(ub4), sizeof(ub2), sizeof(ub1), x);*/
	return x;
}

uint32_t nrnisaac_uint32_pick(void* v) {
	Rng* rng = (Rng*)v;
	double x = isaac64_uint32(rng);
/*printf("uint32 %g\n", x);*/
	return x;
}


