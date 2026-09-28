// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises the top-level objref and strdef accessors: nrn_symbol_object_get/set
// and nrn_symbol_str_get/set. These read and write the Object* / char* that a
// top-level `objref`/`strdef` stores in the top-level object-data array, which
// nrn_symbol_dataptr cannot return (it is not a double*).
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
    static std::array<const char*, 4> argv = {"symbol_object_string",
                                              "-nogui",
                                              "-nopython",
                                              nullptr};
    nrn_init(3, argv.data());

    bool ok = true;

    // --- strdef get/set ---
    nrn_hoc_call("strdef s");
    nrn_hoc_call("s = \"hello\"");
    Symbol* s = nrn_symbol("s");
    ok &= check(s != nullptr, "strdef symbol resolves");
    const char* sval = nrn_symbol_str_get(s);
    ok &= check(sval != nullptr && std::strcmp(sval, "hello") == 0,
                "str_get reads the strdef value");

    ok &= check(nrn_symbol_str_set(s, "world") == true, "str_set succeeds on a strdef");
    ok &= check(std::strcmp(nrn_symbol_str_get(s), "world") == 0, "str_set updated the value");
    // Confirm HOC sees the written value too (aliases the same storage).
    nrn_hoc_call("hoc_ac_ = strcmp(s, \"world\")");
    ok &= check(*nrn_symbol_dataptr(nrn_symbol("hoc_ac_")) == 0.0, "HOC reads the str_set value");

    // --- objref get/set ---
    nrn_hoc_call("objref o");
    nrn_hoc_call("o = new Vector(3)");
    Symbol* o = nrn_symbol("o");
    ok &= check(o != nullptr, "objref symbol resolves");
    Object* vec = nrn_symbol_object_get(o);
    ok &= check(vec != nullptr, "object_get reads the bound object");
    ok &= check(nrn_vector_capacity(vec) == 3, "the bound object is the Vector(3)");

    // Rebind the objref to a different Vector via object_set.
    Object* vec5 = nrn_object_new(nrn_symbol("Vector"), 0);  // empty Vector
    nrn_object_ref(vec5);
    ok &= check(nrn_symbol_object_set(o, vec5) == true, "object_set succeeds on an objref");
    ok &= check(nrn_symbol_object_get(o) == vec5, "object_set rebound the objref");
    // HOC sees the new binding.
    nrn_hoc_call("hoc_ac_ = o.size()");
    ok &= check(*nrn_symbol_dataptr(nrn_symbol("hoc_ac_")) == 0.0, "HOC sees the rebound object");

    // Clearing to nil.
    ok &= check(nrn_symbol_object_set(o, nullptr) == true, "object_set(NULL) clears the objref");
    ok &= check(nrn_symbol_object_get(o) == nullptr, "objref is nil after clear");

    // --- type mismatches return NULL / false, not a crash ---
    ok &= check(nrn_symbol_object_get(s) == nullptr, "object_get on a strdef returns NULL");
    ok &= check(nrn_symbol_str_get(o) == nullptr, "str_get on an objref returns NULL");
    ok &= check(nrn_symbol_object_set(s, vec5) == false, "object_set on a strdef is rejected");
    ok &= check(nrn_symbol_str_set(o, "x") == false, "str_set on an objref is rejected");

    return ok ? 0 : 1;
}
