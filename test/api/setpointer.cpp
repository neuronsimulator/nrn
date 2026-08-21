// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_setpointer_pop, which wires an NMODL POINTER range variable to
// the source pointer the caller has pushed onto the stack (the Python-free form
// of the HOC `setpointer` statement / nrn_pointer_assign). Pushing the source
// rather than naming it reuses every existing way of obtaining a pointer. The
// ptrtest density mechanism copies its POINTER `src` into the range variable
// `out` in INITIAL, so after wiring src -> a source double and calling
// finitialize, reading out confirms the POINTER now aliases the source.
// modl_reg registers ptrtest (its generated _ptrtest_reg is compiled into this
// test), the way nrniv registers built-ins.
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include "neuronapi.h"

using std::cerr;
using std::endl;

// The nocmodl-generated ptrtest.cpp / ppptrtest.cpp define these; nrniv_lib
// calls modl_reg at startup, so registering them here makes both mechanisms
// available: ptrtest (a density mechanism) for nrn_setpointer_pop, and PPPtr (a
// point process) for nrn_pp_setpointer_pop.
extern "C" void _ptrtest_reg();
extern "C" void _ppptrtest_reg();
extern "C" void modl_reg() {
    _ptrtest_reg();
    _ppptrtest_reg();
}

static bool check(bool cond, const char* msg) {
    if (!cond) {
        cerr << "FAIL: " << msg << endl;
    }
    return cond;
}

static bool eq(double got, double want, const char* msg) {
    if (got != want) {
        cerr << "FAIL: " << msg << " -- got " << got << ", want " << want << endl;
        return false;
    }
    return true;
}

