// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
#include "neuronapi.h"
#include <dlfcn.h>

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>

#include "./test_common.h"

using std::cout;
using std::endl;
using std::ofstream;

constexpr std::array<double, 3> EXPECTED_V{
    -0x1.04p+6,
    -0x1.b254ad82e20edp+5,
    -0x1.24a52af1ab463p+6,
};

extern "C" void modl_reg(){/* No modl_reg */};

int main(void) {
    static std::array<const char*, 4> argv = {"hh_sim", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    // load the stdrun library
    char* temp_str = strdup("stdrun.hoc");
    nrn_str_push(&temp_str);
    nrn_function_call(nrn_symbol("load_file"), 1);
    nrn_double_pop();
    free(temp_str);

    // topology
    Section* soma = nrn_section_new("soma");
    nrn_nseg_set(soma, 3);

    // define soma morphology with two 3d points
    nrn_section_push(soma);
    for (double x: {0, 0, 0, 10}) {
        nrn_double_push(x);
    }
    nrn_function_call(nrn_symbol("pt3dadd"), 4);
    nrn_double_pop();  // pt3dadd returns a number
    for (double x: {10, 0, 0, 10}) {
        nrn_double_push(x);
    }
    nrn_function_call(nrn_symbol("pt3dadd"), 4);
    nrn_double_pop();  // pt3dadd returns a number

    // ion channels
    nrn_mechanism_insert(soma, nrn_symbol("hh"));

    // current clamp at soma(0.5)
    nrn_double_push(0.5);
    Object* iclamp = nrn_object_new(nrn_symbol("IClamp"), 1);
    nrn_property_set(iclamp, "amp", 0.3);
    nrn_property_set(iclamp, "del", 1);
    nrn_property_set(iclamp, "dur", 0.1);

    // setup recording
    Object* v = nrn_object_new(nrn_symbol("Vector"), 0);
    nrn_rangevar_push(nrn_symbol("v"), soma, 0.5);
    nrn_double_push(5.);
    nrn_method_call(v, nrn_method_symbol(v, "record"), 2);
    nrn_object_unref(nrn_object_pop());  // record returns the vector

    // finitialize(-65)
    nrn_double_push(-65);
    nrn_function_call(nrn_symbol("finitialize"), 1);
    nrn_double_pop();

    // continuerun(10.5)
    nrn_double_push(10.5);
    nrn_function_call(nrn_symbol("continuerun"), 1);
    nrn_double_pop();

    if (!approximate(EXPECTED_V, v)) {
        return 1;
    }
}
