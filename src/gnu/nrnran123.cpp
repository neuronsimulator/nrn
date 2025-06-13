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

/* deprecated */
nrnran123_State* nrnran123_newstream3(std::uint32_t id1, std::uint32_t id2, std::uint32_t id3) {
    return nrnran123_newstream(id1, id2, id3);
}

nrnran123_State* nrnran123_newstream() {
    extern int nrnmpi_myid;
    static std::uint32_t id3{};
    return nrnran123_newstream(1, nrnmpi_myid, ++id3);
}

nrnran123_State* nrnran123_newstream(std::uint32_t id1, std::uint32_t id2, std::uint32_t id3) {
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

/** @brief seq4which is 34 bit uint encoded as double(seq)*4 + which
 *  More convenient to get and set from interpreter
*/
void nrnran123_setseq(nrnran123_State* s, double seq4which) {
    if (seq4which < 0.0) {
        seq4which = 0.0;
    }
    if (seq4which > double(0XffffffffffLL)) {
        seq4which = 0.0;
    }
    // at least 64 bits even on 32 bit machine (could be more)
    unsigned long long x = ((unsigned long long) seq4which) & 0X3ffffffffLL;
    char which = x & 0X3;
    uint32_t seq = x >> 2;
    nrnran123_setseq(s, seq, which);
}

void nrnran123_getids(nrnran123_State* s, std::uint32_t* id1, std::uint32_t* id2) {
    *id1 = s->c[2];
    *id2 = s->c[3];
}

void nrnran123_getids(nrnran123_State* s,
                       std::uint32_t* id1,
                       std::uint32_t* id2,
                       std::uint32_t* id3) {
    *id3 = s->c[1];
    *id1 = s->c[2];
    *id2 = s->c[3];
}

/* Deprecated */
void nrnran123_getids3(nrnran123_State* s, 
                       std::uint32_t* id1,
                       std::uint32_t* id2,
                       std::uint32_t* id3) {
    nrnran123_getids(s, id1, id2, id3);
}

void nrnran123_setids(nrnran123_State* s, std::uint32_t id1, std::uint32_t id2, std::uint32_t id3) {
    s->c[1] = id3;
    s->c[2] = id1;
    s->c[3] = id2;
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

double nrnran123_uniform(nrnran123_State* s) {
    return nrnran123_dblpick(s);
}

double nrnran123_uniform(nrnran123_State* s, double a, double b) {
    return a + nrnran123_dblpick(s) * (b - a);
}

double nrnran123_negexp(nrnran123_State* s, double mean) {
    /* min 2.3283064e-10 to max 22.18071 (if mean is 1) */
    return -std::log(nrnran123_dblpick(s)) * mean;
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

    y = std::sqrt((-2. * std::log(w)) / w);
    x = u1 * y;
    return x;
}

double nrnran123_normal(nrnran123_State* s, double mu, double sigma) {
    return mu + nrnran123_normal(s) * sigma;
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
