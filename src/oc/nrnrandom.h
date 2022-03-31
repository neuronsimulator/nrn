#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct Rand Rand;
long nrn_get_random_sequence(Rand* r);
Rand* nrn_random_arg(int);
int nrn_random_isran123(Rand* r, uint32_t* id1, uint32_t* id2, uint32_t* id3);
double nrn_random_pick(Rand* r);
void nrn_random_reset(Rand* r);
int nrn_random123_getseq(Rand* r, uint32_t* seq, char* which);
int nrn_random123_setseq(Rand* r, uint32_t seq, char which);
void nrn_set_random_sequence(Rand* r, long seq);
#ifdef __cplusplus
}
#endif
