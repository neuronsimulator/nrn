#include <../../nrnconf.h>
#include <inttypes.h>
#include <nrnran123.h>
#include <hocdec.h>
#include <stdlib.h>
#include <math.h>
#include <Random123/philox.h>

static const double SHIFT32 = 1.0 / 4294967297.0; /* 1/(2^32 + 1) */

static philox4x32_key_t k = {{0}};

struct nrnran123_State {
    philox4x32_ctr_t c;
    philox4x32_ctr_t r;
    char which_;
};

void nrnran123_set_globalindex(uint32_t gix) {
    k.v[0] = gix;
}

/* if one sets the global, one should reset all the stream sequences. */
uint32_t nrnran123_get_globalindex() {
    return k.v[0];
}

extern "C" nrnran123_State* nrnran123_newstream(uint32_t id1, uint32_t id2) {
    return nrnran123_newstream3(id1, id2, 0);
}
extern "C" nrnran123_State* nrnran123_newstream3(uint32_t id1, uint32_t id2, uint32_t id3) {
    nrnran123_State* s;
    s = (nrnran123_State*) ecalloc(sizeof(nrnran123_State), 1);
    s->c.v[1] = id3;
    s->c.v[2] = id1;
    s->c.v[3] = id2;
    nrnran123_setseq(s, 0, 0);
    return s;
}

extern "C" void nrnran123_deletestream(nrnran123_State* s) {
    free(s);
}

void nrnran123_getseq(nrnran123_State* s, uint32_t* seq, char* which) {
    *seq = s->c.v[0];
    *which = s->which_;
}

extern "C" void nrnran123_setseq(nrnran123_State* s, uint32_t seq, char which) {
    if (which > 3 || which < 0) {
        s->which_ = 0;
    } else {
        s->which_ = which;
    }
    s->c.v[0] = seq;
    s->r = philox4x32(s->c, k);
}

extern "C" void nrnran123_getids(nrnran123_State* s, uint32_t* id1, uint32_t* id2) {
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

extern "C" void nrnran123_getids3(nrnran123_State* s, uint32_t* id1, uint32_t* id2, uint32_t* id3) {
    *id3 = s->c.v[1];
    *id1 = s->c.v[2];
    *id2 = s->c.v[3];
}

extern "C" uint32_t nrnran123_ipick(nrnran123_State* s) {
    uint32_t rval;
    char which = s->which_;
    assert(which < 4);
    rval = s->r.v[which++];
    if (which > 3) {
        which = 0;
        s->c.v[0]++;
        s->r = philox4x32(s->c, k);
    }
    s->which_ = which;
    return rval;
}

extern "C" double nrnran123_dblpick(nrnran123_State* s) {
    return nrnran123_uint2dbl(nrnran123_ipick(s));
}

extern "C" double nrnran123_negexp(nrnran123_State* s) {
    /* min 2.3283064e-10 to max 22.18071 */
    return -log(nrnran123_dblpick(s));
}

/* At cost of a cached  value we could compute two at a time. */
/* But that would make it difficult to transfer to coreneuron for t > 0 */
extern "C" double nrnran123_normal(nrnran123_State* s) {
    double w, x, y;
    double u1, u2;
    do {
        u1 = nrnran123_dblpick(s);
        u2 = nrnran123_dblpick(s);
        u1 = 2. * u1 - 1.;
        u2 = 2. * u2 - 1.;
        w = (u1 * u1) + (u2 * u2);
    } while (w > 1);

    y = sqrt((-2. * log(w)) / w);
    x = u1 * y;
    return x;
}

nrnran123_array4x32 nrnran123_iran(uint32_t seq, uint32_t id1, uint32_t id2) {
    return nrnran123_iran3(seq, id1, id2, 0);
}
nrnran123_array4x32 nrnran123_iran3(uint32_t seq, uint32_t id1, uint32_t id2, uint32_t id3) {
    nrnran123_array4x32 a;
    philox4x32_ctr_t c;
    c.v[0] = seq;
    c.v[1] = id3;
    c.v[2] = id1;
    c.v[3] = id2;
    philox4x32_ctr_t r = philox4x32(c, k);
    a.v[0] = r.v[0];
    a.v[1] = r.v[1];
    a.v[2] = r.v[2];
    a.v[3] = r.v[3];
    return a;
}

double nrnran123_uint2dbl(uint32_t u) {
    /* 0 to 2^32-1 transforms to double value in open (0,1) interval */
    /* min 2.3283064e-10 to max (1 - 2.3283064e-10) */
    return ((double) u + 1.0) * SHIFT32;
}
