// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_segment_diam_get's lazy recompute of 3d-derived diameter.
// When a section's geometry comes from pt3dadd, NEURON rewrites the segment
// diam lazily (gated per-section on recalc_area_). nrn_segment_diam_get must
// trigger that recompute so a read before any geometry pass returns the
// 3d-derived value, matching what Python's seg.diam already does.
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

int main(void) {
    static std::array<const char*, 4> argv = {"segment_diam", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    // Uniform 3d geometry: two points, both diam 10, so diam is 10 everywhere.
    Section* s = nrn_section_new("s");
    pt3dadd(s, 0, 0, 0, 10);
    pt3dadd(s, 10, 0, 0, 10);

    bool ok = true;

    // Read BEFORE any geometry pass. Without the recompute this returns the
    // 500.0 default; with it, the 3d-derived 10.0.
    ok &= close_to(nrn_segment_diam_get(s, 0.5), 10.0, "diam read before geometry pass");

    // A geometry pass must not change the already-correct value.
    nrn_double_push(-65);
    nrn_function_call(nrn_symbol("finitialize"), 1);
    nrn_double_pop();
    ok &= close_to(nrn_segment_diam_get(s, 0.5), 10.0, "diam read after finitialize");

    return ok ? 0 : 1;
}
