#include <../../nrnconf.h>

/*
allocate and free property data and Datum arrays for nrnoc
the nrniv/cxprop.cpp version allows for the possibility of
greater cache efficiency
*/

#include <stdio.h>
#include <section.h>
#include <stdlib.h>

void nrn_mk_prop_pools(int n) {}
void nrn_cache_prop_realloc() {}
int nrn_is_valid_section_ptr(void* v) { return 0; }

double* nrn_prop_data_alloc(int type, int count, Prop* p) {
	double* pd = (double*)hoc_Ecalloc(count, sizeof(double)); hoc_malchk();
	return pd;
}
Datum* nrn_prop_datum_alloc(int type, int count, Prop* p) {
	Datum* ppd = (Datum*)hoc_Ecalloc(count, sizeof(Datum)); hoc_malchk();
	return ppd;
}
void nrn_prop_data_free(int type, double* pd) {
	if (pd) {
		free(pd);
	}
}
void nrn_prop_datum_free(int type, Datum* ppd) {
	if (ppd) {
		free(ppd);
	}
}
Section* nrn_section_alloc() {
	return (Section *)emalloc(sizeof(Section));
}
void nrn_section_free(Section* s) {
	free(s);
}

