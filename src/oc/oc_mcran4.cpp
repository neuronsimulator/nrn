#include "hocdec.h"
#include "nrnran123.h"

static nrnran123_State* state = nrnran123_newstream(0, 0);

void hoc_mcran4() {
    uint32_t idx;
    double* xidx;
    double x;
    xidx = hoc_pgetarg(1);
    idx = (uint32_t) (*xidx);
    nrnran123_setseq(state, idx, 0);
    x = nrnran123_dblpick(state);
    *xidx = idx + 1;
    hoc_ret();
    hoc_pushx(x);
}
void hoc_mcran4init() {
    std::uint32_t seq;
    char which;
    nrnran123_getseq(state, &seq, &which);
    double prev = static_cast<double>(seq);
    if (ifarg(1)) {
        uint32_t idx = (uint32_t) chkarg(1, 0., 4294967295.);
        nrnran123_setseq(state, idx, 0);
    }
    hoc_ret();
    hoc_pushx(prev);
}
void hoc_usemcran4() {
    hoc_ret();
    hoc_pushx(0);
}
