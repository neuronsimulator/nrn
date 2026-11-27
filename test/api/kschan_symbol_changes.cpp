// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// nrn_kschan_symbol_changes counts the symbols a KSChan renames or frees.
#include "neuronapi.h"
#include <array>
#include <iostream>

using std::cerr;
using std::endl;

extern "C" void modl_reg() {}

static bool check(bool cond, const char* msg) {
    if (!cond) {
        cerr << "FAIL: " << msg << endl;
    }
    return cond;
}

int main(void) {
    static std::array<const char*, 4> argv = {"kschan_symbol_changes",
                                              "-nogui",
                                              "-nopython",
                                              nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    ok &= check(nrn_kschan_symbol_changes() == 0, "no KSChan yet");

    nrn_hoc_call(
        "objref ks\n"
        "ks = new KSChan(0)\n"
        "ks.name(\"kA\")\n");
    Symbol* gmax = nrn_symbol("gmax_kA");
    ok &= check(gmax != nullptr, "kA has gmax_kA");
    std::size_t count = nrn_kschan_symbol_changes();

    // A rename moves the channel's symbols in place: the old name is gone and
    // the same symbol answers to the new one.
    nrn_hoc_call("ks.name(\"kB\")\n");
    ok &= check(nrn_kschan_symbol_changes() - count == 5, "rename moves five symbols");
    ok &= check(nrn_symbol("gmax_kA") == nullptr, "old name is gone");
    ok &= check(nrn_symbol("gmax_kB") == gmax, "same symbol under the new name");
    count = nrn_kschan_symbol_changes();

    // A state added and renamed.
    nrn_hoc_call(
        "objref st\n"
        "st = ks.add_hhstate(\"xx\")\n");
    ok &= check(nrn_symbol("xx_kB") != nullptr, "hh state added");
    count = nrn_kschan_symbol_changes();
    nrn_hoc_call("st.name(\"yy\")\n");
    ok &= check(nrn_symbol("xx_kB") == nullptr && nrn_symbol("yy_kB") != nullptr, "state renamed");
    ok &= check(nrn_kschan_symbol_changes() - count == 1, "state rename counted");
    count = nrn_kschan_symbol_changes();

    // A state removed frees its symbol.
    nrn_hoc_call("ks.remove_state(st)\n");
    ok &= check(nrn_symbol("yy_kB") == nullptr, "state symbol freed");
    ok &= check(nrn_kschan_symbol_changes() - count == 1, "state removal counted");
    count = nrn_kschan_symbol_changes();

    // ion() frees e_kB.
    nrn_hoc_call("ks.ion(\"k\")\n");
    ok &= check(nrn_kschan_symbol_changes() - count == 1, "ion frees one symbol");
    ok &= check(nrn_symbol("e_kB") == nullptr, "e_kB freed");
    count = nrn_kschan_symbol_changes();

    // The g/p switch renames gmax to pmax in place: the same symbol, not counted.
    nrn_hoc_call("ks.iv_type(2)\n");
    ok &= check(nrn_kschan_symbol_changes() == count, "iv_type switch not counted");
    ok &= check(nrn_symbol("pmax_kB") == gmax, "gmax is pmax_kB, the same symbol");

    return ok ? 0 : 1;
}
