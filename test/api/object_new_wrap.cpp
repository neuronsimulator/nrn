// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_object_new_wrap, which wraps a caller-owned C++ payload as a HOC
// object of a C++ class, storing the payload as the object's this_pointer. This
// is distinct from nrn_object_new, which runs the HOC constructor. SectionList
// is a convenient always-present C++ class here: the public nrn_sectionlist_data
// reads an object's this_pointer back, so the payload round-trip is observable
// without depending on Python or a custom mechanism.
#include <array>
#include <cstring>
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

int main(void) {
    static std::array<const char*, 4> argv = {"object_new_wrap", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;

    Symbol* sl_sym = nrn_symbol("SectionList");
    ok &= check(sl_sym != nullptr, "SectionList symbol exists");

    // Wrap a stand-in payload. `backing` is never dereferenced as a SectionList;
    // we only confirm the pointer is stored and read back. The wrapped object is
    // ref'd (refcount 0 -> 1) so it is not destroyed, and no SectionList method
    // is called on it.
    int backing = 0;
    Object* wrapped = nrn_object_new_wrap(sl_sym, &backing);
    ok &= check(wrapped != nullptr, "wrap returns a non-null object");
    nrn_object_ref(wrapped);
    ok &= check(std::strcmp(nrn_class_name(wrapped), "SectionList") == 0,
                "wrapped object's class is SectionList");
    ok &= check(static_cast<void*>(nrn_sectionlist_data(wrapped)) == static_cast<void*>(&backing),
                "the payload was stored as the object's backing pointer");

    // A null payload is valid too (construct now, fill in the backing later) and
    // yields an object with a null this_pointer.
    Object* empty = nrn_object_new_wrap(sl_sym, nullptr);
    ok &= check(empty != nullptr, "null-payload wrap returns a non-null object");
    nrn_object_ref(empty);
    ok &= check(nrn_sectionlist_data(empty) == nullptr, "null payload stored as null backing");

    return ok ? 0 : 1;
}
