// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_section_length_set under pt3dconst. While pt3dconst(1) is in
// effect, HOC and Python silently ignore sec.L = x on every section, with or
// without 3-d points. The C setter must do the same, and must work as usual
// once pt3dconst(0) is restored.
#include <array>
#include <cmath>
#include <iostream>
#include "neuronapi.h"

using std::cerr;
using std::endl;

extern "C" void modl_reg(){/* No modl_reg */};

static bool close_to(double got, double want, const char* msg) {
    if (std::fabs(got - want) > 1e-9) {
        cerr << "FAIL: " << msg << " — got " << got << ", want " << want << endl;
        return false;
    }
    return true;
}

static void pt3dadd(Section* sec, double xx, double yy, double zz, double diam) {
    nrn_section_push(sec);
    nrn_double_push(xx);
    nrn_double_push(yy);
    nrn_double_push(zz);
    nrn_double_push(diam);
    nrn_function_call(nrn_symbol("pt3dadd"), 4);
    nrn_double_pop();  // pt3dadd returns a number
    nrn_section_pop();
}

static void pt3dconst(int value) {
    nrn_double_push(value);
    nrn_function_call(nrn_symbol("pt3dconst"), 1);
    nrn_double_pop();
}

int main(void) {
    static std::array<const char*, 4> argv = {"section_length", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;

    // A straight 10 µm section described by two 3-d points.
    Section* s = nrn_section_new("s");
    pt3dadd(s, 0, 0, 0, 3);
    pt3dadd(s, 10, 0, 0, 3);
    ok &= close_to(nrn_section_length_get(s), 10.0, "length from 3-d points");

    // The same assignment through HOC on an identical section, for comparison.
    nrn_hoc_call("create h3");
    nrn_hoc_call("access h3");
    Section* h3 = nrn_cas();
    pt3dadd(h3, 0, 0, 0, 3);
    pt3dadd(h3, 10, 0, 0, 3);

    // A section without 3-d points, length 100 µm (the default). HOC ignores
    // its L assignment under pt3dconst(1) as well, since the guard is global.
    Section* plain = nrn_section_new("plain");
    nrn_hoc_call("create hplain");
    nrn_hoc_call("access hplain");
    Section* hplain = nrn_cas();

    pt3dconst(1);
    nrn_section_length_set(s, 25.0);
    nrn_hoc_call("h3.L = 25");
    ok &= close_to(nrn_section_length_get(h3), 10.0, "HOC ignores L under pt3dconst(1)");
    ok &= close_to(nrn_section_length_get(s), 10.0, "C ignores L under pt3dconst(1)");
    nrn_section_length_set(plain, 25.0);
    nrn_hoc_call("hplain.L = 25");
    ok &= close_to(nrn_section_length_get(hplain),
                   100.0,
                   "HOC ignores L under pt3dconst(1), no 3-d points");
    ok &= close_to(nrn_section_length_get(plain),
                   nrn_section_length_get(hplain),
                   "C matches HOC under pt3dconst(1), no 3-d points");
    pt3dconst(0);

    // With pt3dconst(0) the write takes effect, and the 3-d points are
    // rescaled to the new length.
    nrn_section_length_set(s, 25.0);
    ok &= close_to(nrn_section_length_get(s), 25.0, "L set under pt3dconst(0)");
    nrn_section_length_set(plain, 25.0);
    ok &= close_to(nrn_section_length_get(plain), 25.0, "L set under pt3dconst(0), no 3-d points");

    return ok ? 0 : 1;
}
