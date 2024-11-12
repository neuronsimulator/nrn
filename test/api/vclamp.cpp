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

constexpr std::array<double, 7> EXPECTED_V{
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
    char* temp_str = strdup("stdrun.hoc");
    nrn_str_push(&temp_str);
    nrn_function_call(nrn_symbol("load_file"), 1);
    nrn_double_pop();
    free(temp_str);

    // topology
    Section* soma = nrn_section_new("soma");

    // define soma morphology with two 3d points
    nrn_section_push(soma);
    assert(soma);
    assert(nrn_section_length_set);
    nrn_section_length_set(soma, 10);
    nrn_segment_diam_set(soma, 0.5, 3);

    // voltage clamp at soma(0.5)
    nrn_double_push(0.5);
    Object* vclamp = nrn_object_new(nrn_symbol("VClamp"), 1);
    // 0 mV for 1 ms; 10 mV for the next 2 ms; 5 mV for the next 3 ms
    int i = 0;
    for (auto& [amp, dur]: std::initializer_list<std::pair<int, double>>{{0, 1}, {10, 2}, {5, 3}}) {
        nrn_property_array_set(vclamp, "amp", i, amp);
        nrn_property_array_set(vclamp, "dur", i, dur);
        ++i;
    }
    // setup recording
    Object* v = nrn_object_new(nrn_symbol("Vector"), 0);
    nrn_rangevar_push(nrn_symbol("v"), soma, 0.5);
    nrn_double_push(1);
    nrn_method_call(v, nrn_method_symbol(v, "record"), 2);
    nrn_object_unref(nrn_object_pop());  // record returns the vector

    // finitialize(-65)
    nrn_double_push(-65);
    nrn_function_call(nrn_symbol("finitialize"), 1);
    nrn_double_pop();

    // continuerun(6)
    nrn_double_push(6);
    nrn_function_call(nrn_symbol("continuerun"), 1);
    nrn_double_pop();

    if (!approximate(EXPECTED_V, v)) {
        return 1;
    }
}
