// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_segment_diam_get's lazy recompute of 3d-derived diameter.
// When a section's geometry comes from pt3dadd, NEURON rewrites the segment
// diam lazily (gated per-section on recalc_area_). nrn_segment_diam_get must
// trigger that recompute so a read before any geometry pass returns the
// 3d-derived value, matching what Python's seg.diam already does.
// It also checks that x = 0 and x = 1 name the first and last segment.
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

    // A non-positive diameter is clamped during the pending area/resistance
    // recompute. The C accessor must finish that recompute without allowing
    // hoc_execerror's C++ exception to cross the C ABI. Use multiple segments
    // so a fix that merely catches the first clamp error cannot pass.
    Section* zero_diam = nrn_section_new("zero_diam");
    nrn_nseg_set(zero_diam, 3);
    for (int i = 0; i < 3; ++i) {
        double x = (i + 0.5) / 3.0;
        nrn_segment_diam_set(zero_diam, x, 0.0);
    }
    ok &= close_to(nrn_segment_diam_get(zero_diam, 5.0 / 6.0),
                   1e-6,
                   "one getter completes the full zero-diam recompute");
    for (int i = 0; i < 3; ++i) {
        double x = (i + 0.5) / 3.0;
        ok &= close_to(nrn_segment_diam_get(zero_diam, x),
                       1e-6,
                       "zero diam clamped without aborting");
    }

    // x = 0 and x = 1 name the first and last segment, as for any range
    // variable: the HOC expression sec.diam(x) and Python's sec(x).diam read
    // those segments, not the zero-area nodes at the section's ends. Give each
    // segment its own diameter so a read of the wrong segment cannot pass.
    Section* root = nrn_section_new("root");
    Section* kid = nrn_section_new("kid");
    nrn_section_connect(kid, 0, root, 1);
    nrn_nseg_set(root, 3);
    nrn_segment_diam_set(root, 1.0 / 6.0, 2.0);
    nrn_segment_diam_set(root, 0.5, 4.0);
    nrn_segment_diam_set(root, 5.0 / 6.0, 6.0);
    nrn_segment_diam_set(kid, 0.5, 3.0);
    ok &= close_to(nrn_segment_diam_get(root, 0.0), 2.0, "root diam at x = 0 is the first segment");
    ok &= close_to(nrn_segment_diam_get(root, 1.0), 6.0, "root diam at x = 1 is the last segment");
    // kid(0) shares its node with root(1); the diameter is still kid's own.
    ok &= close_to(nrn_segment_diam_get(kid, 0.0), 3.0, "kid diam at x = 0 is kid's segment");
    ok &= close_to(nrn_segment_diam_get(kid, 1.0), 3.0, "kid diam at x = 1 is kid's segment");

    // A write at an end reaches that end's segment and no other.
    nrn_segment_diam_set(root, 0.0, 7.0);
    nrn_segment_diam_set(root, 1.0, 9.0);
    ok &= close_to(nrn_segment_diam_get(root, 1.0 / 6.0), 7.0, "write at x = 0 sets first segment");
    ok &= close_to(nrn_segment_diam_get(root, 0.5), 4.0, "end writes leave the middle segment");
    ok &= close_to(nrn_segment_diam_get(root, 5.0 / 6.0), 9.0, "write at x = 1 sets last segment");
    ok &= close_to(nrn_segment_diam_get(kid, 0.5), 3.0, "write at root(1) leaves kid alone");

    // The accessor agrees with the interpreter's range-variable lookup
    // (nrn_rangevar_get goes through nrn_rangepointer, as sec.diam(x) does).
    Symbol* diam_sym = nrn_symbol("diam");
    for (double x: {0.0, 1.0 / 6.0, 0.5, 5.0 / 6.0, 1.0}) {
        ok &= close_to(nrn_segment_diam_get(root, x),
                       nrn_rangevar_get(diam_sym, root, x),
                       "C accessor matches range-variable lookup");
    }

    return ok ? 0 : 1;
}
