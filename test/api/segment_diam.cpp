// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_segment_diam_get's lazy recompute of 3d-derived diameter.
// When a section's geometry comes from pt3dadd, NEURON rewrites the segment
// diam lazily (gated per-section on recalc_area_). nrn_segment_diam_get must
// trigger that recompute so a read before any geometry pass returns the
// 3d-derived value, matching what Python's seg.diam already does.
// It also checks that x = 0 and x = 1 name the first and last segment, and
// that a diam write on a section with 3-d points updates those points without
// copying stale segment diameters into them.
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

static double diam3d(Section* sec, int i) {
    nrn_section_push(sec);
    nrn_double_push(i);
    nrn_function_call(nrn_symbol("diam3d"), 1);
    double const d = nrn_double_pop();
    nrn_section_pop();
    return d;
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

    // On a section with 3-d points, a diam write must reach the points, or the
    // next area recompute (which derives diam from them) undoes it.
    Section* line = nrn_section_new("line");
    pt3dadd(line, 0, 0, 0, 3);
    pt3dadd(line, 10, 0, 0, 3);
    nrn_segment_diam_set(line, 0.5, 42.0);
    ok &= close_to(nrn_segment_diam_get(line, 0.5), 42.0, "3-d diam write survives a read");
    ok &= close_to(diam3d(line, 0), 42.0, "3-d diam write reaches the first point");
    ok &= close_to(diam3d(line, 1), 42.0, "3-d diam write reaches the last point");

    // With several segments, each 3-d point takes the diameter of the segment
    // it lies in, and the segment diameters are then recomputed from the
    // points. Compare with the same write made by the HOC interpreter on an
    // identical section. Both sections are read first, so their diameters are
    // already derived from the points when the write happens.
    nrn_hoc_call("create hoc3d");
    nrn_hoc_call("access hoc3d");
    Section* const hoc3d = nrn_cas();
    Section* const api3d = nrn_section_new("api3d");
    for (Section* sec: {hoc3d, api3d}) {
        nrn_nseg_set(sec, 3);
        for (int i = 0; i < 3; ++i) {
            pt3dadd(sec, 10.0 * i, 0, 0, 3);
        }
        ok &= close_to(nrn_segment_diam_get(sec, 0.5), 3.0, "uniform 3-d section read");
    }
    nrn_hoc_call("hoc3d.diam(0.5) = 42");
    nrn_segment_diam_set(api3d, 0.5, 42.0);
    for (int i = 0; i < 3; ++i) {
        ok &= close_to(diam3d(api3d, i), diam3d(hoc3d, i), "3-d point diam matches HOC");
    }
    for (double x: {1.0 / 6.0, 0.5, 5.0 / 6.0}) {
        ok &= close_to(nrn_segment_diam_get(api3d, x),
                       nrn_segment_diam_get(hoc3d, x),
                       "segment diam after 3-d write matches HOC");
    }
    ok &= close_to(diam3d(api3d, 1), 42.0, "middle 3-d point takes the new diam");

    // The same write on a fresh section, before anything has derived its
    // segment diameters from the 3-d points. The untouched segments must not
    // copy their stale default diameter (500 µm) into the 3-d points: the end
    // points keep their 3 µm, and the result is the same as for api3d, which
    // was read before the write.
    Section* const fresh = nrn_section_new("fresh");
    nrn_nseg_set(fresh, 3);
    for (int i = 0; i < 3; ++i) {
        pt3dadd(fresh, 10.0 * i, 0, 0, 3);
    }
    nrn_segment_diam_set(fresh, 0.5, 42.0);
    ok &= close_to(diam3d(fresh, 0), 3.0, "fresh 3-d write keeps the first point's diam");
    ok &= close_to(diam3d(fresh, 1), 42.0, "fresh 3-d write sets the middle point's diam");
    ok &= close_to(diam3d(fresh, 2), 3.0, "fresh 3-d write keeps the last point's diam");
    for (double x: {1.0 / 6.0, 0.5, 5.0 / 6.0}) {
        ok &= close_to(nrn_segment_diam_get(fresh, x),
                       nrn_segment_diam_get(api3d, x),
                       "fresh 3-d write matches the write after a read");
    }

    return ok ? 0 : 1;
}
