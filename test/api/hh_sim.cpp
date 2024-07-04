// NOTE: this assumes neuronapi.h is on your CPLUS_INCLUDE_PATH
#include "neuronapi.h"

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

constexpr std::initializer_list<double> EXPECTED_V{
#ifndef CORENEURON_ENABLED
    -0x1.04p+6,
    -0x1.b254ad82e20edp+5,
    -0x1.24a52af1ab463p+6,
#else
    -0x1.04p+6,
    -0x1.b0c75635b5bdbp+5,
    -0x1.24a84bedb7246p+6,
#endif
};

extern "C" void modl_reg(){/* No modl_reg */};

int main(void) {
    static std::array<const char*, 4> argv = {"hh_sim", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    // load the stdrun library
    nrn_function_call_s("load_file", "stdrun.hoc");

    // topology
    Section* soma = nrn_section_new("soma");
    nrn_nseg_set(soma, 3);

    // define soma morphology with two 3d points
    nrn_section_push(soma);
    nrn_function_call("pt3dadd", "dddd", 0., 0., 0., 10.);
    nrn_function_call("pt3dadd", "dddd", 10., 0., 0., 10.);

    // ion channels
    nrn_mechanism_insert(soma, nrn_symbol("hh"));

    // current clamp at soma(0.5)
    Object* iclamp = nrn_object_new("IClamp", "d", 0.5);
    nrn_property_set(iclamp, "amp", 0.3);
    nrn_property_set(iclamp, "del", 1.);
    nrn_property_set(iclamp, "dur", 0.1);

    // setup recording
    RangeVar ref_v = nrn_rangevar_new(soma, 0.5, "v");
    Object* v = nrn_object_new_NoArgs("Vector");
    auto r = nrn_method_call(v, "record", "rd", &ref_v, 5.0);
    // Note: we could potentially even forget to drop the result
    // because it is popped from the Stack anyway. The only problem
    // would be the inner object (if any) would never be decref and destroyed
    nrn_result_drop(&r);

    nrn_function_call("finitialize", "d", -65.0);
    nrn_function_call("continuerun", "d", 10.5);

    if (!approximate(EXPECTED_V, v)) {
        return 1;
    }
}
