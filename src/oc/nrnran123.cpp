#include <cstdint>
#include "nrnran123.h"
#include <cstdlib>
#include <cmath>
#include <Random123/philox.h>

using RNG = r123::Philox4x32;

static RNG::key_type k = {{0}};

struct nrnran123_State {
    RNG::ctr_type c;
    RNG::ctr_type r;
    char which_;
};

void nrnran123_set_globalindex(std::uint32_t gix) {
    k[0] = gix;
}

/* if one sets the global, one should reset all the stream sequences. */
std::uint32_t nrnran123_get_globalindex() {
    return k[0];
}

nrnran123_State* nrnran123_newstream(std::uint32_t id1, std::uint32_t id2) {
    return nrnran123_newstream3(id1, id2, 0);
}
nrnran123_State* nrnran123_newstream3(std::uint32_t id1, std::uint32_t id2, std::uint32_t id3) {
    auto* s = new nrnran123_State;
    s->c[1] = id3;
    s->c[2] = id1;
    s->c[3] = id2;
    nrnran123_setseq(s, 0, 0);
    return s;
}

void nrnran123_deletestream(nrnran123_State* s) {
    delete s;
}

void nrnran123_getseq(nrnran123_State* s, std::uint32_t* seq, char* which) {
    *seq = s->c[0];
    *which = s->which_;
}

void nrnran123_setseq(nrnran123_State* s, std::uint32_t seq, char which) {
    if (which > 3 || which < 0) {
        s->which_ = 0;
    } else {
        s->which_ = which;
    }
    s->c[0] = seq;
    s->r = philox4x32(s->c, k);
}

void nrnran123_getids(nrnran123_State* s, std::uint32_t* id1, std::uint32_t* id2) {
    *id1 = s->c[2];
    *id2 = s->c[3];
}

void nrnran123_getids3(nrnran123_State* s,
                       std::uint32_t* id1,
                       std::uint32_t* id2,
                       std::uint32_t* id3) {
    *id3 = s->c[1];
    *id1 = s->c[2];
    *id2 = s->c[3];
}

std::uint32_t nrnran123_ipick(nrnran123_State* s) {
    char which = s->which_;
    std::uint32_t rval = s->r[which++];
    if (which > 3) {
        which = 0;
        s->c.incr();
        s->r = philox4x32(s->c, k);
    }
    s->which_ = which;
    return rval;
}

double nrnran123_dblpick(nrnran123_State* s) {
    static const double SHIFT32 = 1.0 / 4294967297.0; /* 1/(2^32 + 1) */
    auto u = nrnran123_ipick(s);
    return ((double) u + 1.0) * SHIFT32;
}

double nrnran123_negexp(nrnran123_State* s) {
    /* min 2.3283064e-10 to max 22.18071 */
    return -std::log(nrnran123_dblpick(s));
}

/* At cost of a cached value we could compute two at a time. */
/* But that would make it difficult to transfer to coreneuron for t > 0 */
double nrnran123_normal(nrnran123_State* s) {
    double w, x, y;
    double u1, u2;
    do {
        u1 = nrnran123_dblpick(s);
        u2 = nrnran123_dblpick(s);
        u1 = 2. * u1 - 1.;
        u2 = 2. * u2 - 1.;
        w = (u1 * u1) + (u2 * u2);
    } while (w > 1);

    y = std::sqrt((-2. * log(w)) / w);
    x = u1 * y;
    return x;
}

nrnran123_array4x32 nrnran123_iran(std::uint32_t seq, std::uint32_t id1, std::uint32_t id2) {
    return nrnran123_iran3(seq, id1, id2, 0);
}
nrnran123_array4x32 nrnran123_iran3(std::uint32_t seq,
                                    std::uint32_t id1,
                                    std::uint32_t id2,
                                    std::uint32_t id3) {
    nrnran123_array4x32 a;
    RNG::ctr_type c;
    c[0] = seq;
    c[1] = id3;
    c[2] = id1;
    c[3] = id2;
    RNG::ctr_type r = philox4x32(c, k);
    a.v[0] = r[0];
    a.v[1] = r[1];
    a.v[2] = r[2];
    a.v[3] = r[3];
    return a;
}
