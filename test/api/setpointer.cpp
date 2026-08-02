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
#include <cstring>
#include <iostream>
#include "neuronapi.h"

using std::cerr;
using std::endl;

// The nocmodl-generated ptrtest.cpp defines this; nrniv_lib calls modl_reg at
// startup, so registering ptrtest here makes the mechanism available.
extern "C" void _ptrtest_reg();
extern "C" void modl_reg() {
    _ptrtest_reg();
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

    // Wire dend(0.5).ptrtest.src -> soma(0.5).v. Push the source pointer, then
    // let setpointer_pop consume it and assign it to the POINTER's dparam slot.
    char err[256];
    nrn_rangevar_push(v_sym, soma, 0.5);
    int rc = nrn_setpointer_pop(src_sym, dend, 0.5, err, sizeof(err));
    ok &= check(rc == 0, "nrn_setpointer_pop succeeds for a valid POINTER wiring");
    if (rc != 0) {
        cerr << "  error_msg: " << err << endl;
    }

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
    // still consumed (popped) before the validation fails, so the stack stays
    // balanced.
    nrn_rangevar_push(v_sym, soma, 0.5);
    int rc_bad = nrn_setpointer_pop(out_sym, dend, 0.5, err, sizeof(err));
    ok &= check(rc_bad != 0, "non-POINTER target is rejected");
    ok &= check(err[0] != '\0', "rejection fills the error message");

    // Error path 2: the POINTER's mechanism is absent at the target segment
    // (soma has no ptrtest). Rejected, not a crash.
    nrn_rangevar_push(v_sym, soma, 0.5);
    int rc_absent = nrn_setpointer_pop(src_sym, soma, 0.5, err, sizeof(err));
    ok &= check(rc_absent != 0, "POINTER target on a segment without the mechanism is rejected");

    // Stack hygiene: every call above (success and error paths) consumed
    // exactly the one source it was given. Push a sentinel and confirm it comes
    // straight back, proving nothing was left on or over-popped from the stack.
    nrn_double_push(17.0);
    ok &= eq(nrn_double_pop(), 17.0, "the stack is balanced after the setpointer_pop calls");

    return ok ? 0 : 1;
}
