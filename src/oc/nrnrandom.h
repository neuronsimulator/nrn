#pragma once
#ifdef __cplusplus
#error "This version of nrnrandom.h should never be compiled as C++."
#endif
#include <stdint.h>

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
void nrn_set_random_sequence(void* r, long seq);
