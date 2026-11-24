// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// A diam write on a section with 3-d points must not depend on whether the
// segment diameters had already been derived from those points. After
// pt3dadd, that only happens at the next area recompute, and a diam write
// copies every segment's diam into the 3-d points. So if the untouched
// segments still held their stale default (500 µm), the points' own diameters
// were overwritten. Compare a write on a fresh section with the same write on
// an identical section whose diameters were read first.
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

// Three 3-d points 10 µm apart, all diam 3 µm, and one segment per point.
static Section* make(const char* name) {
    Section* sec = nrn_section_new(name);
    nrn_nseg_set(sec, 3);
    for (int i = 0; i < 3; ++i) {
        pt3dadd(sec, 10.0 * i, 0, 0, 3);
    }
    return sec;
}

int main(void) {
    static std::array<const char*, 4> argv = {"diam3d_pending", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    Symbol* diam_sym = nrn_symbol("diam");

    Section* fresh = make("fresh");
    Section* reference = make("reference");
    // Derive reference's segment diameters from its 3-d points.
    ok &= close_to(nrn_segment_diam_get(reference, 0.5), 3.0, "reference read");

    nrn_rangevar_set(diam_sym, fresh, 0.5, 42.0);
    nrn_rangevar_set(diam_sym, reference, 0.5, 42.0);

    // The middle point lies in the written segment; the end points do not.
    ok &= close_to(diam3d(fresh, 0), 3.0, "first point keeps its diam");
    ok &= close_to(diam3d(fresh, 1), 42.0, "middle point takes the new diam");
    ok &= close_to(diam3d(fresh, 2), 3.0, "last point keeps its diam");
    for (int i = 0; i < 3; ++i) {
        ok &= close_to(diam3d(fresh, i), diam3d(reference, i), "3-d diam matches reference");
    }
    for (double x: {1.0 / 6.0, 0.5, 5.0 / 6.0}) {
        ok &= close_to(nrn_segment_diam_get(fresh, x),
                       nrn_segment_diam_get(reference, x),
                       "segment diam matches reference");
    }

    return ok ? 0 : 1;
}
