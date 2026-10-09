// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// nlayer_extracellular(n) returns the layer count whether or not n changes it.
// Setting the current value used to return without pushing a result, so a
// caller popping the return value took whatever lay beneath it on the stack.
#include <array>
#include <iostream>
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

// Call nlayer_extracellular(n), or with no argument if n < 0, and pop its result.
static double call_nlayer(double n, bool& ok) {
    char err[256]{};
    int narg = 0;
    if (n >= 0) {
        nrn_double_push(n);
        narg = 1;
    }
    ok &= check(
        nrn_function_call_nothrow(nrn_symbol("nlayer_extracellular"), narg, err, sizeof(err)) == 0,
        "nlayer_extracellular call succeeds");
    return nrn_double_pop();
}

int main(void) {
    static std::array<const char*, 4> argv = {"nlayer_extracellular",
                                              "-nogui",
                                              "-nopython",
                                              nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    // A sentinel below the calls: each call must leave exactly its result, so
    // the sentinel is on top again at the end.
    nrn_double_push(-7.0);

    ok &= check(call_nlayer(-1, ok) == 2.0, "default is 2 layers");
    ok &= check(call_nlayer(2.0, ok) == 2.0, "setting the current count returns it");
    ok &= check(call_nlayer(3.0, ok) == 3.0, "changing the count returns the new count");
    ok &= check(call_nlayer(3.0, ok) == 3.0, "setting it again returns it");
    ok &= check(call_nlayer(2.0, ok) == 2.0, "changing it back returns the new count");

    ok &= check(nrn_stack_type() == STACK_IS_NUM && nrn_double_pop() == -7.0,
                "no call left or took anything else on the stack");
    return ok ? 0 : 1;
}
