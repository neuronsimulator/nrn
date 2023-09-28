#ifndef nrnisaac_h
#define nrnisaac_h

#include <../../nrnconf.h>
#include <stdint.h>


void* nrnisaac_new(void);
void nrnisaac_delete(void* rng);
void nrnisaac_init(void* rng, unsigned long int seed);
double nrnisaac_dbl_pick(void* rng);
uint32_t nrnisaac_uint32_pick(void* rng);


#endif
