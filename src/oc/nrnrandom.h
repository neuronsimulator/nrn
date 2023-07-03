#pragma once
#include <stdint.h>

class Rand;
long nrn_get_random_sequence(Rand* r);
Rand* nrn_random_arg(int);
int nrn_random_isran123(Rand* r, uint32_t* id1, uint32_t* id2, uint32_t* id3);
double nrn_random_pick(Rand* r);
void nrn_random_reset(Rand* r);
int nrn_random123_getseq(Rand* r, uint32_t* seq, char* which);
int nrn_random123_setseq(Rand* r, uint32_t seq, char which);
void nrn_set_random_sequence(Rand* r, long seq);

[[deprecated("non-void* overloads are preferred")]] long nrn_get_random_sequence(void* r);
[[deprecated("non-void* overloads are preferred")]] int nrn_random_isran123(void* r,
                                                                            uint32_t* id1,
                                                                            uint32_t* id2,
                                                                            uint32_t* id3);
[[deprecated("non-void* overloads are preferred")]] double nrn_random_pick(void* r);
[[deprecated("non-void* overloads are preferred")]] void nrn_random_reset(void* r);
[[deprecated("non-void* overloads are preferred")]] int nrn_random123_getseq(void* r,
                                                                             uint32_t* seq,
                                                                             char* which);
[[deprecated("non-void* overloads are preferred")]] int nrn_random123_setseq(void* r,
                                                                             uint32_t seq,
                                                                             char which);
// Note that in addition to having void* in place of Rand*, this has int in place of long.
[[deprecated("non-void* overloads are preferred")]] void nrn_set_random_sequence(void* r, int seq);
