// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// nrn_property_array_get/set/push and nrn_property_data_handle_is_valid on a
// C++ class array (Vector.x) push the index as HOC does, so they reach element
// i; they used to raise "array dimension of x now 1". Matrix.x, which has two
// dimensions, has no element i: get returns NaN, set does nothing, push pushes
// a null pointer and the data handle is not valid, as for an invalid
// point-process handle. Point processes are unchanged.
#include <array>
#include <cmath>
#include <exception>
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

static Object* object(const char* name) {
    return nrn_symbol_object_get(nrn_symbol(name));
}

int main(void) {
    static std::array<const char*, 4> argv = {"property_array_steer",
                                              "-nogui",
                                              "-nopython",
                                              nullptr};
    nrn_init(3, argv.data());
    nrn_hoc_call(
        "objref vec, mat, vc\n"
        "vec = new Vector(5)\n"
        "vec.indgen()\n"
        "mat = new Matrix(2, 2)\n"
        "create soma\n"
        "soma vc = new VClamp(0.5)\n"
        "vc.amp[2] = 5\n");

    bool ok = true;
    Object* vec = object("vec");
    double* data = nrn_vector_data(vec);

    ok &= check(nrn_property_array_get(vec, "x", 3) == 3, "get vec.x[3]");
    nrn_property_array_set(vec, "x", 4, 40);
    ok &= check(data[4] == 40, "set vec.x[4]");
    nrn_property_array_push(vec, "x", 2);
    ok &= check(nrn_double_ptr_pop() == data + 2, "push &vec.x[2]");
    ok &= check(nrn_property_data_handle_is_valid(vec, "x", 1), "vec.x[1] has data");

    // An index out of range is the HOC error HOC itself gives.
    bool raised = false;
    try {
        nrn_property_array_get(vec, "x", 5);
    } catch (const std::exception&) {
        raised = true;
    }
    ok &= check(raised, "vec.x[5] raises");

    // Matrix.x needs two indices, so the one-index accessors find no element.
    Object* mat = object("mat");
    ok &= check(std::isnan(nrn_property_array_get(mat, "x", 0)), "get Matrix.x is NaN");
    nrn_property_array_set(mat, "x", 0, 7);
    nrn_hoc_call("hoc_ac_ = mat.x[0][0]");
    ok &= check(*nrn_symbol_dataptr(nrn_symbol("hoc_ac_")) == 0, "set Matrix.x does nothing");
    nrn_property_array_push(mat, "x", 0);
    ok &= check(nrn_double_ptr_pop() == nullptr, "push Matrix.x is null");
    ok &= check(!nrn_property_data_handle_is_valid(mat, "x", 0), "Matrix.x has no data handle");

    // Point-process arrays are unchanged.
    ok &= check(nrn_property_array_get(object("vc"), "amp", 2) == 5, "get vc.amp[2]");
    nrn_property_array_set(object("vc"), "amp", 2, 6);
    ok &= check(nrn_property_array_get(object("vc"), "amp", 2) == 6, "set vc.amp[2]");

    return ok ? 0 : 1;
}
