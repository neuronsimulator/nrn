#pragma once
#ifdef __cplusplus
#error "This version of nrnrandom.h should never be compiled as C++."
#endif
#include <stdint.h>

/** Ideally the prototypes below would take Rand* in place of void*, but this
 *  would break several existing models. We do, however, want to declare the
 *  Rand type, as this makes it possible to write (for example)
 *  nrn_random_pick((Rand*)some_rand) now, which will be required to suppress
 *  deprecation warnings in NEURON 9+ where nrn_random_pick(void*) is a
 *  [[deprecated]] overload and the non-deprecated version takes Rand*.
 */
typedef struct Rand Rand;

/** These declarations are wrong, in the sense that they are inconsistent with
 *  the definitions in ivocrand.cpp. This is an intentional, but possibly
 *  unwise, decision.
 */
long nrn_get_random_sequence(void* r);
void* nrn_random_arg(int);
int nrn_random_isran123(void* r, uint32_t* id1, uint32_t* id2, uint32_t* id3);
double nrn_random_pick(void* r);
void nrn_random_reset(void* r);
int nrn_random123_getseq(void* r, uint32_t* seq, char* which);
int nrn_random123_setseq(void* r, uint32_t seq, char which);

/** Note that in addition to having void* in place of Rand*, this has int in
 *  place of long.
 */
void nrn_set_random_sequence(void* r, int seq);
