/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
to use the 0th counter uint32 as the stream sequence, words 2 and 3 as the
stream identifier, and word 0 of the key as the global index. Unused words
are constant uint32 0.

It is also possible to use Random123 directly without reference to this
interface. See Random123-1.02/docs/html/index.html
of the full distribution available from
http://www.deshawresearch.com/resources_random123.html
*/

#include "Random123/philox.h"
#include <inttypes.h>

#ifdef __CUDACC__
#define DEVICE __device__
#define GLOBAL __global__
#else
#define DEVICE
#define GLOBAL
#endif

#if defined(__CUDACC__) || defined(_OPENACC)
#define nrnran123_newstream cu_nrnran123_newstream
#define nrnran123_deletestream cu_nrnran123_deletestream
#define nrnran123_uint2dbl cu_nrnran123_uint2dbl
#define nrnran123_negexp cu_nrnran123_negexp
#define nrnran123_dblpick cu_nrnran123_dblpick
#define nrnran123_ipick cu_nrnran123_ipick
#define nrnran123_getids cu_nrnran123_getids
#define nrnran123_setseq cu_nrnran123_setseq
#define nrnran123_getseq cu_nrnran123_getseq
#define nrnran123_get_globalindex cu_nrnran123_get_globalindex
#define nrnran123_set_globalindex cu_nrnran123_set_globalindex
#define nrnran123_state_size cu_nrnran123_state_size
#define nrnran123_instance_count cu_nrnran123_instance_count
#define nrnran123_normal cu_nrnran123_normal
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct nrnran123_State {
    philox4x32_ctr_t c;
    philox4x32_ctr_t r;
    char which_;
} nrnran123_State;

typedef struct nrnran123_array4x32 {
        uint32_t v[4];
} nrnran123_array4x32;

/* do this on launch to make nrnran123_newstream threadsafe */
extern DEVICE void nrnran123_mutconstruct(void);

/* global index. eg. run number */
/* all generator instances share this global index */
extern DEVICE void nrnran123_set_globalindex(uint32_t gix);
extern DEVICE uint32_t nrnran123_get_globalindex();

extern DEVICE size_t nrnran123_instance_count(void);
extern DEVICE size_t nrnran123_state_size(void);

/* routines for creating and deleteing streams are called from cpu */
extern nrnran123_State* nrnran123_newstream(uint32_t id1, uint32_t id2);
extern void nrnran123_deletestream(nrnran123_State*);

/* routines for creating and deleteing streams are called from cpu but initializing/deleting gpu context */
extern nrnran123_State* cu_nrnran123_newstream(uint32_t id1, uint32_t id2);
extern void cu_nrnran123_deletestream(nrnran123_State*);

extern GLOBAL void nrnran123_setup_deletestream(nrnran123_State* s);
extern GLOBAL void nrnran123_setup_newstream(nrnran123_State *s, uint32_t id1, uint32_t id2);

/* minimal data stream */
extern DEVICE void nrnran123_getseq(nrnran123_State*, uint32_t* seq, unsigned char* which);
extern DEVICE void nrnran123_getids(nrnran123_State*, uint32_t* id1, uint32_t* id2);
extern DEVICE uint32_t nrnran123_ipick(nrnran123_State*); /* uniform 0 to 2^32-1 */

/* this could be called from openacc parallel construct */
#pragma acc routine seq
extern DEVICE double nrnran123_dblpick(nrnran123_State*); /* uniform open interval (0,1)*/
    /* nrnran123_dblpick minimum value is 2.3283064e-10 and max value is 1-min */

/* this could be called from openacc parallel construct (in INITIAL block) */
#pragma acc routine seq
extern DEVICE void nrnran123_setseq(nrnran123_State*, uint32_t seq, unsigned char which);

#pragma acc routine seq
extern DEVICE double nrnran123_negexp(nrnran123_State*);  /* mean 1.0 */
    /* nrnran123_negexp min value is 2.3283064e-10, max is 22.18071 */

/* missing declaration in coreneuron */
extern DEVICE double nrnran123_normal(nrnran123_State*);

extern DEVICE double nrnran123_gauss(nrnran123_State*); /* mean 0.0, std 1.0 */

/* more fundamental (stateless) (though the global index is still used) */
extern DEVICE nrnran123_array4x32 nrnran123_iran(uint32_t seq, uint32_t id1, uint32_t id2);
extern DEVICE double nrnran123_uint2dbl(uint32_t);

#if defined(__cplusplus)
}
#endif

#endif
