// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Sections made through the C API can have a name of any length. Checks that
// nrn_secname and the HOC secname() / sectionname() return the whole name
// rather than overflowing a fixed-size buffer.
#include <array>
#include <cstring>
#include <iostream>
#include <string>
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

int main(void) {
    static std::array<const char*, 4> argv = {"long_section_name", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;

    for (size_t len: {100, 255, 256, 511, 512, 1000, 5000}) {
        std::string name(len, 'a');
        Section* sec = nrn_section_new(name.c_str());

        ok &= check(name == nrn_secname(sec), "nrn_secname returns the whole name");

        nrn_section_push(sec);
        ok &= check(nrn_hoc_call("strdef sn0, sn1, sn2") == 0, "strdef");
        ok &= check(nrn_hoc_call("sn0 = secname()") == 0, "secname()");
        ok &= check(nrn_hoc_call("sn1 = secname(0)") == 0, "secname(0)");
        ok &= check(nrn_hoc_call("sectionname(sn2)") == 0, "sectionname()");
        nrn_section_pop();

        for (const char* var: {"sn0", "sn1", "sn2"}) {
            const char* got = nrn_symbol_str_get(nrn_symbol(var));
            ok &= check(got && name == got, "HOC secname/sectionname returns the whole name");
        }
    }

    if (ok) {
        std::cout << "OK" << std::endl;
    }
    return ok ? 0 : 1;
}
