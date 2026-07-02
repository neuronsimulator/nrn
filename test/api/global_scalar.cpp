// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_global_double_get / nrn_global_double_set across VAR subtypes.
// The interesting case is a NOTUSER runtime scalar (created by HOC `x = 42`),
// whose storage is not at sym->u.pval -- so nrn_symbol_dataptr cannot read it,
// but these accessors can.
#include <cmath>
#include <iostream>
#include "neuronapi.h"

using std::cerr;
using std::endl;

extern "C" void modl_reg(){/* No modl_reg */};

static bool check(bool cond, const char* msg) {
    if (!cond) {
        cerr << "FAIL: " << msg << endl;
    }
    return cond;
}

static bool eq(double got, double want, const char* msg) {
    if (std::fabs(got - want) > 1e-12) {
        cerr << "FAIL: " << msg << " — got " << got << ", want " << want << endl;
        return false;
    }
    return true;
}

int main(void) {
    static const char* argv[] = {"global_scalar", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv);

    bool ok = true;
    double v = -1.0;

    // NOTUSER: a runtime scalar. Its data lives in the top-level object-data
    // array, not at sym->u.pval, so this is the case the accessor exists for.
    nrn_hoc_call("myvar = 42");
    Symbol* mv = nrn_symbol("myvar");
    ok &= check(nrn_global_double_get(mv, &v) == 0, "NOTUSER get returns 0");
    ok &= eq(v, 42.0, "NOTUSER reads 42");

    ok &= check(nrn_global_double_set(mv, 3.5) == 0, "NOTUSER set returns 0");
    ok &= check(nrn_global_double_get(mv, &v) == 0, "NOTUSER re-get returns 0");
    ok &= eq(v, 3.5, "NOTUSER reads back the written 3.5");

    // The write must reach the same storage HOC reads: copy myvar into the
    // built-in hoc_ac_ from HOC, then read hoc_ac_ back through the accessor.
    nrn_hoc_call("hoc_ac_ = myvar");
    double ac = -1.0;
    nrn_global_double_get(nrn_symbol("hoc_ac_"), &ac);
    ok &= eq(ac, 3.5, "HOC sees the NOTUSER write");

    // USERDOUBLE built-in (t): round-trip.
    Symbol* t = nrn_symbol("t");
    ok &= check(nrn_global_double_set(t, 12.0) == 0, "USERDOUBLE set returns 0");
    ok &= check(nrn_global_double_get(t, &v) == 0, "USERDOUBLE get returns 0");
    ok &= eq(v, 12.0, "USERDOUBLE reads back 12");

    // Error contract: null symbol and a non-VAR (function) symbol return nonzero.
    ok &= check(nrn_global_double_get(nullptr, &v) != 0, "null symbol returns nonzero");
    ok &= check(nrn_global_double_get(nrn_symbol("topology"), &v) != 0,
                "non-VAR symbol returns nonzero");

    return ok ? 0 : 1;
}
