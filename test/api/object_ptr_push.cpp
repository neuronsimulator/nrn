// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// Exercises nrn_object_ptr_push, which pushes a writable object-reference cell
// (an Object**) rather than an object by value. This is the out-parameter form
// behind the h.ref(obj) idiom: a callee that assigns to its $oN arg writes back
// through the cell. nrn_object_push (by value) cannot express this.
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
    static std::array<const char*, 4> argv = {"object_ptr_push", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;

    // A proc whose sole job is to write a fresh Vector back into its objref arg.
    nrn_hoc_call("proc make3() { $o1 = new Vector(3) }");
    // And one that reads the passed object's size into the built-in hoc_ac_.
    nrn_hoc_call("proc readsize() { hoc_ac_ = $o1.size() }");

    // Write-back: push the address of an empty cell, call make3, and confirm the
    // cell was populated. A by-value push could not update `slot` here.
    Object* slot = nullptr;
    nrn_object_ptr_push(&slot);
    nrn_function_call(nrn_symbol("make3"), 1);
    ok &= check(slot != nullptr, "objref cell was written back by the callee");
    ok &= check(nrn_vector_capacity(slot) == 3, "written-back Vector has capacity 3");

    // Read direction: push the now-populated cell and have HOC read $o1.size().
    nrn_object_ptr_push(&slot);
    nrn_function_call(nrn_symbol("readsize"), 1);
    double* ac = nrn_symbol_dataptr(nrn_symbol("hoc_ac_"));
    ok &= check(ac != nullptr && *ac == 3.0, "HOC read the pushed object's size through the cell");

    nrn_object_unref(slot);
    return ok ? 0 : 1;
}
