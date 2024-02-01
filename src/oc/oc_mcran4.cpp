#include "hocdec.h"
#include "mcran4.h"

int use_mcell_ran4_;

void set_use_mcran4(bool value) {
    use_mcell_ran4_ = value ? 1 : 0;
}

bool use_mcran4() {
    return use_mcell_ran4_ != 0;
}

void hoc_mcran4() {
    uint32_t idx;
    double* xidx;
    double x;
    xidx = hoc_pgetarg(1);
    idx = (uint32_t) (*xidx);
    x = mcell_ran4a(&idx);
    *xidx = idx;
    hoc_ret();
    hoc_pushx(x);
}
void hoc_mcran4init() {
    double prev = mcell_lowindex();
    if (ifarg(1)) {
        uint32_t idx = (uint32_t) chkarg(1, 0., 4294967295.);
        mcell_ran4_init(idx);
    }
    hoc_ret();
    hoc_pushx(prev);
}
void hoc_usemcran4() {
    double prev = (double) use_mcell_ran4_;
    if (ifarg(1)) {
        use_mcell_ran4_ = (int) chkarg(1, 0., 1.);
    }
    hoc_ret();
    hoc_pushx(prev);
}
