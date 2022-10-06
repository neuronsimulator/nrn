#ifndef nrnran123_h
#define nrnran123_h

/* interface to Random123 */
/* http://www.thesalmons.org/john/random123/papers/random123sc11.pdf */

/*
The 4x32 generators utilize a uint32x4 counter and uint32x4 key to transform
into an almost cryptographic quality uint32x4 random result.
There are many possibilites for balancing the sharing of the internal
state instances while reserving a uint32 counter for the stream sequence
and reserving other portions of the counter vector for stream identifiers
and global index used by all streams.

We currently provide a single instance by default in which the policy is
to use the 0th counter uint32 as the stream sequence, words 2, 3 and 4 as the
stream identifier, and word 0 of the key as the global index. Unused words
are constant uint32 0.

It is also possible to use Random123 directly without reference to this
interface. See Random123-1.02/docs/html/index.html
of the full distribution available from
http://www.deshawresearch.com/resources_random123.html
*/

#include <inttypes.h>


typedef struct nrnran123_State nrnran123_State;

typedef struct nrnran123_array4x32 {
    uint32_t v[4];
} nrnran123_array4x32;

/* global index. eg. run number */
/* all generator instances share this global index */
extern void nrnran123_set_globalindex(uint32_t gix);
extern uint32_t nrnran123_get_globalindex();

/* minimal data stream */
extern nrnran123_State* nrnran123_newstream(uint32_t id1, uint32_t id2);
extern nrnran123_State* nrnran123_newstream3(uint32_t id1, uint32_t id2, uint32_t id3);
extern void nrnran123_deletestream(nrnran123_State*);
extern void nrnran123_getseq(nrnran123_State*, uint32_t* seq, char* which);
extern void nrnran123_setseq(nrnran123_State*, uint32_t seq, char which);
extern void nrnran123_getids(nrnran123_State*, uint32_t* id1, uint32_t* id2);
extern void nrnran123_getids3(nrnran123_State*, uint32_t* id1, uint32_t* id2, uint32_t* id3);

extern double nrnran123_negexp(nrnran123_State*);  /* mean 1.0 */
extern uint32_t nrnran123_ipick(nrnran123_State*); /* uniform 0 to 2^32-1 */
extern double nrnran123_dblpick(nrnran123_State*); /* uniform open interval (0,1)*/
/* nrnran123_dblpick minimum value is 2.3283064e-10 and max value is 1-min */

/* nrnran123_negexp min value is 2.3283064e-10, max is 22.18071 */
extern double nrnran123_normal(nrnran123_State*); /* mean 0.0, std 1.0 */

/* more fundamental (stateless) (though the global index is still used) */
extern nrnran123_array4x32 nrnran123_iran(uint32_t seq, uint32_t id1, uint32_t id2);
extern nrnran123_array4x32 nrnran123_iran3(uint32_t seq, uint32_t id1, uint32_t id2, uint32_t id3);
extern double nrnran123_uint2dbl(uint32_t);

#endif
