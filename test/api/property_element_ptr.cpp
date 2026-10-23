// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
// nrn_property_element_ptr returns a pointer to one element of an object's
// variable, given every index: Matrix.x[i][j], Vector.x[i], Pointer.val and
// point-process variables (scalars and arrays). Errors are reported, not thrown.
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

static Object* object(const char* name) {
    return nrn_symbol_object_get(nrn_symbol(name));
}

static double hoc_value(const char* expr) {
    std::string stmt = std::string("element_ptr_value = ") + expr + "\n";
    nrn_hoc_call(stmt.c_str());
    return *nrn_symbol_dataptr(nrn_symbol("element_ptr_value"));
}

int main(void) {
    static std::array<const char*, 4> argv = {"property_element_ptr",
                                              "-nogui",
                                              "-nopython",
                                              nullptr};
    nrn_init(3, argv.data());
    nrn_hoc_call(
        "element_ptr_value = 0\n"
        "pointee = 4\n"
        "objref mat, vec, ptr, ic, vc\n"
        "mat = new Matrix(3, 4)\n"
        "mat.x[1][2] = 7\n"
        "vec = new Vector(5)\n"
        "vec.indgen()\n"
        "ptr = new Pointer(&pointee)\n"
        "create soma\n"
        "soma ic = new IClamp(0.5)\n"
        "soma vc = new VClamp(0.5)\n"
        "ic.amp = 0.25\n"
        "vc.amp[1] = -20\n");

    bool ok = true;
    char err[256]{};
    double* p = nullptr;
    Object* mat = object("mat");
    Object* vec = object("vec");

    // Matrix.x takes two indices; the pointer is the element HOC sees.
    std::array<int, 2> ij{1, 2};
    ok &= check(nrn_property_element_ptr(mat, "x", ij.data(), 2, &p, err, sizeof(err)) == 0 &&
                    *p == 7,
                "Matrix.x[1][2]");
    *p = 9;
    ok &= check(hoc_value("mat.x[1][2]") == 9, "write through the Matrix pointer");
    ij = {2, 3};
    ok &= check(nrn_property_element_ptr(mat, "x", ij.data(), 2, &p, err, sizeof(err)) == 0 &&
                    *p == 0,
                "Matrix.x[2][3] (last element)");
    ij = {3, 0};
    ok &= check(nrn_property_element_ptr(mat, "x", ij.data(), 2, &p, err, sizeof(err)) != 0 &&
                    p == nullptr && std::strstr(err, "out of range") != nullptr,
                "Matrix row out of range is reported");
    ok &= check(nrn_property_element_ptr(mat, "x", ij.data(), 1, &p, err, sizeof(err)) != 0 &&
                    std::strstr(err, "wrong number of indices") != nullptr,
                "Matrix.x needs two indices");

    // Vector.x takes one.
    int i = 3;
    ok &= check(nrn_property_element_ptr(vec, "x", &i, 1, &p, err, sizeof(err)) == 0 && *p == 3 &&
                    p == nrn_vector_data(vec) + 3,
                "Vector.x[3]");
    i = 5;
    ok &= check(nrn_property_element_ptr(vec, "x", &i, 1, &p, err, sizeof(err)) != 0 &&
                    std::strstr(err, "out of range") != nullptr,
                "Vector index out of range is reported");

    // Pointer.val is a scalar.
    ok &= check(nrn_property_element_ptr(object("ptr"), "val", nullptr, 0, &p, err, sizeof(err)) ==
                        0 &&
                    p == nrn_symbol_dataptr(nrn_symbol("pointee")),
                "Pointer.val");

    // Point processes: a scalar and an array element.
    ok &= check(nrn_property_element_ptr(object("ic"), "amp", nullptr, 0, &p, err, sizeof(err)) ==
                        0 &&
                    *p == 0.25,
                "IClamp.amp");
    i = 1;
    ok &= check(nrn_property_element_ptr(object("vc"), "amp", &i, 1, &p, err, sizeof(err)) == 0 &&
                    *p == -20,
                "VClamp.amp[1]");
    *p = -30;
    ok &= check(nrn_property_array_get(object("vc"), "amp", 1) == -30,
                "write through the point-process pointer");
    i = 3;
    ok &= check(nrn_property_element_ptr(object("vc"), "amp", &i, 1, &p, err, sizeof(err)) != 0 &&
                    std::strstr(err, "out of range") != nullptr,
                "VClamp.amp[3] is out of range");

    // Unknown names and methods.
    ok &= check(nrn_property_element_ptr(vec, "nosuch", nullptr, 0, &p, err, sizeof(err)) != 0,
                "unknown property");
    ok &= check(nrn_property_element_ptr(vec, "size", nullptr, 0, &p, err, sizeof(err)) != 0,
                "a method is not a variable");

    // Failures leave the stack alone.
    nrn_double_push(-7.0);
    ij = {0, 9};
    nrn_property_element_ptr(mat, "x", ij.data(), 2, &p, err, sizeof(err));
    i = 9;
    nrn_property_element_ptr(vec, "x", &i, 1, &p, err, sizeof(err));
    ok &= check(nrn_double_pop() == -7.0, "failures leave the stack alone");

    return ok ? 0 : 1;
}
