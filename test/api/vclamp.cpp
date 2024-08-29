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

extern "C" void modl_reg(){/* No modl_reg */};

constexpr std::initializer_list<double> EXPECTED_V{
    -0x1.04p+6,
    -0x1.00d7f6756215p-182,
    0x1.3ffe5c93f70cep+3,
    0x1.3ffe5c93f70cep+3,
    0x1.3ffe5c93f70cep+2,
    0x1.3ffe5c93f70cep+2,
    0x1.3ffe5c93f70cep+2,
};

int main(void) {
    static std::array<const char*, 4> argv = {"vclamp", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    // load the stdrun library
    // Ensure If arguments are not correct we handle gracefully
    auto res = nrn_function_call("load_file", NRN_NO_ARGS);
    if (res.type == NrnResultType::NRN_ERR) {
        std::cerr << "Err: " << nrn_stack_err() << std::endl;
    }
    // Do it right this time
    nrn_function_call_s("load_file", "stdrun.hoc");
    if (nrn_stack_err() != NULL) {
        std::cerr << "Err: nrn_stack_error not cleared" << std::endl;
        return 1;
    }

    // topology
    Section* soma = nrn_section_new("soma");

    // define soma morphology with two 3d points
    nrn_section_push(soma);
    assert(soma);
    assert(nrn_section_length_set);
    nrn_section_length_set(soma, 10);
    nrn_segment_diam_set(soma, 0.5, 3);

    // voltage clamp at soma(0.5)
    Object* vclamp = nrn_object_new("VClamp", "d", 0.5);
    // 0 mV for 1 ms; 10 mV for the next 2 ms; 5 mV for the next 3 ms
    int i = 0;
    for (auto& [amp, dur]: std::initializer_list<std::pair<int, double>>{{0, 1}, {10, 2}, {5, 3}}) {
        nrn_property_array_set(vclamp, "amp", i, amp);
        nrn_property_array_set(vclamp, "dur", i, dur);
        ++i;
    }

    // setup recording
    Object* v = nrn_object_new_NoArgs("Vector");
    auto ref_v = nrn_rangevar_new(soma, 0.5, "v");
    auto r = nrn_method_call(v, "record", /* "rd" */ NRN_ARG_RANGEVAR NRN_ARG_DOUBLE, &ref_v, 1.0);

    nrn_function_call("finitialize", NRN_ARG_DOUBLE, -65.0);
    nrn_function_call("continuerun", NRN_ARG_DOUBLE, 6.0);

    if (!approximate(EXPECTED_V, v)) {
        return 1;
    }
}
