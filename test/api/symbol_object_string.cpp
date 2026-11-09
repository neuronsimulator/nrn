// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises the top-level objref and strdef accessors: nrn_symbol_object_get/set
// and nrn_symbol_str_get/set. These read and write the Object* / char* that a
// top-level `objref`/`strdef` stores in the top-level object-data array, which
// nrn_symbol_dataptr cannot return (it is not a double*).
#include <array>
#include <cstring>
#include <iostream>
#include <string>
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

// Evaluate a HOC expression through hoc_ac_, so the test sees what HOC sees.
static double hoc_eval(const std::string& expr) {
    nrn_hoc_call(("hoc_ac_ = " + expr).c_str());
    return *nrn_symbol_dataptr(nrn_symbol("hoc_ac_"));
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

    // --- object_set when the new object is kept alive only by the old binding ---
    // HOC assignment retains the new object before releasing the old one; the
    // setter must too, or the new object is freed before it is bound.
    nrn_hoc_call(
        "func vector_count() { localobj l  l = new List(\"Vector\")  return l.count() }\n"
        "o = new Vector(5)\n");
    const double vectors = hoc_eval("vector_count()");
    Object* sole = nrn_symbol_object_get(o);
    ok &= check(nrn_symbol_object_set(o, sole), "object_set to the bound object succeeds");
    ok &= check(nrn_symbol_object_get(o) == sole, "self-assignment keeps the binding");
    ok &= check(hoc_eval("vector_count()") == vectors, "self-assignment frees no Vector");
    ok &= check(hoc_eval("o.size()") == 5.0, "self-assigned Vector is intact");

    // The new object is an element of the List being replaced.
    nrn_hoc_call("o = new List()\n{ o.append(new Vector(7)) }\n");
    Object* list = nrn_symbol_object_get(o);
    char err[256]{};
    nrn_double_push(0);
    ok &= check(
        nrn_method_call_nothrow(list, nrn_method_symbol(list, "object"), 1, err, sizeof(err)) == 0,
        "List.object(0) succeeds");
    Object* element = nrn_object_pop();
    nrn_object_unref(element);  // the List now holds the only reference
    ok &= check(nrn_symbol_object_set(o, element), "object_set to a List element succeeds");
    ok &= check(nrn_symbol_object_get(o) == element, "objref is bound to the former element");
    ok &= check(hoc_eval("vector_count()") == vectors, "the former element is not freed");
    ok &= check(hoc_eval("o.size()") == 7.0, "the former element is intact");

    // --- type mismatches return NULL / false, not a crash ---
    ok &= check(nrn_symbol_object_get(s) == nullptr, "object_get on a strdef returns NULL");
    ok &= check(nrn_symbol_str_get(o) == nullptr, "str_get on an objref returns NULL");
    ok &= check(nrn_symbol_object_set(s, vec5) == false, "object_set on a strdef is rejected");
    ok &= check(nrn_symbol_str_set(o, "x") == false, "str_set on an objref is rejected");

    return ok ? 0 : 1;
}
