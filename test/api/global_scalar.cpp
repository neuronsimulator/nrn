// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_symbol_dataptr on a NOTUSER runtime scalar (created by HOC
// `x = 42`). Its value is stored in the top-level object-data array, not at
// sym->u.pval, so before the fix the returned pointer was an offset cast to a
// pointer and dereferencing it was undefined. The pointer must now alias the
// same storage HOC reads and writes.
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

    // NOTUSER: a runtime scalar. Its data lives in the top-level object-data
    // array, not at sym->u.pval -- this is the case the fix addresses.
    nrn_hoc_call("myvar = 42");
    double* p = nrn_symbol_dataptr(nrn_symbol("myvar"));
    ok &= check(p != nullptr, "NOTUSER dataptr is non-null");
    ok &= eq(*p, 42.0, "NOTUSER dataptr dereferences to 42");

    // The pointer must alias the storage HOC uses: write through it, then have
    // HOC copy myvar into the built-in hoc_ac_ and confirm HOC saw the change.
    *p = 3.5;
    nrn_hoc_call("hoc_ac_ = myvar");
    double* ac = nrn_symbol_dataptr(nrn_symbol("hoc_ac_"));
    ok &= eq(*ac, 3.5, "HOC reads the value written through the NOTUSER dataptr");

    // USERDOUBLE built-in (t): dataptr already worked; make sure it still does.
    double* t = nrn_symbol_dataptr(nrn_symbol("t"));
    *t = 12.0;
    nrn_hoc_call("hoc_ac_ = t");
    ok &= eq(*ac, 12.0, "USERDOUBLE dataptr round-trips through HOC");

    // Symbols with no data pointer must return nullptr, not a reinterpreted
    // union member the caller would dereference as garbage.
    ok &= check(nrn_symbol_dataptr(nullptr) == nullptr, "null symbol -> nullptr");

    // A function symbol (finitialize) is non-null but has no dataptr.
    Symbol* fi = nrn_symbol("finitialize");
    ok &= check(fi != nullptr, "finitialize symbol exists");
    ok &= check(nrn_symbol_dataptr(fi) == nullptr, "function symbol -> nullptr");

    // Top-level object and string variables are not double storage.
    nrn_hoc_call("objref myobj");
    Symbol* obj = nrn_symbol("myobj");
    ok &= check(obj != nullptr, "objref symbol exists");
    ok &= check(nrn_symbol_dataptr(obj) == nullptr, "objref -> nullptr");

    nrn_hoc_call("strdef mystr");
    Symbol* str = nrn_symbol("mystr");
    ok &= check(str != nullptr, "strdef symbol exists");
    ok &= check(nrn_symbol_dataptr(str) == nullptr, "strdef -> nullptr");

    // A section-level property (USERPROPERTY: L, nseg, ...) has no global
    // storage pointer.
    Symbol* len = nrn_symbol("L");
    ok &= check(len != nullptr, "L symbol exists");
    ok &= check(nrn_symbol_dataptr(len) == nullptr, "section-level property -> nullptr");

    return ok ? 0 : 1;
}
