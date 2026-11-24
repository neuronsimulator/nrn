// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// nrn_rangevar_get/set/push_nothrow report an error instead of throwing for
// cases where nrn_rangevar_get/set/push raise a HOC error or throw through the
// caller: a mechanism that is not inserted, an unset POINTER, i_membrane_
// without fast_imem, and array elements (which nrn_rangevar_get/push read from
// the stack).
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include "neuronapi.h"

using std::cerr;
using std::endl;

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

int main(void) {
    static std::array<const char*, 4> argv = {"rangevar_nothrow", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    char err[256]{};
    double value = 0;
    Section* sec = nrn_section_new("soma");
    Symbol* v = nrn_symbol("v");
    Symbol* gnabar = nrn_symbol("gnabar_hh");

    // A scalar that exists, and a write that HOC sees.
    ok &= check(nrn_rangevar_set_nothrow(v, sec, 0.5, 0, -61.5, err, sizeof(err)) == 0, "set v");
    ok &= check(nrn_rangevar_get_nothrow(v, sec, 0.5, 0, &value, err, sizeof(err)) == 0 &&
                    value == -61.5,
                "get v");
    ok &= check(nrn_rangevar_get(v, sec, 0.5) == -61.5, "write is seen by nrn_rangevar_get");

    // Mechanism not inserted: an error, not a throw.
    ok &= check(nrn_rangevar_get_nothrow(gnabar, sec, 0.5, 0, &value, err, sizeof(err)) != 0 &&
                    std::strstr(err, "not inserted") != nullptr,
                "uninserted mechanism is reported");
    nrn_mechanism_insert(sec, nrn_symbol("hh"));
    ok &= check(nrn_rangevar_get_nothrow(gnabar, sec, 0.5, 0, &value, err, sizeof(err)) == 0 &&
                    value == 0.12,
                "inserted mechanism reads its default");

    // Index checks for scalars.
    ok &= check(nrn_rangevar_get_nothrow(gnabar, sec, 0.5, 1, &value, err, sizeof(err)) != 0 &&
                    std::strstr(err, "index out of range") != nullptr,
                "scalar rejects index 1");
    ok &= check(nrn_rangevar_get_nothrow(v, sec, 1.5, 0, &value, err, sizeof(err)) != 0,
                "x outside [0, 1]");
    ok &= check(nrn_rangevar_get_nothrow(nrn_symbol("hh"), sec, 0.5, 0, &value, err, sizeof(err)) !=
                    0,
                "mechanism symbol is not a range variable");

    // Array range variables: element by index, including extracellular layers.
    nrn_mechanism_insert(sec, nrn_symbol("extracellular"));
    Symbol* xg = nrn_symbol("xg");
    Symbol* vext = nrn_symbol("vext");
    ok &= check(nrn_rangevar_set_nothrow(xg, sec, 0.5, 1, 7.25, err, sizeof(err)) == 0,
                "set xg[1]");
    ok &= check(nrn_rangevar_get_nothrow(xg, sec, 0.5, 1, &value, err, sizeof(err)) == 0 &&
                    value == 7.25,
                "get xg[1]");
    ok &= check(nrn_rangevar_get_nothrow(xg, sec, 0.5, 0, &value, err, sizeof(err)) == 0 &&
                    value != 7.25,
                "xg[0] is a different element");
    ok &= check(nrn_rangevar_set_nothrow(vext, sec, 0.5, 1, 3.5, err, sizeof(err)) == 0 &&
                    nrn_rangevar_get_nothrow(vext, sec, 0.5, 1, &value, err, sizeof(err)) == 0 &&
                    value == 3.5,
                "vext[1] round trip");
    ok &= check(nrn_rangevar_get_nothrow(vext, sec, 0.5, 0, &value, err, sizeof(err)) == 0 &&
                    value != 3.5,
                "vext[0] is a different element");
    ok &= check(nrn_rangevar_get_nothrow(xg, sec, 0.5, 2, &value, err, sizeof(err)) != 0 &&
                    std::strstr(err, "index out of range") != nullptr,
                "xg[2] is out of range with 2 layers");

    // An unset POINTER: an error, not a throw.
    nrn_mechanism_insert(sec, nrn_symbol("ptrtest"));
    ok &= check(nrn_rangevar_get_nothrow(
                    nrn_symbol("src_ptrtest"), sec, 0.5, 0, &value, err, sizeof(err)) != 0 &&
                    std::strstr(err, "POINTER is not set") != nullptr,
                "unset POINTER is reported");

    // i_membrane_ needs fast_imem.
    ok &= check(nrn_rangevar_get_nothrow(
                    nrn_symbol("i_membrane_"), sec, 0.5, 0, &value, err, sizeof(err)) != 0 &&
                    std::strstr(err, "use_fast_imem") != nullptr,
                "i_membrane_ without fast_imem is reported");

    // A pointer to an array element, pushed for a function that takes one.
    ok &= check(nrn_rangevar_push_nothrow(xg, sec, 0.5, 1, err, sizeof(err)) == 0, "push xg[1]");
    double* pxg = nrn_double_ptr_pop();
    ok &= check(pxg && *pxg == 7.25, "pushed pointer is xg[1]");
    *pxg = 8.5;
    ok &= check(nrn_rangevar_get_nothrow(xg, sec, 0.5, 1, &value, err, sizeof(err)) == 0 &&
                    value == 8.5,
                "write through the pushed pointer");
    ok &= check(nrn_rangevar_push_nothrow(vext, sec, 0.5, 1, err, sizeof(err)) == 0 &&
                    *nrn_double_ptr_pop() == 3.5,
                "push vext[1]");
    ok &= check(nrn_rangevar_push_nothrow(v, sec, 0.5, 0, err, sizeof(err)) == 0 &&
                    *nrn_double_ptr_pop() == -61.5,
                "push a scalar");
    ok &= check(nrn_rangevar_push_nothrow(xg, sec, 0.5, 2, err, sizeof(err)) != 0 &&
                    std::strstr(err, "index out of range") != nullptr,
                "push xg[2] is out of range");

    // The stack is untouched by failures.
    nrn_double_push(-7.0);
    nrn_rangevar_get_nothrow(gnabar, sec, 0.5, 3, &value, err, sizeof(err));
    nrn_rangevar_push_nothrow(gnabar, sec, 0.5, 3, err, sizeof(err));
    nrn_rangevar_push_nothrow(nrn_symbol("src_ptrtest"), sec, 0.5, 0, err, sizeof(err));
    ok &= check(nrn_double_pop() == -7.0, "failures leave the stack alone");

    // A diam write updates the area, through either setter, as HOC assignment does.
    Section* cyl = nrn_section_new("cyl");
    nrn_section_length_set(cyl, 10.0);
    Symbol* diam = nrn_symbol("diam");
    double* ac = nrn_symbol_dataptr(nrn_symbol("hoc_ac_"));
    double const pi = std::acos(-1.0);
    auto area = [&]() {
        nrn_section_push(cyl);
        nrn_hoc_call("hoc_ac_ = area(0.5)");
        nrn_section_pop();
        return *ac;
    };
    area();  // computed once, so a missed update would leave it stale
    ok &= check(nrn_rangevar_set_nothrow(diam, cyl, 0.5, 0, 2.0, err, sizeof(err)) == 0 &&
                    std::fabs(area() - pi * 2.0 * 10.0) < 1e-9,
                "diam via set_nothrow updates area");
    nrn_rangevar_set(diam, cyl, 0.5, 4.0);
    ok &= check(std::fabs(area() - pi * 4.0 * 10.0) < 1e-9,
                "diam via nrn_rangevar_set updates area");

    return ok ? 0 : 1;
}
