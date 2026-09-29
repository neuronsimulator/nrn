// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_object_new_nothrow, which constructs an object like
// nrn_object_new but reports a HOC constructor error through a return code and
// error buffer instead of letting a C++ exception cross the call boundary.
#include <array>
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
    static std::array<const char*, 4> argv = {"object_new_nothrow", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    char err[256];

    // Success path: a Vector(3) constructs, returns 0, and fills *result.
    Object* v = nullptr;
    nrn_double_push(3);
    int rc = nrn_object_new_nothrow(nrn_symbol("Vector"), 1, &v, err, sizeof(err));
    ok &= check(rc == 0, "successful construction returns 0");
    ok &= check(v != nullptr, "successful construction sets *result");
    ok &= check(v != nullptr && nrn_vector_capacity(v) == 3, "constructed the requested Vector(3)");
    ok &= check(err[0] == '\0', "error buffer stays empty on success");

    // Failure path: a template whose init calls execerror. nrn_object_new would
    // throw; nrn_object_new_nothrow must catch it.
    nrn_hoc_call("begintemplate Boom\nproc init() { execerror(\"boom\", \"\") }\nendtemplate Boom");
    Object* b = reinterpret_cast<Object*>(0x1);  // sentinel; must be nulled
    rc = nrn_object_new_nothrow(nrn_symbol("Boom"), 0, &b, err, sizeof(err));
    ok &= check(rc != 0, "a failing constructor returns nonzero");
    ok &= check(b == nullptr, "*result is NULL after a failed construction");
    ok &= check(err[0] != '\0', "error buffer is populated on failure");

    // A failed construction must not corrupt the stack: a value pushed after it
    // pops back cleanly.
    nrn_double_push(42.0);
    ok &= check(nrn_double_pop() == 42.0, "stack is intact after a failed construction");

    // A NULL error buffer is tolerated on both paths.
    Object* v2 = nullptr;
    rc = nrn_object_new_nothrow(nrn_symbol("Vector"), 0, &v2, nullptr, 0);
    ok &= check(rc == 0 && v2 != nullptr, "NULL error buffer tolerated on success");
    Object* b2 = nullptr;
    rc = nrn_object_new_nothrow(nrn_symbol("Boom"), 0, &b2, nullptr, 0);
    ok &= check(rc != 0 && b2 == nullptr, "NULL error buffer tolerated on failure");

    return ok ? 0 : 1;
}
