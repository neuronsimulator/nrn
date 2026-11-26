// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// nrn_rangevar_get_or_nan reads a range variable in one call and returns NaN,
// instead of throwing, when the value cannot be read. A valid read is the same
// double nrn_rangevar_get returns.
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include "neuronapi.h"

using std::cerr;
using std::endl;

extern "C" void modl_reg() {}

static bool check(bool cond, const char* msg) {
    if (!cond) {
        cerr << "FAIL: " << msg << endl;
    }
    return cond;
}

static bool same_bits(double a, double b) {
    return std::memcmp(&a, &b, sizeof(double)) == 0;
}

int main(void) {
    static std::array<const char*, 4> argv = {"rangevar_get_or_nan",
                                              "-nogui",
                                              "-nopython",
                                              nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    Section* soma = nrn_section_new("soma");
    nrn_nseg_set(soma, 3);
    Symbol* v = nrn_symbol("v");
    Symbol* m_hh = nrn_symbol("m_hh");
    Symbol* gnabar = nrn_symbol("gnabar_hh");
    Symbol* diam = nrn_symbol("diam");

    // Valid reads: exact values, and the same bits as nrn_rangevar_get.
    nrn_rangevar_set(v, soma, 0.5, -61.5);
    ok &= check(nrn_rangevar_get_or_nan(v, soma, 0.5) == -61.5, "v");
    nrn_mechanism_insert(soma, nrn_symbol("hh"));
    ok &= check(nrn_rangevar_get_or_nan(gnabar, soma, 0.5) == 0.12, "gnabar_hh default");
    nrn_double_push(-65.0);
    nrn_function_call(nrn_symbol("finitialize"), 1);
    nrn_double_pop();
    for (double x: {1.0 / 6, 0.5, 5.0 / 6}) {
        for (Symbol* sym: {v, m_hh, gnabar, diam}) {
            ok &= check(same_bits(nrn_rangevar_get_or_nan(sym, soma, x),
                                  nrn_rangevar_get(sym, soma, x)),
                        "matches nrn_rangevar_get");
        }
    }
    ok &= check(nrn_rangevar_get_or_nan(v, soma, 0.5) == -65.0, "v after finitialize");
    ok &= check(nrn_rangevar_get_or_nan(m_hh, soma, 0.5) == nrn_rangevar_get(m_hh, soma, 0.5) &&
                    nrn_rangevar_get_or_nan(m_hh, soma, 0.5) > 0.05 &&
                    nrn_rangevar_get_or_nan(m_hh, soma, 0.5) < 0.06,
                "m_hh at rest");

    // v at each node, including the ends, is the node HOC's sec.v(x) uses.
    nrn_hoc_call(
        "create cable\n"
        "access cable\n"
        "cable { nseg = 3 }\n"
        "cable.v(0) = -70\n"
        "cable.v(0.1) = -71\n"
        "cable.v(0.5) = -72\n"
        "cable.v(0.9) = -73\n"
        "cable.v(1) = -74\n");
    Section* cable = nrn_cas();
    ok &= check(nrn_rangevar_get_or_nan(v, cable, 0.0) == -70.0, "v(0)");
    ok &= check(nrn_rangevar_get_or_nan(v, cable, 1.0 / 6) == -71.0, "v(1/6)");
    ok &= check(nrn_rangevar_get_or_nan(v, cable, 0.5) == -72.0, "v(0.5)");
    ok &= check(nrn_rangevar_get_or_nan(v, cable, 5.0 / 6) == -73.0, "v(5/6)");
    ok &= check(nrn_rangevar_get_or_nan(v, cable, 1.0) == -74.0, "v(1)");

    // diam after pt3dadd is recomputed from the 3-d points, as in HOC.
    nrn_hoc_call(
        "create rod\n"
        "access rod\n"
        "rod { nseg = 1 pt3dadd(0, 0, 0, 10) pt3dadd(100, 0, 0, 20) }\n");
    Section* rod = nrn_cas();
    ok &= check(nrn_rangevar_get_or_nan(diam, rod, 0.5) == 15.0, "diam from 3-d points");

    // A diam <= 0 is clamped to 1e-6 without a HOC error.
    nrn_rangevar_set(diam, cable, 0.5, 0.0);
    ok &= check(nrn_rangevar_get_or_nan(diam, cable, 0.5) == 1e-6, "diam 0 reads 1e-6");

    // A mechanism not inserted at that node.
    Section* dend = nrn_section_new("dend");
    ok &= check(std::isnan(nrn_rangevar_get_or_nan(m_hh, dend, 0.5)), "missing mechanism");

    // Position, index and symbol out of range.
    ok &= check(std::isnan(nrn_rangevar_get_or_nan(v, soma, 1.5)), "x > 1");
    ok &= check(std::isnan(nrn_rangevar_get_or_nan(v, soma, -0.1)), "x < 0");
    ok &= check(std::isnan(nrn_rangevar_get_or_nan(v, soma, std::nan(""))), "x is NaN");
    ok &= check(std::isnan(nrn_rangevar_get_or_nan(nrn_symbol("i_membrane_"), soma, 0.5)),
                "i_membrane_ without fast_imem");
    ok &= check(std::isnan(nrn_rangevar_get_or_nan(nrn_symbol("hh"), soma, 0.5)),
                "not a range variable");
    ok &= check(std::isnan(nrn_rangevar_get_or_nan(nullptr, soma, 0.5)), "null symbol");
    ok &= check(std::isnan(nrn_rangevar_get_or_nan(v, nullptr, 0.5)), "null section");

    // A deleted section that the caller still holds a reference to.
    nrn_section_ref(dend);
    nrn_section_push(dend);
    nrn_hoc_call("{delete_section()}\n");
    nrn_section_pop();
    ok &= check(!nrn_section_is_active(dend), "dend is deleted");
    ok &= check(std::isnan(nrn_rangevar_get_or_nan(v, dend, 0.5)), "deleted section");
    nrn_section_unref(dend);

    // An array range variable reads element 0.
    nrn_mechanism_insert(soma, nrn_symbol("extracellular"));
    ok &= check(nrn_rangevar_get_or_nan(nrn_symbol("xg"), soma, 0.5) == 1e9, "xg[0]");

    // A stored NaN is returned as NaN; nrn_rangevar_get_nothrow tells it apart.
    nrn_rangevar_set(v, soma, 0.5, std::numeric_limits<double>::quiet_NaN());
    ok &= check(std::isnan(nrn_rangevar_get_or_nan(v, soma, 0.5)), "stored NaN");
    double value = 0;
    char err[256]{};
    ok &= check(nrn_rangevar_get_nothrow(v, soma, 0.5, 0, &value, err, sizeof(err)) == 0 &&
                    std::isnan(value),
                "stored NaN is a successful read");

    return ok ? 0 : 1;
}