int main(void) {
    static std::array<const char*, 4> argv = {"setpointer", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;

    // Two sections. dend's ptrtest.src will be wired to soma(0.5).v, so it reads
    // soma's membrane potential rather than its own.
    Section* soma = nrn_section_new("soma");
    Section* dend = nrn_section_new("dend");
    nrn_section_connect(dend, 0, soma, 1);

    Symbol* ptrtest = nrn_symbol("ptrtest");
    ok &= check(ptrtest != nullptr, "ptrtest mechanism registered");
    nrn_mechanism_insert(dend, ptrtest);

    Symbol* src_sym = nrn_symbol("src_ptrtest");  // the POINTER range variable
    Symbol* out_sym = nrn_symbol("out_ptrtest");  // plain range variable
    Symbol* v_sym = nrn_symbol("v");
    ok &= check(src_sym != nullptr && out_sym != nullptr && v_sym != nullptr,
                "ptrtest range-variable symbols resolve");

    // Stack hygiene is checked inline on every call with a sentinel pushed
    // *below* the source: the HOC stack is LIFO, so a correct call pops exactly
    // the one source it was given and leaves the sentinel on top. Popping the
    // sentinel back afterward proves the call neither under-popped (left the
    // source behind) nor over-popped (ate into the sentinel). A sentinel pushed
    // *after* the calls could catch neither -- it only ever probes the current
    // top. SENTINEL is a distinctive value unlikely to arise by chance.
    const double SENTINEL = 424242.0;

    // Wire dend(0.5).ptrtest.src -> soma(0.5).v. Push the source pointer, then
    // let setpointer_pop consume it and assign it to the POINTER's dparam slot.
    char err[256];
    nrn_double_push(SENTINEL);            // sentinel below
    nrn_rangevar_push(v_sym, soma, 0.5);  // source on top
    int rc = nrn_setpointer_pop(src_sym, dend, 0.5, err, sizeof(err));
    ok &= check(rc == 0, "nrn_setpointer_pop succeeds for a valid POINTER wiring");
    if (rc != 0) {
        cerr << "  error_msg: " << err << endl;
    }
    ok &= eq(nrn_double_pop(),
             SENTINEL,
             "setpointer_pop consumed exactly its source (sentinel below intact)");

    // finitialize(-42): every segment's v becomes -42, and ptrtest's INITIAL
    // copies its POINTER (now soma.v) into out. So dend(0.5).out == -42.
    nrn_double_push(-42);
    nrn_function_call(nrn_symbol("finitialize"), 1);
    nrn_double_pop();
    ok &= eq(nrn_rangevar_get(out_sym, dend, 0.5),
             -42.0,
             "POINTER reads the wired source's value after finitialize");

    // Error path 1: a non-POINTER target (out is a plain RANGE var) is rejected
    // with a nonzero return and a message, not a crash. The pushed source is
    // still consumed (popped) before the validation fails, so the sentinel
    // below comes back untouched.
    nrn_double_push(SENTINEL);
    nrn_rangevar_push(v_sym, soma, 0.5);
    int rc_bad = nrn_setpointer_pop(out_sym, dend, 0.5, err, sizeof(err));
    ok &= check(rc_bad != 0, "non-POINTER target is rejected");
    ok &= check(err[0] != '\0', "rejection fills the error message");
    ok &= eq(nrn_double_pop(),
             SENTINEL,
             "rejected setpointer_pop still consumed exactly its source (sentinel intact)");

    // Error path 2: the POINTER's mechanism is absent at the target segment
    // (soma has no ptrtest). Rejected, not a crash; the source is still consumed.
    nrn_double_push(SENTINEL);
    nrn_rangevar_push(v_sym, soma, 0.5);
    int rc_absent = nrn_setpointer_pop(src_sym, soma, 0.5, err, sizeof(err));
    ok &= check(rc_absent != 0, "POINTER target on a segment without the mechanism is rejected");
    ok &= eq(nrn_double_pop(),
             SENTINEL,
             "rejected setpointer_pop (absent mechanism) still consumed exactly its source");

    // -------------------------------------------------------------------------
    // Point-process path: nrn_pp_setpointer_pop. Unlike a density mechanism, a
    // point process is addressed by its instance Object, because several can
    // share one segment. Put TWO PPPtr point processes at the SAME location,
    // soma(0.5), and cross-wire them (each src -> the other's feed) -- the
    // HalfGap shape, where both sides point at the other. (sec, x) cannot tell
    // the two instances apart; the object can.
    nrn_section_push(soma);
    nrn_double_push(0.5);
    Object* pp1 = nrn_object_new(nrn_symbol("PPPtr"), 1);
    nrn_double_push(0.5);
    Object* pp2 = nrn_object_new(nrn_symbol("PPPtr"), 1);
    nrn_section_pop();
    ok &= check(pp1 != nullptr && pp2 != nullptr, "two PPPtr point processes created at soma(0.5)");

    // An unset POINTER has an empty data handle, while an ordinary PARAMETER
    // already has storage. The predicate is the second-line check for callers
    // that receive NaN from a value accessor and need to distinguish an empty
    // handle from a legitimate NaN value.
    ok &= check(!nrn_property_data_handle_is_valid(pp1, "src", 0),
                "unset point-process POINTER has no valid data handle");
    ok &= check(nrn_property_data_handle_is_valid(pp1, "feed", 0),
                "ordinary point-process property has a valid data handle");
    nrn_property_set(pp1, "feed", std::numeric_limits<double>::quiet_NaN());
    ok &= check(std::isnan(nrn_property_get(pp1, "feed")),
                "ordinary property preserves a legitimate NaN value");
    ok &= check(nrn_property_data_handle_is_valid(pp1, "feed", 0),
                "legitimate NaN value still has a valid data handle");

    // Distinct, finitialize-stable source values (feed is a PARAMETER).
    nrn_property_set(pp1, "feed", 10);
    nrn_property_set(pp2, "feed", 20);

    // Cross-wire: pp1.src -> pp2.feed, pp2.src -> pp1.feed. Push the source
    // handle with nrn_property_push, then let nrn_pp_setpointer_pop consume it.
    // Same sentinel-below hygiene check as the density calls above.
    nrn_double_push(SENTINEL);
    nrn_property_push(pp2, "feed");
    int rc_pp1 = nrn_pp_setpointer_pop(pp1, "src", err, sizeof(err));
    ok &= check(rc_pp1 == 0, "nrn_pp_setpointer_pop wires pp1.src -> pp2.feed");
    if (rc_pp1 != 0) {
        cerr << "  error_msg: " << err << endl;
    }
    ok &= eq(nrn_double_pop(),
             SENTINEL,
             "pp_setpointer_pop consumed exactly its source (sentinel intact)");
    nrn_double_push(SENTINEL);
    nrn_property_push(pp1, "feed");
    int rc_pp2 = nrn_pp_setpointer_pop(pp2, "src", err, sizeof(err));
    ok &= check(rc_pp2 == 0, "nrn_pp_setpointer_pop wires pp2.src -> pp1.feed");
    ok &= eq(nrn_double_pop(),
             SENTINEL,
             "second pp_setpointer_pop consumed exactly its source (sentinel intact)");
    ok &= check(nrn_property_data_handle_is_valid(pp1, "src", 0),
                "wired point-process POINTER has a valid data handle");
    ok &= check(nrn_property_data_handle_is_valid(pp2, "src", 0),
                "second wired point-process POINTER has a valid data handle");

    // finitialize(-65): INITIAL runs out = src, so pp1.out reads pp2.feed (20)
    // and pp2.out reads pp1.feed (10). If the wiring were segment-addressed and
    // had grabbed the wrong instance, these would come back swapped or equal.
    nrn_double_push(-65);
    nrn_function_call(nrn_symbol("finitialize"), 1);
    nrn_double_pop();
    ok &= eq(nrn_property_get(pp1, "out"), 20.0, "pp1 read its own wired source (pp2.feed)");
    ok &= eq(nrn_property_get(pp2, "out"), 10.0, "pp2 read its own wired source (pp1.feed)");

    // Error path: a non-POINTER target name (out is a plain RANGE var) is
    // rejected, with the pushed source still consumed so the sentinel below
    // comes back untouched.
    nrn_double_push(SENTINEL);
    nrn_property_push(pp2, "feed");
    int rc_pp_bad = nrn_pp_setpointer_pop(pp1, "out", err, sizeof(err));
    ok &= check(rc_pp_bad != 0, "non-POINTER point-process target is rejected");
    ok &= check(err[0] != '\0', "point-process rejection fills the error message");
    ok &= eq(nrn_double_pop(),
             SENTINEL,
             "rejected pp_setpointer_pop still consumed exactly its source (sentinel intact)");

    return ok ? 0 : 1;
}
